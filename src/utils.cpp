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
bool USE_OFF_SCREEN_RENDERING = true;
#pragma endregion Config

std::vector<glm::vec3> particleInitialPositions = {
    {0.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 2.0f},
    {1.0f, 1.0f, 3.0f},
};

std::vector<glm::vec3> particleBasePositions = {
    {0.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 2.0f},
    {1.0f, 1.0f, 3.0f},
};

// Generates a UV-sphere centered at origin with the given radius.
// Colors encode the surface normal mapped to [0,1] for basic shading.
static std::vector<Vertex> makeSphereVertices(float r, int stacks, int sectors) {
    std::vector<Vertex> verts;
    for (int i = 0; i <= stacks; i++) {
        float theta = (float)i / stacks * M_PI;        // 0 .. PI
        for (int j = 0; j <= sectors; j++) {
            float phi = (float)j / sectors * 2.0f * M_PI; // 0 .. 2PI
            glm::vec3 n = {
                sinf(theta) * cosf(phi),
                sinf(theta) * sinf(phi),
                cosf(theta)
            };
            Vertex v;
            v.pos   = n * r;
            v.color = (n + glm::vec3(1.0f)) * 0.5f; // normal → [0,1] color
            verts.push_back(v);
        }
    }
    return verts;
}

static std::vector<uint16_t> makeSphereIndices(int stacks, int sectors) {
    std::vector<uint16_t> idx;
    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < sectors; j++) {
            uint16_t v0 = (uint16_t)( i      * (sectors + 1) + j);
            uint16_t v1 = (uint16_t)( i      * (sectors + 1) + j + 1);
            uint16_t v2 = (uint16_t)((i + 1) * (sectors + 1) + j);
            uint16_t v3 = (uint16_t)((i + 1) * (sectors + 1) + j + 1);
            idx.insert(idx.end(), {v0, v2, v1, v1, v2, v3});
        }
    }
    return idx;
}

std::vector<Vertex>   vertices;
std::vector<uint16_t> indices;

void buildSphereMesh(appState& state) {
    vertices = makeSphereVertices(state.sphereRadius, state.sphereStacks, state.sphereSectors);
    indices  = makeSphereIndices(state.sphereStacks, state.sphereSectors);
}

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

