#include "../cudaParticleSimulator.h"


__global__ void applyExternalForcesKernel(
    float *v,
    int *cellType,
    int nx, int ny, int nz,
    float gravity, float dt)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int j = blockIdx.y * blockDim.y + threadIdx.y;
    int k = blockIdx.z * blockDim.z + threadIdx.z;

    if (i >= nx || j >= ny + 1 || k >= nz)
        return;

    // La faccia v[i,j,k] è tra la cella (i,j-1,k) e (i,j,k)
    // Applica gravità solo se almeno una delle due celle adiacenti è FLUID
    // Non viene applicate se sono solid (quindi boundary) o aria
    bool fluid = false;

    if (j > 0 && j < ny)
    {
        int below = i + nx * ((j - 1) + ny * k);
        int above = i + nx * (j + ny * k);
        fluid = (cellType[below] == (int)cellType::FLUID ||
                 cellType[above] == (int)cellType::FLUID);
    }
    else if (j == 0)
    {
        fluid = (cellType[i + nx * (j + ny * k)] == (int)cellType::FLUID);
    }
    else if (j == ny)
    {
        fluid = (cellType[i + nx * ((j - 1) + ny * k)] == (int)cellType::FLUID);
    }

    if (fluid)
    {
        int vidx = i + nx * (j + (ny + 1) * k);
        v[vidx] += gravity * dt;
    }
}

void cudaParticleSimulator::applyExternalForces()
{
    int nx = grid_size.x, ny = grid_size.y, nz = grid_size.z;

    dim3 block(8, 8, 4);
    dim3 grid(
        (nx + block.x - 1) / block.x,
        (ny + 1 + block.y - 1) / block.y,
        (nz + block.z - 1) / block.z);

    applyExternalForcesKernel<<<grid, block>>>(
        grid_data.v,
        grid_data.cellType,
        nx, ny, nz,
        -9.81f, // es. -9.81f
        deltaTime);

    cudaDeviceSynchronize();
}