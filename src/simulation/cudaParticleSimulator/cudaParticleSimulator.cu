#include "cudaParticleSimulator.h"

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

cudaParticleSimulator::cudaParticleSimulator(int numParticles, float deltaTime, vulkanData h_vkData, int deviceId = 0, fluidProperties fluidProps, int num_iterations, gridSize grid_size)
{
    this->numParticles = numParticles;
    this->deltaTime = deltaTime;
    this->vkData = h_vkData;
    this->deviceId = deviceId;
    this->grid = grid_size;
    this->fluidProps = fluidProps;
    this->num_iterations = num_iterations;
    //inizializza tutti i parametri
    size_t size = sizeof(float3) * numParticles;
    this->deviceData.acc = nullptr;
    this->deviceData.pos = nullptr;
    this->deviceData.vel = nullptr;

    CHECK(cudaSetDevice(this->deviceId));
    if (numParticles <= 0)
        std::cerr << "Error: Number of particles must be greater than 0." << std::endl;
    CHECK(cudaMalloc(&this->deviceData.acc, size));
    CHECK(cudaMalloc(&this->deviceData.pos, size));
    CHECK(cudaMalloc(&this->deviceData.vel, size));
    CHECK(cudaMemset(this->deviceData.vel, 0, size));
    CHECK(cudaMemset(this->deviceData.acc, 0, size));
    CHECK(cudaMemset(this->deviceData.pos, 0, size));
    CHECK(cudaGetLastError());
}

cudaParticleSimulator::~cudaParticleSimulator()
{
    CHECK(cudaFree(this->deviceData.acc));
    CHECK(cudaFree(this->deviceData.pos));
    CHECK(cudaFree(this->deviceData.vel));
}