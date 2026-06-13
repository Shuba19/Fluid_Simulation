#include "offscreen.h"
#include "buffer.h"   // findMemoryType, createBuffer

#include <stdexcept>
#include <cstring>
#include <cstdio>

// ---------------------------------------------------------------------------
static void createImage(uint32_t width, uint32_t height,
                        VkFormat format,
                        VkImageTiling tiling,
                        VkImageUsageFlags usage,
                        VkMemoryPropertyFlags properties,
                        VkImage& image,
                        VkDeviceMemory& imageMemory,
                        appState& state)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = format;
    imageInfo.tiling        = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = usage;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(state.device, &imageInfo, nullptr, &image) != VK_SUCCESS)
        throw std::runtime_error("failed to create off-screen image!");

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(state.device, image, &memReq);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, properties, state);

    if (vkAllocateMemory(state.device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
        throw std::runtime_error("failed to allocate off-screen image memory!");

    vkBindImageMemory(state.device, image, imageMemory, 0);
}

// ---------------------------------------------------------------------------
void createOffscreenResources(appState& state)
{
    uint32_t w = state.swapChainExtent.width;
    uint32_t h = state.swapChainExtent.height;

    VkFormat fmt = state.swapChainImageFormat;

    state.offscreenImages.resize(state.MAX_FRAMES_IN_FLIGHT);
    state.offscreenImageMemories.resize(state.MAX_FRAMES_IN_FLIGHT);
    state.offscreenImageViews.resize(state.MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < state.MAX_FRAMES_IN_FLIGHT; i++) {
        createImage(
            w, h, fmt,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            state.offscreenImages[i],
            state.offscreenImageMemories[i],
            state
        );

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = state.offscreenImages[i];
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = fmt;
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(state.device, &viewInfo, nullptr, &state.offscreenImageViews[i]) != VK_SUCCESS)
            throw std::runtime_error("failed to create off-screen image view!");
    }

    VkDeviceSize bufSize = (VkDeviceSize)w * h * 4;
    createBuffer(
        bufSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        state.readbackBuffer,
        state.readbackBufferMemory,
        state
    );
}

// ---------------------------------------------------------------------------
void cleanupOffscreenResources(appState& state)
{
    vkDestroyBuffer(state.device, state.readbackBuffer, nullptr);
    vkFreeMemory(state.device, state.readbackBufferMemory, nullptr);

    for (int i = 0; i < state.MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyImageView(state.device, state.offscreenImageViews[i], nullptr);
        vkDestroyImage(state.device, state.offscreenImages[i], nullptr);
        vkFreeMemory(state.device, state.offscreenImageMemories[i], nullptr);
    }
}

// ---------------------------------------------------------------------------
// Copies the rendered image to the host readback buffer, then writes raw BGRA
// pixels straight into the ffmpeg pipe — no PNG encoding, no temp files.
// ---------------------------------------------------------------------------
void writeFrameToFfmpeg(FILE* pipe, appState& state)
{
    uint32_t w = state.swapChainExtent.width;
    uint32_t h = state.swapChainExtent.height;
    VkDeviceSize bufSize = (VkDeviceSize)w * h * 4;

    // --- single-use command buffer for image → buffer copy ---
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool        = state.commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(state.device, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // Transition: COLOR_ATTACHMENT_OPTIMAL → TRANSFER_SRC_OPTIMAL
    VkImageMemoryBarrier barrier{};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = state.offscreenImages[state.currentFrame];
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.srcAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask                   = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Copy image → host buffer
    VkBufferImageCopy region{};
    region.bufferOffset      = 0;
    region.bufferRowLength   = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {w, h, 1};

    vkCmdCopyImageToBuffer(cmd,
        state.offscreenImages[state.currentFrame],
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        state.readbackBuffer,
        1, &region);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &cmd;
    vkQueueSubmit(state.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(state.graphicsQueue);

    vkFreeCommandBuffers(state.device, state.commandPool, 1, &cmd);

    // --- write raw BGRA directly into ffmpeg pipe (no conversion needed) ---
    void* mapped;
    vkMapMemory(state.device, state.readbackBufferMemory, 0, bufSize, 0, &mapped);
    fwrite(mapped, 1, bufSize, pipe);
    vkUnmapMemory(state.device, state.readbackBufferMemory);
}
