#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>
#include <iostream>

const int WIDTH = 800;
const int HEIGHT = 600;

GLFWwindow* window;

void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Vulkan
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Triangle", nullptr, nullptr);
}

void initVulkan() {
    
    
    // Vulkan app Instance
    //struttura dati utile a vulkan per avere info sull'applicazione
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    //appInfo.pApplicationName = "Triangle App";  
    //appInfo.apiVersion = VK_API_VERSION_1_0;

    //Vulkan instance
    //effettiva instanza di un contesto di vulkan, utile per gestire i rendering 
    VkInstanceCreateInfo InstanceVK{};
    InstanceVK.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    InstanceVK.pApplicationInfo = &appInfo;

    // Extensions required by GLFW
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    InstanceVK.enabledExtensionCount = glfwExtensionCount;
    InstanceVK.ppEnabledExtensionNames = glfwExtensions;

    if (vkCreateInstance(&InstanceVK, nullptr, new VkInstance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        // Qui normalmente renderizzeresti il triangolo
        // Per semplicità al momento non facciamo il rendering completo
    }
}

void cleanup() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

int main() {
    try {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}