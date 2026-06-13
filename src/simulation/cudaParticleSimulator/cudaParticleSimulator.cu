#include "cudaParticleSimulator.h"
#include <vector>
#include <cmath>
#include <iostream>
#include <numeric>
#include <algorithm>
#include <random>
#define CHECK(call)                                                         \
    {                                                                       \
        const cudaError_t error = call;                                     \
        if (error != cudaSuccess)                                           \
        {                                                                   \
            printf("Error %s : %d\n", __FILE__, __LINE__);                  \
            printf("code:%d, reason:%s", error, cudaGetErrorString(error)); \
            exit(1);                                                        \
        }                                                                   \
    }

// init all
/*
@param numParticles: numero di particelle da simulare
@param deltaTime: passo temporale della simulazione
@param h_vkData: dati di vulkan per la condivisione del buffer
@param deviceId: id del dispositivo CUDA da utilizzare
@param fluidProps: proprietà del fluido (densità, viscosità, ecc.)
@param num_iterations: numero di iterazioni per il solver della pressione
@param grid_size: dimensione della griglia (numero di celle in x, y, z)

*/
cudaParticleSimulator::cudaParticleSimulator(int numParticles, float deltaTime, int deviceId, fluidProperties fluidProps, int num_iterations, gridSize grid_size)
{
    srand(time(NULL));
    this->deviceId = deviceId;
    cudaSetDevice(this->deviceId);

    this->numParticles = numParticles;
    this->deltaTime = deltaTime;
    this->fluidProps = fluidProps;
    this->num_iterations = num_iterations;
    this->grid_size = grid_size;
    printf("Initializing CUDA Particle Simulator with %d particles, grid size (%d, %d, %d), and %d pressure iterations\n",
           numParticles, grid_size.x, grid_size.y, grid_size.z, num_iterations);
    cudaMalloc(&deviceData.pos, numParticles * sizeof(float3));
    cudaMalloc(&deviceData.vel, numParticles * sizeof(float3));
    cudaMalloc(&deviceData.old_vel, numParticles * sizeof(float3));
    
    // MAc num components calculations
    // Si aggiunge una cella extra per ogni dimensione per le velocità sulle facce
    // Se ho N celle, avrò N+1 velocità
    int uCells = (grid_size.x + 1) * grid_size.y * grid_size.z;
    int vCells = grid_size.x * (grid_size.y + 1) * grid_size.z;
    int wCells = grid_size.x * grid_size.y * (grid_size.z + 1);
    int centerCells = grid_size.gridCells; // x * y * z

    // Allocazione facce della griglia (Velocità)
    cudaMalloc(&this->grid_data.u, uCells * sizeof(float));
    cudaMalloc(&this->grid_data.v, vCells * sizeof(float));
    cudaMalloc(&this->grid_data.w, wCells * sizeof(float));
    // allocazione old uvw
    cudaMalloc(&this->grid_data.u_old, uCells * sizeof(float));
    cudaMalloc(&this->grid_data.v_old, vCells * sizeof(float));
    cudaMalloc(&this->grid_data.w_old, wCells * sizeof(float));

    // PESI
    cudaMalloc(&grid_data.uWeight, uCells * sizeof(float));
    cudaMalloc(&grid_data.vWeight, vCells * sizeof(float));
    cudaMalloc(&grid_data.wWeight, wCells * sizeof(float));

    cudaMemset(grid_data.uWeight, 0, uCells * sizeof(float));
    cudaMemset(grid_data.vWeight, 0, vCells * sizeof(float));
    cudaMemset(grid_data.wWeight, 0, wCells * sizeof(float));

    // Allocazione centri delle celle (Pressione e Tipo di cella)
    cudaMalloc(&this->grid_data.p, centerCells * sizeof(float));
    cudaMalloc(&this->grid_data.cellType, centerCells * sizeof(int));

    // allocazione per la divergenza e il buffer della pressione
    cudaMalloc(&grid_data.divergence, grid_size.x * grid_size.y * grid_size.z * sizeof(float));
    cudaMalloc(&grid_data.pBuffer, grid_size.x * grid_size.y * grid_size.z * sizeof(float));

    cudaMemset(grid_data.divergence, 0, grid_size.x * grid_size.y * grid_size.z * sizeof(float));
    cudaMemset(grid_data.pBuffer, 0, grid_size.x * grid_size.y * grid_size.z * sizeof(float));

    // conviene sempre fare un po di memsetr per i valori
    cudaMemset(this->grid_data.u, 0, uCells * sizeof(float));
    cudaMemset(this->grid_data.v, 0, vCells * sizeof(float));
    cudaMemset(this->grid_data.w, 0, wCells * sizeof(float));
    cudaMemset(this->grid_data.p, 0, centerCells * sizeof(float));
    // tutta le cella iniziallizata come aria. POoi si mette  la prima
    cudaMemset(this->grid_data.cellType, int(cellType::AIR), centerCells * sizeof(int));
}

