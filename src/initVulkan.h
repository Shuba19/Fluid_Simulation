#pragma once

struct appState;
void initWindow(appState& state);
void initVulkan();
void createInstance(appState & state);
void createSurface(appState & state);