#ifndef CUDA_COMMONS_H
#define CUDA_COMMONS_H

#include <cuda_runtime.h>
#include <vector_types.h>
#include <driver_types.h>
#include <vulkan/vulkan.h>
#include <iostream>

struct fluidProperties
{
    int numParticles;
    float density;
    float viscosity;
    float timeStep;
};

struct particleDeviceData
{
    float3 *pos;
    float3 *vel;
    float3 *acc;
};

struct gridSize{
    int x,y,z;
    int gridCells;
};

struct vulkanData{
    VkBuffer posBuffer;
    VkBuffer velBuffer;
    VkBuffer accBuffer;
    VkDeviceMemory posBufferMemory;
    VkDeviceMemory velBufferMemory;
    VkDeviceMemory accBufferMemory;
};


#endif