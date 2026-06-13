#ifndef CUDA_PARTICLE_SIMULATOR_H
#define CUDA_PARTICLE_SIMULATOR_H
#include "../cudaCommons.h"
#include "../../utils.h"

enum class cellType
{
    SOLID = 0,
    FLUID = 1,
    AIR = 2
};

class cudaParticleSimulator
{
private:
    fluidProperties fluidProps;
    particleDeviceData deviceData;
    gridSize grid_size;
    gridDeviceData grid_data;
    int numParticles;
    int numIterations;
    int deviceId;
    float deltaTime;
    int num_iterations;
    int v_bufferPos;
    int v_buggerLen;
    gridSize grid;

public:
    cudaParticleSimulator() = default;
    cudaParticleSimulator(int numParticles, float deltaTime, int deviceId, fluidProperties fluidProps, int num_iterations, gridSize grid_size = {10, 10, 10});
    ~cudaParticleSimulator();
    void initParticles();
    void computeBoundary();
    void updateSystem();
    void p2g();
    void saveGridVelocities();
    void applyExternalForces();
    void computeDivergence();
    void pressureSolve();
    void applyPressureGradient();
    void g2p();
    void computeAdvection();
    void cuda2vulkan(uint32_t currentImage, appState& state);
    float getDeltaTime() const { return deltaTime; }
};

#endif