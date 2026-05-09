#ifndef PIPELINE_H
#define PIPELINE_H

#include "utils.h"

static std::vector<char> readFile(const std::string& filename);
VkShaderModule createShaderModule(const std::vector<char>& code, appState& state);
void createGraphicsPipeline(appState& state);
void createRenderPass(appState& state);


#endif
