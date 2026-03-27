#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <optional>


const int WIDTH = 800;
const int HEIGHT = 600;

GLFWwindow* window;
VkInstance instance;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkDevice device;
VkQueue graphicsQueue;




void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //tells GLFW we're using Vulkan
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Triangle", nullptr, nullptr);
}

void createInstance() {
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

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

//funzione per vedere che tipo di proprieta e informazioni si possono richiedere dalla gpu
// bool isDeviceSuitable(VkPhysicalDevice device) {
//     VkPhysicalDeviceProperties deviceProperties; //struttura per proprietà base del device (es nome, tipo etc)
//     VkPhysicalDeviceFeatures deviceFeatures;  //struttura per feature aggiuntive del device (es texture compression and 64bit float op.)
//     vkGetPhysicalDeviceProperties(device, &deviceProperties);
//     vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

//     //return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;
//     //return deviceFeatures.geometryShader; //il dispositivo supporta geometry shaders?
//     //return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU; // il dispositivo è una gpu esterna?
//     return true; //false entrambe su mac perche la gpu è integrata e vulkan interagisce con metal che non supporta direttamente geometry shaders
// }

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    // ogni device ha un array di famiglie di code ogniuna che supporta operazioni diverse,
    // a sua volta ogni famiglia di code ha piu code.
    // quindi una volta restituito l'array con tutte le famiglie si sceglie l'indice della famiglia
    // che ci serve e da li si prende una coda per fare quello che ci serve

    bool isComplete() {
        return graphicsFamily.has_value();
    }
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    //stesso pattern di find devices 
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());//punto data passa il puntatore al primo elemento del vettore da riempire
    
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }        

        i++;
    }
    return indices;
}

bool isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    return indices.isComplete();
}

void pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr); //first call just to have number of devices
    
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }else{
        printf("device\n");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount); 
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()); //actually writing devices to the list

    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
    
}

void createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createDeviceInfo{};
    createDeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createDeviceInfo.pQueueCreateInfos = &queueCreateInfo;
    createDeviceInfo.queueCreateInfoCount = 1;
    createDeviceInfo.pEnabledFeatures = &deviceFeatures;
    createDeviceInfo.enabledExtensionCount = 0;

    //validation steps, commented because skipped
    // if (enableValidationLayers) {
    //     createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    //     createInfo.ppEnabledLayerNames = validationLayers.data();
    // } else {
        createDeviceInfo.enabledLayerCount = 0;
    //}
    if (vkCreateDevice(physicalDevice, &createDeviceInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);

}

void initVulkan() {
    printf("ciao\n");
    createInstance();
    //validation layers 
    pickPhysicalDevice();
    createLogicalDevice();
}



void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        // Qui normalmente renderizzeresti il triangolo
        // Per semplicità al momento non facciamo il rendering completo
    }
}

void cleanup() {
    vkDestroyInstance(instance, nullptr); //destroy vk instance 
    //no cleanup for the phisical device, automatically destroyed after destroy of instance
    vkDestroyDevice(device, nullptr); //destroy the logical device 
    //no cleanup for VkQueue, automatically destroyed after destroy of device
    
    glfwDestroyWindow(window); //destroy window instance 
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