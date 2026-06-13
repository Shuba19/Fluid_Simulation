#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <optional>
#include <set>

#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp

#include <fstream>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp> //for matrices
#include <glm/gtc/matrix_transform.hpp>




#include <array>
#include "utils.h"

#include "utils.h"

#pragma region Config
bool OBJ_INSTANCING = true;
//bool USE_OFF_SCREEN_RENDERING = false;
bool USE_OFF_SCREEN_RENDERING = false;
#pragma endregion Config

/*


Particle position: (3.001085, 7.501266, 4.999569)
Particle position: (3.050902, 7.100272, 4.852314)
Particle position: (3.050189, 7.101429, 4.898479)
Particle position: (3.049405, 7.101741, 4.951680)
Segmentation fault (core dumped)
*/


std::vector<glm::vec3> particleInitialPositions = {
    {0.0f, 0.0f, 0.0f},
    {3.050902f, 7.100272f, 4.852314f},
    {0.0f, 1.0f, 2.0f},
    {1.0f, 1.0f, 3.0f},
};

std::vector<glm::vec3> particleBasePositions = {
    {0.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 2.0f},
    {1.0f, 1.0f, 3.0f},
};
const std::vector<Vertex> vertices = {
    {{-0.05f, -0.05f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.05f, -0.05f}, {0.0f, 1.0f, 0.0f}},
    {{ 0.05f,  0.05f}, {0.0f, 0.0f, 1.0f}},
    {{-0.05f,  0.05f}, {1.0f, 1.0f, 1.0f}},
};

const std::vector<uint16_t> indices = {
    0, 1, 2,
    2, 3, 0
};

// std::vector<glm::vec3> particleBasePositions = particleInitialPositions;

std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    std::cout<<"Reading file: " << filename << std::endl;
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

