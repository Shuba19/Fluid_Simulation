#pragma once

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

#include "utils.h"

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    //la bind configura la gpu in modo da ottimizzare il passaggio dei dati
    //alla gpu, è come se configurasse uno stato della gpu dicendo qualcosa tipo
    //prendi i dati da qua
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};

        bindingDescription.binding = 0; //start Index in the binding array
        bindingDescription.stride = sizeof(Vertex); 
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; //change if we want to pass to instance rendering

        return bindingDescription;
    }

    //specifica come gestire ottenrere le informazioni dal blocco di vertici della bind
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        //data pos
        attributeDescriptions[0].binding = 0; //which binding to take
        attributeDescriptions[0].location = 0; //reference alla direttiva del vertex shader 
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; //deve matchare il numero di componenti in di shader 
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        //data color
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, appState & state);
void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, appState & state);
void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, appState & state);
void createIndexBuffer(std::vector<uint16_t> indices, appState & state);
void createVertexBuffer(std::vector<Vertex> vertices, appState & state);