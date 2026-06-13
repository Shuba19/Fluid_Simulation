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
#include "device.h"
#include "utils.h"
#include "offscreen.h"



#pragma region swapChain

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, appState& state) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(state.window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {

    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

void createSwapChain(appState& state) {
    if (USE_OFF_SCREEN_RENDERING) {
        state.swapChainExtent = {(uint32_t)state.WIDTH, (uint32_t)state.HEIGHT};
        state.swapChainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;
        return;
    }
    //swapChainSupport struct
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(state);

    //swap chain format, presentMode e extent 
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, state);

    //number of images in the swapChain
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    //create infos
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = state.surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    //queues 
    QueueFamilyIndices indices = findQueueFamilies(state);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform; //specifies that no tranform should be applied at the beginning 
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //alpha (credo della finestra a 1 -> opaco non trasparente)

    createInfo.presentMode = presentMode; 
    createInfo.clipped = VK_TRUE; //clipping in caso di occlusione tra finestre 
    
    //se la swapchain cambia per qualche motivo (es resize della finestra) 
    //va ricreata da capo e questo campo indica la swapChain precedente
    createInfo.oldSwapchain = VK_NULL_HANDLE; 


    if (vkCreateSwapchainKHR(state.device, &createInfo, nullptr, &state.swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(state.device, state.swapChain, &imageCount, nullptr);
    state.swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(state.device, state.swapChain, &imageCount, state.swapChainImages.data());

    state.swapChainImageFormat = surfaceFormat.format;
    state.swapChainExtent = extent;

}

void cleanupSwapChain(appState& state) {
    for (auto framebuffer : state.swapChainFramebuffers) {
        vkDestroyFramebuffer(state.device, framebuffer, nullptr);
    }

    // In off-screen mode the framebuffers reference our off-screen image views,
    // not the swapchain views, so skip the swapchain-specific teardown.
    if (USE_OFF_SCREEN_RENDERING) return;

    for (auto imageView : state.swapChainImageViews) {
        vkDestroyImageView(state.device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(state.device, state.swapChain, nullptr);
}

void createImageViews(appState& state)
{
    if (USE_OFF_SCREEN_RENDERING) return;
    //una per ongi imagine della swapchain
    state.swapChainImageViews.resize(state.swapChainImages.size());

    for (size_t i = 0; i < state.swapChainImages.size(); i++) {

        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = state.swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = state.swapChainImageFormat;

        //allows any transformation to channels if neaded 
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(state.device, &createInfo, nullptr, &state.swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

void createFramebuffers(appState& state) 
{
    //state.swapChainFramebuffers.resize(state.swapChainImageViews.size());
        // Off-screen: one framebuffer per frame-in-flight backed by our off-screen images.
    // Windowed:   one framebuffer per swapchain image as usual.
    size_t count = USE_OFF_SCREEN_RENDERING
                       ? (size_t)state.MAX_FRAMES_IN_FLIGHT
                       : state.swapChainImageViews.size();
    state.swapChainFramebuffers.resize(count);

    //for (size_t i = 0; i < state.swapChainImageViews.size(); i++) 
    for (size_t i = 0; i < count; i++)
    {
        VkImageView attachments[] = {
            USE_OFF_SCREEN_RENDERING ? state.offscreenImageViews[i]
                                     : state.swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = state.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = state.swapChainExtent.width;
        framebufferInfo.height = state.swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(state.device, &framebufferInfo, nullptr, &state.swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void recreateSwapChain(appState& state) {
    vkDeviceWaitIdle(state.device);

    cleanupSwapChain(state);

    createSwapChain(state);
    createImageViews(state);
    createFramebuffers(state);
}


#pragma endregion swapChain
