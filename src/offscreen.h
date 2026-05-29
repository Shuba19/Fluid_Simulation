#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include "utils.h"

// Creates the off-screen VkImage that acts as color attachment (replaces swapchain images),
// and the host-visible readback buffer used to pull pixels to CPU each frame.
void createOffscreenResources(appState& state);

// Destroys the off-screen image, its view, its memory, and the readback buffer.
void cleanupOffscreenResources(appState& state);

// Reads pixels from the GPU, converts BGRA→RGBA if needed, and writes a PNG file.
// Call this after vkQueueWaitIdle on the frame that just finished.
void saveFrame(uint32_t frameIndex, appState& state);
