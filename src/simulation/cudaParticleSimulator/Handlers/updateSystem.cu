#include "../cudaParticleSimulator.h"

void cudaParticleSimulator::updateSystem()
{
    // update system, funz<ione che verrà usata solo per chiamare altre funzioni, a mò di wrapper
    // La prima fase è P2G
    p2g();
    // vengono applicate forze esterne, per ora solo g = -9.81
    saveGridVelocities();
    applyExternalForces();
    // qua vengono calcolate le divergenze, essenzialmente quanto il fluido si sta comprimendo o espandendo
    pressureSolve();
    g2p();
    computeAdvection();

}

void cudaParticleSimulator::cuda2vulkan(uint32_t currentImage, appState& state)
{
    //printf("Transferring data from CUDA to Vulkan...\n");
    std::vector<glm::vec3> positions(this->numParticles);
    cudaMemcpy(positions.data(), deviceData.pos, this->numParticles * sizeof(float3), cudaMemcpyDeviceToHost);
    cudaDeviceSynchronize();
    memcpy(state.instanceBuffersMapped[currentImage], positions.data(),
            sizeof(positions[0]) * positions.size());
}

__global__ void p2gNormalizeKernel(
    float *u, float *v, float *w,
    float *uW, float *vW, float *wW,
    int nx, int ny, int nz)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    // Normalizza u
    int uSize = (nx + 1) * ny * nz;
    if (idx < uSize && uW[idx] > 1e-6f)
        u[idx] /= uW[idx];

    // Normalizza v
    int vSize = nx * (ny + 1) * nz;
    if (idx < vSize && vW[idx] > 1e-6f)
        v[idx] /= vW[idx];

    // Normalizza w
    int wSize = nx * ny * (nz + 1);
    if (idx < wSize && wW[idx] > 1e-6f)
        w[idx] /= wW[idx];
}

