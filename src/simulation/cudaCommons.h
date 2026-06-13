#ifndef CUDA_COMMONS_H
#define CUDA_COMMONS_H

#include <cuda_runtime.h>
#include <vector_types.h>
#include <driver_types.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <GLFW/glfw3.h>

/*
@params density: density of the fluid
@params viscosity: viscosity of the fluid
@params flipRatio: ratio of FLIP to PIC, between 0 and 1
*/
struct fluidProperties
{
    float density;
    float viscosity;
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
struct gridSize {
    int x, y, z;
    int gridCells;
    float cellSize;  
    gridSize() : x(0), y(0), z(0), cellSize(0.1f), gridCells(0) {} // Default constructor
    gridSize(int x, int y, int z, float cellSize = 0.1f)
        : x(x), y(y), z(z), cellSize(cellSize), gridCells(x * y * z) {}
};

struct vulkanData{
    VkBuffer posBuffer;
};


#endif