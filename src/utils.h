#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <array>
#include <optional>
#include <cstdio>

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#pragma region Config

// static bool OBJ_INSTANCING = false;
extern bool OBJ_INSTANCING;

extern bool USE_OFF_SCREEN_RENDERING;



#pragma endregion Config



#pragma region structs
struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getVertexBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding   = 0;
        bindingDescription.stride    = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static VkVertexInputBindingDescription getInstancingBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding   = 1;
        bindingDescription.stride    = sizeof(glm::vec3);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding  = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset   = offsetof(Vertex, pos);

        attributeDescriptions[1].binding  = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset   = offsetof(Vertex, color);

        return attributeDescriptions;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getInstanceAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding  = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset   = offsetof(Vertex, pos);

        attributeDescriptions[1].binding  = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset   = offsetof(Vertex, color);

        attributeDescriptions[2].binding  = 1;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset   = 0;

        return attributeDescriptions;
    }
};

struct ParticleInstance {
    glm::vec3 position;
};

// extern const std::vector<Vertex> vertices = {
//     {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // 0
//     {{0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}}, // 1
//     {{0.5f, 0.5f},   {0.0f, 0.0f, 1.0f}}, // 2
//     {{-0.5f, 0.5f},  {1.0f, 1.0f, 1.0f}}  // 3
// };

// extern const std::vector<uint16_t> indices = {
//     0, 1, 2,
//     2, 3, 0
// };

// extern std::vector<glm::vec3> particleInitialPositions = {
//     {0.0f, 0.0f, 0.0f},
//     {1.0f, 0.0f, 1.0f},
//     {0.0f, 1.0f, 2.0f},
//     {1.0f, 1.0f, 3.0f},

//     // {0.0f, 0.1f, 0.0f},
//     // {0.1f, 0.1f, 0.0f},
//     // {0.2f, 0.1f, 0.0f},
//     // {0.3f, 0.1f, 0.0f},

//     // {0.0f, 0.2f, 0.0f},
//     // {0.1f, 0.2f, 0.0f},
//     // {0.2f, 0.2f, 0.0f},
//     // {0.3f, 0.2f, 0.0f},
// };

extern std::vector<Vertex>   vertices;
extern std::vector<uint16_t> indices;
extern std::vector<glm::vec3> particleInitialPositions;
extern std::vector<glm::vec3> particleBasePositions;


struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};


struct appState{

    // ---- sphere mesh resolution ----
    int sphereStacks   = 16;
    int sphereSectors  = 32;
    float sphereRadius = 0.05f;

    // ---- off-screen video recording ----
    float maxDuration = 5.0f;

    // One VkImage per frame-in-flight used as color attachment instead of a swapchain image.
    std::vector<VkImage>        offscreenImages;
    std::vector<VkDeviceMemory> offscreenImageMemories;
    std::vector<VkImageView>    offscreenImageViews;

    // Host-visible staging buffer for pixel readback.
    VkBuffer       readbackBuffer       = VK_NULL_HANDLE;
    VkDeviceMemory readbackBufferMemory = VK_NULL_HANDLE;

    // Sequential counter incremented every saved frame.
    uint32_t offscreenFrameIndex = 0;

    // Pipe to a running ffmpeg process (opened in mainLoop, closed after the loop).
    FILE* ffmpegPipe = nullptr;

    // Target frames-per-second for the output video.
    uint32_t videoFPS = 60;

    
    //app info
    bool framebufferResized = false;
    
    const int WIDTH = 800;
    const int HEIGHT = 600;

    const int MAX_FRAMES_IN_FLIGHT = 1;
    // const int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t currentFrame = 0;

    //vulkan basics
    GLFWwindow* window;
    VkInstance instance;
    VkSurfaceKHR surface;

    //device 
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    //buffers
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    std::vector<VkCommandBuffer> commandBuffers;

    //geometry instaincing 
    std::vector<VkBuffer> instanceBuffers;
    std::vector<VkDeviceMemory> instanceBuffersMemory;
    std::vector<void*> instanceBuffersMapped;

    uint32_t particleCount;

    VkCommandPool commandPool; //insieme di command buffer diversi

    //pipeline
    VkPipeline graphicsPipeline;
    VkPipeline fluidPipeline;
    VkPipeline particlePipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    VkRenderPass renderPass;


    //swapchain
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkExtent2D swapChainExtent;
    VkFormat swapChainImageFormat;
    std::vector<VkImageView> swapChainImageViews;

    //uniform buffers
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;


};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    // ogni device ha un array di famiglie di code ogniuna che supporta operazioni diverse,
    // a sua volta ogni famiglia di code ha piu code.
    // quindi una volta restituito l'array con tutte le famiglie si sceglie l'indice della famiglia
    // che ci serve e da li si prende una coda per fare quello che ci serve
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
#pragma endregion structs


#pragma region functions

std::vector<char> readFile(const std::string& filename);

// Populates vertices and indices with a UV-sphere using state.sphere* params.
// Must be called before initVulkan().
void buildSphereMesh(appState& state);

#pragma endregion functions

#endif