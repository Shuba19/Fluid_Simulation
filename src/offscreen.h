#pragma once

#include <vulkan/vulkan.h>
#include <cstdio>
#include "utils.h"

void createOffscreenResources(appState& state);
void cleanupOffscreenResources(appState& state);

// Copies the current off-screen image to the readback buffer and writes raw BGRA
// pixels into the ffmpeg pipe.  Call after the render fence has been waited on.
void writeFrameToFfmpeg(FILE* pipe, appState& state);
