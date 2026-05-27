#include "../cudaParticleSimulator.h"

__global__ void computeDivergenceKernel(
    float *u, float *v, float *w,
    float *divergence,
    int *CellType,
    int nx, int ny, int nz,
    float invDx)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int j = blockIdx.y * blockDim.y + threadIdx.y;
    int k = blockIdx.z * blockDim.z + threadIdx.z;

    if (i >= nx || j >= ny || k >= nz)
        return;

    int cidx = i + nx * (j + ny * k);

    if (CellType[cidx] != (int)cellType::FLUID)
    {
        divergence[cidx] = 0.0f;
        return;
    }

    // Indici facce staggered
    float u_right = u[(i + 1) + (nx + 1) * (j + ny * k)];
    float u_left = u[i + (nx + 1) * (j + ny * k)];

    float v_top = v[i + nx * ((j + 1) + (ny + 1) * k)];
    float v_bottom = v[i + nx * (j + (ny + 1) * k)];

    float w_front = w[i + nx * (j + ny * (k + 1))];
    float w_back = w[i + nx * (j + ny * k)];

    divergence[cidx] = (u_right - u_left + v_top - v_bottom + w_front - w_back) * invDx;
}

// qua vengono calcolate le divergenze, essenzialmente quanto il fluido si sta comprimendo o espandendo
void cudaParticleSimulator::computeDivergence()
{
    int nx = grid_size.x, ny = grid_size.y, nz = grid_size.z;

    dim3 block(8, 8, 4);
    dim3 grid(
        (nx + block.x - 1) / block.x,
        (ny + block.y - 1) / block.y,
        (nz + block.z - 1) / block.z);

    computeDivergenceKernel<<<grid, block>>>(
        grid_data.u, grid_data.v, grid_data.w,
        grid_data.divergence,
        grid_data.cellType,
        nx, ny, nz,
        1.0f / grid_size.cellSize);

    cudaDeviceSynchronize();
}

__global__ void jacobiKernel(
    float *p, float *pNew,
    float *divergence,
    int *CellType,
    int nx, int ny, int nz,
    float dx)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int j = blockIdx.y * blockDim.y + threadIdx.y;
    int k = blockIdx.z * blockDim.z + threadIdx.z;

    if (i >= nx || j >= ny || k >= nz)
        return;

    int cidx = i + nx * (j + ny * k);

    if (CellType[cidx] != (int)cellType::FLUID)
    {
        pNew[cidx] = 0.0f;
        return;
    }

    float sum = 0.0f;
    int count = 0;

    // Vicino X-
    if (i > 0 && CellType[cidx - 1] != (int)cellType::SOLID)
    {
        sum += p[cidx - 1];
        count++;
    }
    // Vicino X+
    if (i < nx - 1 && CellType[cidx + 1] != (int)cellType::SOLID)
    {
        sum += p[cidx + 1];
        count++;
    }
    // Vicino Y-
    if (j > 0 && CellType[cidx - nx] != (int)cellType::SOLID)
    {
        sum += p[cidx - nx];
        count++;
    }
    // Vicino Y+
    if (j < ny - 1 && CellType[cidx + nx] != (int)cellType::SOLID)
    {
        sum += p[cidx + nx];
        count++;
    }
    // Vicino Z-
    if (k > 0 && CellType[cidx - nx * ny] != (int)cellType::SOLID)
    {
        sum += p[cidx - nx * ny];
        count++;
    }
    // Vicino Z+
    if (k < nz - 1 && CellType[cidx + nx * ny] != (int)cellType::SOLID)
    {
        sum += p[cidx + nx * ny];
        count++;
    }

    if (count == 0)
    {
        pNew[cidx] = 0.0f;
        return;
    }

    pNew[cidx] = (sum - dx * dx * divergence[cidx]) / count;
}


/*
    Nel pressure solver viene usato un metodo di Jacobi.
    1- All'inizio viene calcolata la divergenza
    2- Viene fatto un reset della pressione
    3- 1 thread per cella-> ogni thread aggiorna la pressione usando i valori dei vicini
    4- usa due array (verrebbe sovrascritto e verrebbero persi i dati)
    Essenzialmente serve a compensare l'errore trovato nel computeDivergence
*/
void cudaParticleSimulator::pressureSolve()
{
    int nx = grid_size.x, ny = grid_size.y, nz = grid_size.z;
    float dx = grid_size.cellSize;

    // Passo 1 — calcola divergenza
    computeDivergence();

    // Passo 2 — azzera pressione
    cudaMemset(grid_data.p, 0, nx * ny * nz * sizeof(float));
    cudaMemset(grid_data.pBuffer, 0, nx * ny * nz * sizeof(float));

    // Passo 3 — Jacobi iterativo
    dim3 block(8, 8, 4);
    dim3 grid(
        (nx + block.x - 1) / block.x,
        (ny + block.y - 1) / block.y,
        (nz + block.z - 1) / block.z);

    for (int iter = 0; iter < numIterations; iter++)
    {
        jacobiKernel<<<grid, block>>>(
            grid_data.p, grid_data.pBuffer,
            grid_data.divergence,
            grid_data.cellType,
            nx, ny, nz, dx);
        cudaDeviceSynchronize();

        // Swappa i puntatori — pBuffer diventa la nuova p
        float *tmp = grid_data.p;
        grid_data.p = grid_data.pBuffer;
        grid_data.pBuffer = tmp;
    }

    // Passo 4 — applica gradiente pressione alle velocità
    applyPressureGradient();
}



