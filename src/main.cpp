#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <optional>
#include <set>

#include <cstdint>   // Necessary for uint32_t
#include <limits>    // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp

#include <fstream>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp> //for matrices
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <chrono>
#include <cstdlib>    // system()
#include <sys/stat.h> // mkdir

// project modules
#include "utils.h"
#include "initVulkan.h"
#include "buffer.h"
#include "pipeline.h"
#include "device.h"
#include "swapChain.h"
#include "offscreen.h"
#include <cstring>

// basciu
#include "simulation/ChronoCuda/ChronoCuda.h"
#include "simulation/cudaParticleSimulator/cudaParticleSimulator.h"
#include "simulation/cudaCommons.h"
appState state;
cudaParticleSimulator cPS;
std::vector<VkSemaphore> imageAvailableSemaphores;
std::vector<VkSemaphore> renderFinishedSemaphores;
std::vector<VkFence> inFlightFences;

void createSyncObjects()
{
    imageAvailableSemaphores.resize(state.MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(state.MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(state.MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // crea una fence gia "segnalata" per superare il wait alla prima iterazione

    for (size_t i = 0; i < state.MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(state.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(state.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(state.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
        {

            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

#define NUM_PARTICLES 10000
void initCuda()
{
    float deltaTime = 0.016f; // Assuming 60 FPS
    int deviceId = 0;         // Use the first CUDA device
    fluidProperties fluidProps{1000.0f, 1.0f, 0.1f};
    int num_iterations = 10;                                // Number of iterations for the simulation
    gridSize grid_size{100, 100, 100, 0.03f};                  // Example grid size
    state.gridWorldSize = grid_size.x * grid_size.cellSize; // Update global grid world size
    cPS = cudaParticleSimulator(NUM_PARTICLES, deltaTime, deviceId, fluidProps, num_iterations, grid_size);
    cPS.initParticles();
}

void initVulkan()
{
    printf("ciao\n");
    createInstance(state);
    createSurface(state);
    pickPhysicalDevice(state);
    createLogicalDevice(state);

    createSwapChain(state);
    createImageViews(state);

    createRenderPass(state);
    createDescriptorSetLayout(state);

    // createGraphicsPipeline(state);
    createInstancingPipeline(state);

    // Off-screen resources must exist before createFramebuffers so the
    // framebuffers can reference the off-screen image views.
    if (USE_OFF_SCREEN_RENDERING)
    {
        createOffscreenResources(state);
    }

    createFramebuffers(state);
    createCommandPool(state);
    createVertexBuffer(vertices, state);
    // questo da moficare
    std::vector<glm::vec3> cudaInitPos(NUM_PARTICLES, glm::vec3(0.0f));
    createInstanceBuffer(cudaInitPos, state);
    // init particle(state.InstanceBuffer)

    createIndexBuffer(indices, state);

    createUniformBuffers(state);
    createDescriptorPool(state);
    createDescriptorSets(state);

    createCommandBuffer(state);

    createSyncObjects();
}

void drawFrame()
{

    if (USE_OFF_SCREEN_RENDERING)
    {
        // -----------------------------------------------------------------
        // Off-screen path: no swapchain acquire/present.
        // We just submit the command buffer, wait, then save the frame.
        // -----------------------------------------------------------------
        vkWaitForFences(state.device, 1, &inFlightFences[state.currentFrame], VK_TRUE, UINT64_MAX);

        cPS.updateSystem();
        updateUniformBuffer(state.currentFrame, state);
        cPS.cuda2vulkan(state.currentFrame, state);
        cudaDeviceSynchronize(); // ← assicura che CUDA abbia finito

        vkResetFences(state.device, 1, &inFlightFences[state.currentFrame]); // ← spostata qui

        vkResetCommandBuffer(state.commandBuffers[state.currentFrame], 0);
        recordCommandBuffer(state.commandBuffers[state.currentFrame], 0, state);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &state.commandBuffers[state.currentFrame];

        if (vkQueueSubmit(state.graphicsQueue, 1, &submitInfo, inFlightFences[state.currentFrame]) != VK_SUCCESS)
            throw std::runtime_error("failed to submit off-screen command buffer!");

        // Wait for the frame to finish before reading back pixels.
        vkWaitForFences(state.device, 1, &inFlightFences[state.currentFrame], VK_TRUE, UINT64_MAX);

        writeFrameToFfmpeg(state.ffmpegPipe, state);
        state.offscreenFrameIndex++;

        state.currentFrame = (state.currentFrame + 1) % state.MAX_FRAMES_IN_FLIGHT;
        return;
    }

    // -----------------------------------------------------------------
    // Windowed path: original code unchanged.
    // -----------------------------------------------------------------

    //------------------------
    // waits until the gpu has finished
    vkWaitForFences(state.device, 1, &inFlightFences[state.currentFrame], VK_TRUE, UINT64_MAX);
    uint32_t imageIndex;
    // gets the image from swapchain
    VkResult result = vkAcquireNextImageKHR(state.device, state.swapChain, UINT64_MAX, imageAvailableSemaphores[state.currentFrame], VK_NULL_HANDLE, &imageIndex);

    // checks if the swapchain needs to be recreate
    // TODO check if can be optimized by producig result as soon as possible
    if (result == VK_ERROR_OUT_OF_DATE_KHR ||
        result == VK_SUBOPTIMAL_KHR ||
        state.framebufferResized)
    {
        state.framebufferResized = false;
        recreateSwapChain(state);
        return;
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    cPS.updateSystem();

    updateUniformBuffer(state.currentFrame, state);
    // updateInstanceBuffer(state.currentFrame, state);
    cPS.cuda2vulkan(state.currentFrame, state);
    // reset fences only if submit work
    vkResetFences(state.device, 1, &inFlightFences[state.currentFrame]);

    vkResetCommandBuffer(state.commandBuffers[state.currentFrame], 0);
    recordCommandBuffer(state.commandBuffers[state.currentFrame], imageIndex, state);

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[state.currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &state.commandBuffers[state.currentFrame];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[state.currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(state.graphicsQueue, 1, &submitInfo, inFlightFences[state.currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkSwapchainKHR swapChains[] = {state.swapChain};

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr; // Optional

    // present the image to window via sys call
    result = vkQueuePresentKHR(state.presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        recreateSwapChain(state);
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    state.currentFrame = (state.currentFrame + 1) % state.MAX_FRAMES_IN_FLIGHT;
}

void mainLoop()
{
    if (USE_OFF_SCREEN_RENDERING)
    {
        // Open ffmpeg pipe: reads raw BGRA frames from stdin and encodes to MP4.
        char cmd[512];
        snprintf(cmd, sizeof(cmd),
                 "ffmpeg -y -f rawvideo -pixel_format bgra "
                 "-video_size %ux%u -framerate %u -i pipe:0 "
                 "-c:v libx264 -pix_fmt yuv420p output.mp4",
                 state.swapChainExtent.width, state.swapChainExtent.height, state.videoFPS);
        printf("[offscreen] opening pipe: %s\n", cmd);
        state.ffmpegPipe = popen(cmd, "w");
        if (!state.ffmpegPipe)
            throw std::runtime_error("failed to open ffmpeg pipe!");

        uint32_t totalFrames = static_cast<uint32_t>(state.maxDuration * state.videoFPS);
        printf("max duration = %f, fps = %u, total frames = %u\n", state.maxDuration, state.videoFPS, totalFrames);
        while (state.offscreenFrameIndex < totalFrames)
        {
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(state.device);
        int ret = pclose(state.ffmpegPipe);
        state.ffmpegPipe = nullptr;
        if (ret != 0)
            fprintf(stderr, "[offscreen] ffmpeg exited with code %d — "
                            "make sure ffmpeg is installed.\n",
                    ret);
        else
            printf("[offscreen] video saved to output.mp4\n");
    }
    else
    {
        auto startTime = std::chrono::high_resolution_clock::now();
        float duration = 15.0f;
        float counter = 0.0f;
        while (!glfwWindowShouldClose(state.window))
        {
            // float elapsed = std::chrono::duration<float>(
            //     std::chrono::high_resolution_clock::now() - startTime).count();
            // //printf("%f\n", elapsed);
            // if (elapsed > state.maxDuration)
            //     glfwSetWindowShouldClose(state.window, true);
            counter += cPS.getDeltaTime();
            if (counter >= state.maxDuration)
            {
                glfwSetWindowShouldClose(state.window, true);
            }
            glfwPollEvents();
            drawFrame();
        }
        vkDeviceWaitIdle(state.device);
    }
}

void cleanup()
{
    cleanupSwapChain(state);

    if (USE_OFF_SCREEN_RENDERING)
    {
        cleanupOffscreenResources(state);
    }

    for (size_t i = 0; i < state.MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyBuffer(state.device, state.uniformBuffers[i], nullptr);
        vkFreeMemory(state.device, state.uniformBuffersMemory[i], nullptr);
    }
    vkDestroyDescriptorSetLayout(state.device, state.descriptorSetLayout, nullptr);

    vkDestroyDescriptorPool(state.device, state.descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(state.device, state.descriptorSetLayout, nullptr);

    vkDestroyBuffer(state.device, state.indexBuffer, nullptr);
    vkFreeMemory(state.device, state.indexBufferMemory, nullptr);

    vkDestroyBuffer(state.device, state.vertexBuffer, nullptr);
    vkFreeMemory(state.device, state.vertexBufferMemory, nullptr);

    for (size_t i = 0; i < state.MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(state.device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(state.device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(state.device, inFlightFences[i], nullptr);

        // instance buffer cleanup
        vkDestroyBuffer(state.device, state.instanceBuffers[i], nullptr);
        vkFreeMemory(state.device, state.instanceBuffersMemory[i], nullptr);
    }

    // Command resources
    vkDestroyCommandPool(state.device, state.commandPool, nullptr);
    // no cleanup of the command buffer bc destroyed when pool destroyed

    // Framebuffers
    // for (auto framebuffer : swapChainFramebuffers) {
    //     vkDestroyFramebuffer(device, framebuffer, nullptr);
    // }

    // Pipeline
    vkDestroyPipeline(state.device, state.graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(state.device, state.pipelineLayout, nullptr);
    vkDestroyRenderPass(state.device, state.renderPass, nullptr);

    // Image views
    // for (auto imageView : swapChainImageViews) {
    //     vkDestroyImageView(device, imageView, nullptr);
    // }

    // Swapchain
    // vkDestroySwapchainKHR(device, swapChain, nullptr);
    // no cleanup for swapChainImages becouse done in vkDestroySwapchainKHR

    // Device
    vkDestroyDevice(state.device, nullptr);
    // no cleanup for VkQueue, automatically destroyed after destroy of device

    if (!USE_OFF_SCREEN_RENDERING)
    {
        vkDestroySurfaceKHR(state.instance, state.surface, nullptr);
    }

    // Instance (ULTIMO)
    vkDestroyInstance(state.instance, nullptr);

    if (!USE_OFF_SCREEN_RENDERING)
    {
        glfwDestroyWindow(state.window);
        glfwTerminate();
    }
}

int main()
{
    try
    {
        buildSphereMesh(state);
        initWindow(state);
        initCuda();
        initVulkan();
        mainLoop();
        cleanup();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}