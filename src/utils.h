#ifndef UTILS_H
#define UTILS_H

struct appState{
    bool framebufferResized = false;
    float maxDuration = 5.0f;
    
    const int WIDTH = 800;
    const int HEIGHT = 600;

    GLFWwindow* window;
    VkInstance instance;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkCommandPool commandPool; //insieme di command buffer diversi
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

#endif