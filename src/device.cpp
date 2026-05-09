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


#include "device.h"
#include "utils.h"




#pragma region device


QueueFamilyIndices findQueueFamilies(appState & state) {

    //stesso pattern di find devices 
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(state.physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(state.physicalDevice, &queueFamilyCount, queueFamilies.data());//punto data passa il puntatore al primo elemento del vettore da riempire
    
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        //sets the graphics family
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        //sets the present family
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(state.physicalDevice, i, state.surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }        

        i++;
    }
    return indices;
}


SwapChainSupportDetails querySwapChainSupport(appState & state) {
    SwapChainSupportDetails details;

    //capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state.physicalDevice, state.surface, &details.capabilities);

    //formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(state.physicalDevice, state.surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(state.physicalDevice, state.surface, &formatCount, details.formats.data());
    }

    //presentModes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(state.physicalDevice, state.surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(state.physicalDevice, state.surface, &presentModeCount, details.presentModes.data());
    }


    return details;
}

bool checkDeviceExtensionSupport(appState & state) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(state.physicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(state.physicalDevice, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

    return requiredExtensions.empty();
}

bool isDeviceSuitable(appState & state) {

    //verifica che i buffer siano disponibili
    QueueFamilyIndices indices = findQueueFamilies(state);

    //verifica che il device supporti le estensioni 
    bool extensionsSupported = checkDeviceExtensionSupport(state);

    //verifica che il device abbia la swapchain
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(state);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

void pickPhysicalDevice(appState & state)
{

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(state.instance, &deviceCount, nullptr); //first call just to have number of devices
    
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }else{
        printf("device\n");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount); 
    vkEnumeratePhysicalDevices(state.instance, &deviceCount, devices.data()); //actually writing devices to the list

    for (const auto& device : devices) {
        state.physicalDevice = device;

        if (isDeviceSuitable(state)) {
            break;
        }
    }

    if (state.physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
    
}

void createLogicalDevice(appState & state)
{
    QueueFamilyIndices indices = findQueueFamilies(state);
    

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }


    VkPhysicalDeviceFeatures deviceFeatures{};

    // ho rinominato da createInfo a createDeviceInfo
    VkDeviceCreateInfo createDeviceInfo{};
    createDeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    //createDeviceInfo.pQueueCreateInfos = &queueCreateInfo;
    //createDeviceInfo.queueCreateInfoCount = 1;
    createDeviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createDeviceInfo.pQueueCreateInfos = queueCreateInfos.data();
    createDeviceInfo.pEnabledFeatures = &deviceFeatures;
    //createDeviceInfo.enabledExtensionCount = 0;
    createDeviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createDeviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

    //validation steps, commented because skipped
    // if (enableValidationLayers) {
    //     createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    //     createInfo.ppEnabledLayerNames = validationLayers.data();
    // } else {
        createDeviceInfo.enabledLayerCount = 0;
    //}
    if (vkCreateDevice(state.physicalDevice, &createDeviceInfo, nullptr, &state.device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
    vkGetDeviceQueue(state.device, indices.presentFamily.value(), 0, &state.presentQueue);
    vkGetDeviceQueue(state.device, indices.graphicsFamily.value(), 0, &state.graphicsQueue);


}
#pragma endregion device