__global__ void p2gKernel(
    float3 *pos, float3 *vel,
    float *u, float *v, float *w,
    float *uW, float *vW, float *wW,
    int *ctype,
    int nx, int ny, int nz,
    float invDx,
    int numParticles)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= numParticles)
        return;
    float3 p = pos[idx];
    float3 v_l = vel[idx];
    // coord
    float gx = p.x * invDx;
    float gy = p.y * invDx;
    float gz = p.z * invDx;
    // shift U
    {
        float ux = gx, uy = gy - 0.5f, uz = gz - 0.5f;
        int i = (int)ux, j = (int)uy, k = (int)uz;
        float fx = ux - i, fy = uy - j, fz = uz - k;

        // 8 pesi trilineari
        // i pesi stanno as significare ( quanto la particella è vicina al nodo)
        // il nodo essenzialmente è il vertice del cubo di riferimenti di ora
        // essendo una mac grid, i valori vanno shiftati ogni volta
        // questo perchè, altrimenti non sarebbe possibile analizzare la velocità in maniera corretta, visto che la velocità di u indipendente dalle altre e vicevesa
        float w000 = (1 - fx) * (1 - fy) * (1 - fz), w100 = fx * (1 - fy) * (1 - fz);
        float w010 = (1 - fx) * fy * (1 - fz), w110 = fx * fy * (1 - fz);
        float w001 = (1 - fx) * (1 - fy) * fz, w101 = fx * (1 - fy) * fz;
        float w011 = (1 - fx) * fy * fz, w111 = fx * fy * fz;

// Stride griglia u: (nx+1) * ny * nz
#define U_IDX(i, j, k) ((i) + (nx + 1) * ((j) + ny * (k)))

        // Clamp per sicurezza ai bordi
        if (i >= 0 && i < nx && j >= 0 && j < ny && k >= 0 && k < nz)
        {
            atomicAdd(&u[U_IDX(i, j, k)], w000 * v_l.x);
            atomicAdd(&uW[U_IDX(i, j, k)], w000);
        }
        if (i + 1 <= nx && j >= 0 && j < ny && k >= 0 && k < nz)
        {
            atomicAdd(&u[U_IDX(i + 1, j, k)], w100 * v_l.x);
            atomicAdd(&uW[U_IDX(i + 1, j, k)], w100);
        }
        if (i >= 0 && i < nx && j + 1 < ny && k >= 0 && k < nz)
        {
            atomicAdd(&u[U_IDX(i, j + 1, k)], w010 * v_l.x);
            atomicAdd(&uW[U_IDX(i, j + 1, k)], w010);
        }
        if (i + 1 <= nx && j + 1 < ny && k >= 0 && k < nz)
        {
            atomicAdd(&u[U_IDX(i + 1, j + 1, k)], w110 * v_l.x);
            atomicAdd(&uW[U_IDX(i + 1, j + 1, k)], w110);
        }
        if (i >= 0 && i < nx && j >= 0 && j < ny && k + 1 < nz)
        {
            atomicAdd(&u[U_IDX(i, j, k + 1)], w001 * v_l.x);
            atomicAdd(&uW[U_IDX(i, j, k + 1)], w001);
        }
        if (i + 1 <= nx && j >= 0 && j < ny && k + 1 < nz)
        {
            atomicAdd(&u[U_IDX(i + 1, j, k + 1)], w101 * v_l.x);
            atomicAdd(&uW[U_IDX(i + 1, j, k + 1)], w101);
        }
        if (i >= 0 && i < nx && j + 1 < ny && k + 1 < nz)
        {
            atomicAdd(&u[U_IDX(i, j + 1, k + 1)], w011 * v_l.x);
            atomicAdd(&uW[U_IDX(i, j + 1, k + 1)], w011);
        }
        if (i + 1 <= nx && j + 1 < ny && k + 1 < nz)
        {
            atomicAdd(&u[U_IDX(i + 1, j + 1, k + 1)], w111 * v_l.x);
            atomicAdd(&uW[U_IDX(i + 1, j + 1, k + 1)], w111);
        }
#undef U_IDX
    }
    // V
    {
        float vx = gx - 0.5f, vy = gy, vz = gz - 0.5f;
        int i = (int)vx, j = (int)vy, k = (int)vz;
        float fx = vx - i, fy = vy - j, fz = vz - k;

        float w000 = (1 - fx) * (1 - fy) * (1 - fz), w100 = fx * (1 - fy) * (1 - fz);
        float w010 = (1 - fx) * fy * (1 - fz), w110 = fx * fy * (1 - fz);
        float w001 = (1 - fx) * (1 - fy) * fz, w101 = fx * (1 - fy) * fz;
        float w011 = (1 - fx) * fy * fz, w111 = fx * fy * fz;

// Stride griglia v: nx * (ny+1) * nz
#define V_IDX(i, j, k) ((i) + nx * ((j) + (ny + 1) * (k)))

        if (i >= 0 && i < nx && j >= 0 && j < ny && k >= 0 && k < nz)
        {
            atomicAdd(&v[V_IDX(i, j, k)], w000 * v_l.y);
            atomicAdd(&vW[V_IDX(i, j, k)], w000);
        }
        if (i + 1 < nx && j >= 0 && j < ny && k >= 0 && k < nz)
        {
            atomicAdd(&v[V_IDX(i + 1, j, k)], w100 * v_l.y);
            atomicAdd(&vW[V_IDX(i + 1, j, k)], w100);
        }
        if (i >= 0 && i < nx && j + 1 <= ny && k >= 0 && k < nz)
        {
            atomicAdd(&v[V_IDX(i, j + 1, k)], w010 * v_l.y);
            atomicAdd(&vW[V_IDX(i, j + 1, k)], w010);
        }
        if (i + 1 < nx && j + 1 <= ny && k >= 0 && k < nz)
        {
            atomicAdd(&v[V_IDX(i + 1, j + 1, k)], w110 * v_l.y);
            atomicAdd(&vW[V_IDX(i + 1, j + 1, k)], w110);
        }
        if (i >= 0 && i < nx && j >= 0 && j < ny && k + 1 < nz)
        {
            atomicAdd(&v[V_IDX(i, j, k + 1)], w001 * v_l.y);
            atomicAdd(&vW[V_IDX(i, j, k + 1)], w001);
        }
        if (i + 1 < nx && j >= 0 && j < ny && k + 1 < nz)
        {
            atomicAdd(&v[V_IDX(i + 1, j, k + 1)], w101 * v_l.y);
            atomicAdd(&vW[V_IDX(i + 1, j, k + 1)], w101);
        }
        if (i >= 0 && i < nx && j + 1 <= ny && k + 1 < nz)
        {
            atomicAdd(&v[V_IDX(i, j + 1, k + 1)], w011 * v_l.y);
            atomicAdd(&vW[V_IDX(i, j + 1, k + 1)], w011);
        }
        if (i + 1 < nx && j + 1 <= ny && k + 1 < nz)
        {
            atomicAdd(&v[V_IDX(i + 1, j + 1, k + 1)], w111 * v_l.y);
            atomicAdd(&vW[V_IDX(i + 1, j + 1, k + 1)], w111);
        }
#undef V_IDX
    }
    // W
    {
        float wx = gx - 0.5f, wy = gy - 0.5f, wz = gz;
        int i = (int)wx, j = (int)wy, k = (int)wz;
        float fx = wx - i, fy = wy - j, fz = wz - k;

        float w000 = (1 - fx) * (1 - fy) * (1 - fz), w100 = fx * (1 - fy) * (1 - fz);
        float w010 = (1 - fx) * fy * (1 - fz), w110 = fx * fy * (1 - fz);
        float w001 = (1 - fx) * (1 - fy) * fz, w101 = fx * (1 - fy) * fz;
        float w011 = (1 - fx) * fy * fz, w111 = fx * fy * fz;

// Stride griglia w: nx * ny * (nz+1)
#define W_IDX(i, j, k) ((i) + nx * ((j) + ny * (k)))

        if (i >= 0 && i < nx && j >= 0 && j < ny && k >= 0 && k < nz)
        {
            atomicAdd(&w[W_IDX(i, j, k)], w000 * v_l.z);
            atomicAdd(&wW[W_IDX(i, j, k)], w000);
        }
        if (i + 1 < nx && j >= 0 && j < ny && k >= 0 && k < nz)
        {
            atomicAdd(&w[W_IDX(i + 1, j, k)], w100 * v_l.z);
            atomicAdd(&wW[W_IDX(i + 1, j, k)], w100);
        }
        if (i >= 0 && i < nx && j + 1 < ny && k >= 0 && k < nz)
        {
            atomicAdd(&w[W_IDX(i, j + 1, k)], w010 * v_l.z);
            atomicAdd(&wW[W_IDX(i, j + 1, k)], w010);
        }
        if (i + 1 < nx && j + 1 < ny && k >= 0 && k < nz)
        {
            atomicAdd(&w[W_IDX(i + 1, j + 1, k)], w110 * v_l.z);
            atomicAdd(&wW[W_IDX(i + 1, j + 1, k)], w110);
        }
        if (i >= 0 && i < nx && j >= 0 && j < ny && k + 1 <= nz)
        {
            atomicAdd(&w[W_IDX(i, j, k + 1)], w001 * v_l.z);
            atomicAdd(&wW[W_IDX(i, j, k + 1)], w001);
        }
        if (i + 1 < nx && j >= 0 && j < ny && k + 1 <= nz)
        {
            atomicAdd(&w[W_IDX(i + 1, j, k + 1)], w101 * v_l.z);
            atomicAdd(&wW[W_IDX(i + 1, j, k + 1)], w101);
        }
        if (i >= 0 && i < nx && j + 1 < ny && k + 1 <= nz)
        {
            atomicAdd(&w[W_IDX(i, j + 1, k + 1)], w011 * v_l.z);
            atomicAdd(&wW[W_IDX(i, j + 1, k + 1)], w011);
        }
        if (i + 1 < nx && j + 1 < ny && k + 1 <= nz)
        {
            atomicAdd(&w[W_IDX(i + 1, j + 1, k + 1)], w111 * v_l.z);
            atomicAdd(&wW[W_IDX(i + 1, j + 1, k + 1)], w111);
        }
#undef W_IDX
    }

    int ci = (int)gx, cj = (int)gy, ck = (int)gz;
    if (ci >= 0 && ci < nx && cj >= 0 && cj < ny && ck >= 0 && ck < nz)
    {
        int cidx = ci + nx * (cj + ny * ck);
        if (ctype[cidx] != (int)cellType::SOLID)
            ctype[cidx] = (int)cellType::FLUID;
    }
}
void cudaParticleSimulator::p2g()
{
    int nx = grid_size.x, ny = grid_size.y, nz = grid_size.z;
    float invDx = 1.0f / grid_size.cellSize;

    // Azzera griglia e pesi
    cudaMemset(grid_data.u, 0, (nx + 1) * ny * nz * sizeof(float));
    cudaMemset(grid_data.v, 0, nx * (ny + 1) * nz * sizeof(float));
    cudaMemset(grid_data.w, 0, nx * ny * (nz + 1) * sizeof(float));
    cudaMemset(grid_data.uWeight, 0, (nx + 1) * ny * nz * sizeof(float));
    cudaMemset(grid_data.vWeight, 0, nx * (ny + 1) * nz * sizeof(float));
    cudaMemset(grid_data.wWeight, 0, nx * ny * (nz + 1) * sizeof(float));

    cudaMemset(grid_data.cellType, (int)cellType::AIR, nx * ny * nz * sizeof(int));
    computeBoundary();

    // P2G kernel — 1 thread per particella
    int blockSize = 256;
    int gridDim = (numParticles + blockSize - 1) / blockSize;
    p2gKernel<<<gridDim, blockSize>>>(
        deviceData.pos, deviceData.vel,
        grid_data.u, grid_data.v, grid_data.w,
        grid_data.uWeight, grid_data.vWeight, grid_data.wWeight,
        grid_data.cellType,
        nx, ny, nz, invDx, numParticles);

    int s1 = (nx + 1) * ny * nz;
    int s2 = nx * (ny + 1) * nz;
    int s3 = nx * ny * (nz + 1);

    int maxSize = s1;
    if (s2 > maxSize)
        maxSize = s2;
    if (s3 > maxSize)
        maxSize = s3;
    int gridNorm = (maxSize + blockSize - 1) / blockSize;
    p2gNormalizeKernel<<<gridNorm, blockSize>>>(
        grid_data.u, grid_data.v, grid_data.w,
        grid_data.uWeight, grid_data.vWeight, grid_data.wWeight,
        nx, ny, nz);

    cudaDeviceSynchronize();
}

void cudaParticleSimulator::saveGridVelocities()
{
    int nx = grid_size.x, ny = grid_size.y, nz = grid_size.z;

    cudaMemcpy(grid_data.u_old, grid_data.u, (nx + 1) * ny * nz * sizeof(float), cudaMemcpyDeviceToDevice);
    cudaMemcpy(grid_data.v_old, grid_data.v, nx * (ny + 1) * nz * sizeof(float), cudaMemcpyDeviceToDevice);
    cudaMemcpy(grid_data.w_old, grid_data.w, nx * ny * (nz + 1) * sizeof(float), cudaMemcpyDeviceToDevice);
}