cmake_minimum_required(VERSION 3.15)# versione minima
project(FluidSim)# nome del progetto usato generare target e file di output.

set(CMAKE_CXX_STANDARD 17) #setta lo standard di cpp

add_executable(FluidSymulation src/main.cpp)# crea un eseguibile chiamato app usando il file sorgente src/main.cpp.

# Vulkan (deve essere installato)
find_package(Vulkan REQUIRED) 
target_link_libraries(FluidSymulation PRIVATE Vulkan::Vulkan)

# Librerie scaricate automaticamente
include(FetchContent)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
)
FetchContent_MakeAvailable(glfw)
target_link_libraries(FluidSymulation PRIVATE glfw)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
)
FetchContent_MakeAvailable(glm)
target_link_libraries(FluidSymulation PRIVATE glm)