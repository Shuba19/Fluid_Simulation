#ifndef CUDA_COMMONS_H
#define CUDA_COMMONS_H

#include <cuda_runtime.h>
#include <vector_types.h>
#include <driver_types.h>
#include <vulkan/vulkan.h>
#include <iostream>

struct fluidProperties
{
    float density;
    float viscosity;
    float timeStep;
    float flipRatio;
};

struct particleDeviceData
{
    float3 *pos;
    float3 *vel, *old_vel; //la old vel serve per il calcolo dell'accelerazione, che è data da (vel - old_vel) / deltaTime
};
struct gridDeviceData {
    //velocità delle 3 facce
    //usando MAC GRID, le velocità sono memorizzate solo sulle facce anzichè al centro, quindi una valocità simmetyrica per coppie di facce
    float *u, *u_old; // Velocità sulle facce x
    float *v, *v_old; // Velocità sulle facce y
    float *w, *w_old; // Velocità sulle facce z

    float *uWeight, *vWeight, *wWeight;  //PESI
    float *divergence; // Divergenza al centro della cella
    float *pBuffer;

    float *p; // Pressione al centro della cella
    int *cellType;
};
struct gridSize{
    int x,y,z;
    int gridCells;
    int cellSize;
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