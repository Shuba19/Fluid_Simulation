#ifndef UTILS_H
#define UTILS_H


#pragma region structs
struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    //la bind configura la gpu in modo da ottimizzare il passaggio dei dati
    //alla gpu, è come se configurasse uno stato della gpu dicendo qualcosa tipo
    //prendi i dati da qua
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};

        bindingDescription.binding = 0; //start Index in the binding array
        bindingDescription.stride = sizeof(Vertex); 
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; //change if we want to pass to instance rendering

        return bindingDescription;
    }

    //specifica come gestire ottenrere le informazioni dal blocco di vertici della bind
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        //data pos
        attributeDescriptions[0].binding = 0; //which binding to take
        attributeDescriptions[0].location = 0; //reference alla direttiva del vertex shader 
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; //deve matchare il numero di componenti in di shader 
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        //data color
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

struct ParticleInstance {
    glm::vec3 position;
};

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // 0
    {{0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}}, // 1
    {{0.5f, 0.5f},   {0.0f, 0.0f, 1.0f}}, // 2
    {{-0.5f, 0.5f},  {1.0f, 1.0f, 1.0f}}  // 3
};

const std::vector<uint16_t> indices = {
    0, 1, 2,
    2, 3, 0
};

const std::vector<glm::vec3> particlePositions = {
    {0.0f, 0.0f, 0.0f},
    {0.1f, 0.0f, 0.0f},
    {0.2f, 0.0f, 0.0f},
    {0.3f, 0.0f, 0.0f},

    {0.0f, 0.1f, 0.0f},
    {0.1f, 0.1f, 0.0f},
    {0.2f, 0.1f, 0.0f},
    {0.3f, 0.1f, 0.0f},

    {0.0f, 0.2f, 0.0f},
    {0.1f, 0.2f, 0.0f},
    {0.2f, 0.2f, 0.0f},
    {0.3f, 0.2f, 0.0f},
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};


struct appState{
    bool framebufferResized = false;
    float maxDuration = 5.0f;
    
    const int WIDTH = 800;
    const int HEIGHT = 600;

    const int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t currentFrame = 0;

    //vulkan basics
    GLFWwindow* window;
    VkInstance instance;
    VkSurfaceKHR surface;

    //device 
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    //buffers
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    std::vector<VkCommandBuffer> commandBuffers;

    //geometry instaincing 
    VkBuffer instanceBuffer;
    VkDeviceMemory instanceBufferMemory;
    uint32_t particleCount;

    VkCommandPool commandPool; //insieme di command buffer diversi

    //pipeline
    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    VkRenderPass renderPass;


    //swapchain
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkExtent2D swapChainExtent;
    VkFormat swapChainImageFormat;
    std::vector<VkImageView> swapChainImageViews;

    //uniform buffers
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;


};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    // ogni device ha un array di famiglie di code ogniuna che supporta operazioni diverse,
    // a sua volta ogni famiglia di code ha piu code.
    // quindi una volta restituito l'array con tutte le famiglie si sceglie l'indice della famiglia
    // che ci serve e da li si prende una coda per fare quello che ci serve
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
#pragma endregion structs


#pragma region functions

std::vector<char> readFile(const std::string& filename);

#pragma endregion functions

#endif