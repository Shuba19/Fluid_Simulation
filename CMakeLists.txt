cmake_minimum_required(VERSION 3.15)

# ALWAYS set overrides before the project() call
set(CMAKE_CXX_COMPILER /usr/bin/g++-13)

project(FluidSim LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_ARCHITECTURES "86;89")

add_executable(FluidSymulation
    src/main.cpp
    src/initVulkan.cpp
    src/buffer.cpp
    src/pipeline.cpp
    src/device.cpp
    src/swapChain.cpp
    src/utils.cpp
    src/offscreen.cpp
    src/simulation/ChronoCuda/ChronoCuda.cu
    src/simulation/cudaParticleSimulator/cudaParticleSimulator.cu
    src/simulation/cudaParticleSimulator/Handlers/extForces.cu
    src/simulation/cudaParticleSimulator/Handlers/pressure_solver.cu
    src/simulation/cudaParticleSimulator/Handlers/transfer.cu
    src/simulation/cudaParticleSimulator/Handlers/updateSystem.cu
)

set_target_properties(FluidSymulation PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)

# Dependencies
find_package(Vulkan REQUIRED)
target_link_libraries(FluidSymulation PRIVATE Vulkan::Vulkan)

find_package(CUDAToolkit REQUIRED)
target_link_libraries(FluidSymulation PRIVATE CUDA::cudart)

include(FetchContent)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8 # Pinning a tag prevents unexpected remote updates/loops
)
FetchContent_MakeAvailable(glfw)
target_link_libraries(FluidSymulation PRIVATE glfw)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)
target_link_libraries(FluidSymulation PRIVATE glm)