#ifndef CUDA_PARTICLE_SIMULATOR_H
#define CUDA_PARTICLE_SIMULATOR_H
#include "../cudaCommons.h"


class cudaParticleSimulator
{
    private:
        fluidProperties fluidProps;
        vulkanData vkData;
        particleDeviceData deviceData;
        gridSize grid_size;
        int numParticles;
        int deviceId;
        float deltaTime;
        int num_iterations;
        gridSize grid;

    public:
        cudaParticleSimulator(int numParticles, float deltaTime, vulkanData h_vkData, int deviceId = 0, fluidProperties fluidProps, int num_iterations = 1, gridSize grid_size = {10,10,10});
        ~cudaParticleSimulator();
        void updateSystem();
        void calculateGridParticles();
        void calculateGridAdvection();
        void collisionHandling();
        void updateParticlePositions();
};


#endif