/*
    Applica il gradiente di pressione alle velocità della grid.
    InvDx è l'inverso del deltaX, viene passata sottoforma di 1/invDx per evitare div, le molt sono più veloci
    il paramentro risultante gx è la x-esima posizione della griglia (è adimensionale), same per gy e gz
*/
__global__ void applyPressureGradientKernel(
    float *u, float *v, float *w,
    float *p,
    int *cellType,
    int nx, int ny, int nz,
    float invDx, float dt)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int j = blockIdx.y * blockDim.y + threadIdx.y;
    int k = blockIdx.z * blockDim.z + threadIdx.z;

    if (i <= nx && j < ny && k < nz)
    {
        // La faccia u[i,j,k] sta tra cella (i-1,j,k) e cella (i,j,k) (le due celle comunicanti per quella faccia)
        bool leftFluid = (i > 0 && cellType[(i - 1) + nx * (j + ny * k)] == (int)cellType::FLUID);
        bool rightFluid = (i < nx && cellType[i + nx * (j + ny * k)] == (int)cellType::FLUID);
        bool leftSolid = (i > 0 && cellType[(i - 1) + nx * (j + ny * k)] == (int)cellType::SOLID);
        bool rightSolid = (i < nx && cellType[i + nx * (j + ny * k)] == (int)cellType::SOLID);

        if ((leftFluid || rightFluid) && !leftSolid && !rightSolid)
        {
            float pRight = (i < nx) ? p[i + nx * (j + ny * k)] : 0.0f;
            float pLeft = (i > 0) ? p[(i - 1) + nx * (j + ny * k)] : 0.0f;
            u[i + (nx + 1) * (j + ny * k)] -= dt * (pRight - pLeft) * invDx;
        }
    }

    
    if (i < nx && j <= ny && k < nz)
    {
        bool bottomFluid = (j > 0 && cellType[i + nx * ((j - 1) + ny * k)] == (int)cellType::FLUID);
        bool topFluid = (j < ny && cellType[i + nx * (j + ny * k)] == (int)cellType::FLUID);
        bool bottomSolid = (j > 0 && cellType[i + nx * ((j - 1) + ny * k)] == (int)cellType::SOLID);
        bool topSolid = (j < ny && cellType[i + nx * (j + ny * k)] == (int)cellType::SOLID);

        if ((bottomFluid || topFluid) && !bottomSolid && !topSolid)
        {
            float pTop = (j < ny) ? p[i + nx * (j + ny * k)] : 0.0f;
            float pBottom = (j > 0) ? p[i + nx * ((j - 1) + ny * k)] : 0.0f;
            v[i + nx * (j + (ny + 1) * k)] -= dt * (pTop - pBottom) * invDx;
        }
    }

    
    if (i < nx && j < ny && k <= nz)
    {
        bool backFluid = (k > 0 && cellType[i + nx * (j + ny * (k - 1))] == (int)cellType::FLUID);
        bool frontFluid = (k < nz && cellType[i + nx * (j + ny * k)] == (int)cellType::FLUID);
        bool backSolid = (k > 0 && cellType[i + nx * (j + ny * (k - 1))] == (int)cellType::SOLID);
        bool frontSolid = (k < nz && cellType[i + nx * (j + ny * k)] == (int)cellType::SOLID);

        if ((backFluid || frontFluid) && !backSolid && !frontSolid)
        {
            float pFront = (k < nz) ? p[i + nx * (j + ny * k)] : 0.0f;
            float pBack = (k > 0) ? p[i + nx * (j + ny * (k - 1))] : 0.0f;
            w[i + nx * (j + ny * k)] -= dt * (pFront - pBack) * invDx;
        }
    }
}

void cudaParticleSimulator::applyPressureGradient()
{
    int nx = grid_size.x, ny = grid_size.y, nz = grid_size.z;

    dim3 block(8, 8, 4);
    dim3 grid(
        (nx + 1 + block.x - 1) / block.x,
        (ny + 1 + block.y - 1) / block.y,
        (nz + 1 + block.z - 1) / block.z);

    applyPressureGradientKernel<<<grid, block>>>(
        grid_data.u, grid_data.v, grid_data.w,
        grid_data.p,
        grid_data.cellType,
        nx, ny, nz,
        1.0f / grid_size.cellSize,
        deltaTime);

    cudaDeviceSynchronize();
}