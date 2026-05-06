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

#include "initVulkan.h"
#include "utils.h"

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto state = reinterpret_cast<appState*>(
        glfwGetWindowUserPointer(window)
    );

    if (state) {
        state->framebufferResized = true;
    }
}

void initWindow(appState & state) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //tells GLFW we're using Vulkan
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    //window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Triangle", nullptr, nullptr);
    state.window = glfwCreateWindow(state.WIDTH, state.HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetFramebufferSizeCallback(state.window, framebufferResizeCallback);
    glfwSetWindowUserPointer(state.window, &state);
}

void createInstance(appState & state) {
    //creates an App info struct
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    //appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    //appInfo.apiVersion = VK_API_VERSION_1_0;

    //creates an crateInfo for global information 
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    //extensions 
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.enabledLayerCount = 0;

    if (vkCreateInstance(&createInfo, nullptr, &state.instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

void createSurface(appState & state)
{
    if (glfwCreateWindowSurface(state.instance, state.window, nullptr, &state.surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