cudaParticleSimulator::~cudaParticleSimulator()
{
}

void cudaParticleSimulator::initParticles()
{
    float dx = grid_size.cellSize; // dimensione della cella

    //limiti griglia
    float worldWidth = grid_size.x * dx;
    float worldHeight = grid_size.y * dx;
    float worldDepth = grid_size.z * dx;

    // Posizioniamo la sfera in alto (vicino a worldHeight) e al centro di X e Z
    float3 center = {worldWidth / 2.0f, worldHeight * 0.75f, worldDepth / 2.0f};
    float radius = worldWidth * 0.2f; // Il raggio è il 20% della larghezza del mondo

    //spaziatura tra particelle
    float p_spacing = dx * 0.5f;

    std::vector<float3> h_pos;
    std::vector<float3> h_vel;

    // Bounding box della sfera
    float3 minB = {center.x - radius, center.y - radius, center.z - radius};
    float3 maxB = {center.x + radius, center.y + radius, center.z + radius};

    // Generazione cubo
    for (float x = minB.x; x <= maxB.x; x += p_spacing)
    {
        for (float y = minB.y; y <= maxB.y; y += p_spacing)
        {
            for (float z = minB.z; z <= maxB.z; z += p_spacing)
            {

                float distSq = (x - center.x) * (x - center.x) +
                               (y - center.y) * (y - center.y) +
                               (z - center.z) * (z - center.z);

                if (distSq <= radius * radius)
                {
                    float jitterx = ((rand() / (float)RAND_MAX) - 0.5f) * p_spacing * 0.1f; // Jitter fino al 10% della spaziatura
                    float jittery = ((rand() / (float)RAND_MAX) - 0.5f) * p_spacing * 0.1f; // Jitter fino al 10% della spaziatura
                    float jitterz = ((rand() / (float)RAND_MAX) - 0.5f) * p_spacing * 0.1f; // Jitter fino al 10% della spaziatura
                    h_pos.push_back({x + jitterx, y + jittery, z + jitterz});
                    h_vel.push_back({0.0f, 0.0f, 0.0f}); // Fluido inizialmente fermo
                }
            }
        }
    }

    int generatedCount = h_pos.size();
    if(generatedCount == 0)
    {
        std::cerr << "Error: No particles generated. Check the sphere parameters and grid size." << std::endl;
        return;
    }
    printf("Generated %d particles, but the simulator is set to handle %d. Using the smaller of the two.\n", generatedCount, this->numParticles);
    if (generatedCount > this->numParticles)
    {
        // Shuffle so truncation doesn't cut a spatial slice off the sphere
        std::vector<int> idx(generatedCount);
        std::iota(idx.begin(), idx.end(), 0);
        std::mt19937 rng(42);
        std::shuffle(idx.begin(), idx.end(), rng);
        std::vector<float3> tmp_pos(this->numParticles), tmp_vel(this->numParticles);
        for (int i = 0; i < this->numParticles; ++i) {
            tmp_pos[i] = h_pos[idx[i]];
            tmp_vel[i] = h_vel[idx[i]];
        }
        h_pos = std::move(tmp_pos);
        h_vel = std::move(tmp_vel);
        generatedCount = this->numParticles;
    }
    else
        this->numParticles = generatedCount; // Aggiorna il numero reale di particelle attive

    cudaMemcpy(deviceData.pos, h_pos.data(), generatedCount * sizeof(float3), cudaMemcpyHostToDevice);
    cudaMemcpy(deviceData.vel, h_vel.data(), generatedCount * sizeof(float3), cudaMemcpyHostToDevice);
    cudaMemcpy(deviceData.old_vel, h_vel.data(), generatedCount * sizeof(float3), cudaMemcpyHostToDevice);
}
__global__ void initBoundariesKernel(int *cellType, int nx, int ny, int nz)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int j = blockIdx.y * blockDim.y + threadIdx.y;
    int k = blockIdx.z * blockDim.z + threadIdx.z;

    if (i >= nx || j >= ny || k >= nz)
        return;

    int idx = i + nx * (j + ny * k);

    if (i == 0 || i == nx - 1 ||
        j == 0 || j == ny - 1 ||
        k == 0 || k == nz - 1)
    {
        cellType[idx] = (int)cellType::SOLID;
    }
}
void cudaParticleSimulator::computeBoundary()
{
    dim3 block(8, 8, 8);
    dim3 grid(
        (grid_size.x + block.x - 1) / block.x,
        (grid_size.y + block.y - 1) / block.y,
        (grid_size.z + block.z - 1) / block.z);
    initBoundariesKernel<<<grid, block>>>(
        grid_data.cellType,
        grid_size.x, grid_size.y, grid_size.z);
    cudaDeviceSynchronize();
}