#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/gtx/rotate_vector.hpp >

#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <cstdint> // Necessary for UINT32_MAX
#include <fstream>
#include <array>
#include <algorithm>
#include <chrono>
#include <math.h>

#define LERP(a,b,t) (a + (t*(b-a)))

// Concurrent frame count
const int MAX_FRAMES_IN_FLIGHT = 2;

class A2VulkanApp {
public:
    // Class Structures

    // A struct for finding and storing the indices of the required queue familied on a device
    struct QueueFamilyIndices {
        // Indices fo the queue family on a deivce
        // Optional so that has_value() can be used to see if a value has been found
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        // If the queue Family search is complete, all families have an index
        bool isComplete() {
            return graphicsFamily.has_value() and presentFamily.has_value();
        }

        // Assignment overload
        void operator =(const QueueFamilyIndices& other) {
            graphicsFamily = other.graphicsFamily;
            presentFamily = other.presentFamily;
        }
    };

    // Struct to store swap chain support for a device
    struct SwapChainSupportDetails {
        // Capabilited of the surface (supported image count, supported image size ect.)
        VkSurfaceCapabilitiesKHR capabilities;
        // How the surface is formatted (pixel format, color space ect.)
        std::vector<VkSurfaceFormatKHR> formats;
        // Modes of presentation supported
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec3 normal;
        glm::vec2 texCoord;

        // Specify the binding for transfering the vertex data to GPU
        static VkVertexInputBindingDescription getBindingDescription() {
            // The binding description to be returned
            VkVertexInputBindingDescription bindingDescription{};
            // As all of the vertex data is in one array we only have one binding
            bindingDescription.binding = 0;
            // How many bytes there are between items in the array
            bindingDescription.stride = sizeof(Vertex);
            // Move to the next data entry after every vertex as we are not using isntanced rendering
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        // One attribute description for position, one for color
        static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

            // Position
            // Which binding to get the date from
            attributeDescriptions[0].binding = 0;
            // Location derective in the vertex shader input
            attributeDescriptions[0].location = 0;
            // The Format of the data for the attribute
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            // The offset into the Vertex struct to get the data
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            // Color
            // Same settings but for the color attribute of the vertex
            attributeDescriptions[1].binding = 0;
            // Since this is the second attribute it will be bound to location=1
            attributeDescriptions[1].location = 1;
            // Since Color is a vec3 it is a 3 element format
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            // Normal
            attributeDescriptions[2].binding = 0;
            // Since this is the second attribute it will be bound to location=1
            attributeDescriptions[2].location = 2;
            // Since Color is a vec3 it is a 3 element format
            attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, normal);

            // Tex Coordinates
            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            // Tex coords are a vec2
            attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

            return attributeDescriptions;
        }

        // Equality test between vertices
        bool operator==(const Vertex& other) const {
            return pos == other.pos && color == other.color && texCoord == other.texCoord && normal == other.normal;
        }
    };

    //struct TerrainUBO {
    //    alignas(16) glm::mat4 model;
    //    alignas(16) glm::mat4 view;
    //    alignas(16) glm::mat4 proj;
    //    alignas(16) glm::vec4 l_pos;
    //    alignas(16) glm::vec4 l_ambient;
    //    alignas(16) glm::vec4 l_diffuse;
    //    alignas(16) glm::vec4 l_specular;
    //    alignas(16) glm::vec4 m_ambient;
    //    alignas(16) glm::vec4 m_diffuse;
    //    alignas(16) glm::vec4 m_specular;
    //    alignas(16) glm::vec4 m_emission;
    //    alignas(4)  float m_shininess;
    //};   
    struct TerrainUBO {
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
        alignas(16) glm::vec4 l_pos;
        alignas(16) glm::vec4 m_ambient;
        alignas(16) glm::vec4 m_diffuse;
        alignas(16) glm::vec4 m_specular;
        alignas(16) glm::vec4 m_emission;
        alignas(4)  float m_shininess;

        alignas(4)  float binWidth;
        alignas(4)  int binQuads;
    };

    struct SkyUBO {
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    struct TerrainPushData {
        glm::vec2 pos;
    };

    struct TerrainBin {
        bool visible = true;
        TerrainPushData pushData;
        glm::vec3 viewPos;

        // Get the mid point of the bin
        glm::vec2 getMid(float bWidth) {
            return pushData.pos + (glm::vec2(bWidth, bWidth) / 2.f);
        }
    };

    std::vector<TerrainBin> terrBins;

public:
    void run();

private:
    // GLFW members
    GLFWwindow* window;
    const uint32_t WIDTH = 1600;
    const uint32_t HEIGHT = 900;

    const std::string TEXTURE_PATH = "resources/textures/hTexMap2.png";
    const std::string HMAP_PATH = "resources/textures/height_map3.png";
    const std::string SKYBOX_PATH = "resources/textures/skybox.png";

    const float BINWIDTH = 200;
    const float BINRAD = sqrt(2 * BINWIDTH * BINWIDTH)/2.f; // Distance from center to corner of a bin
    const int BINQUADS = 200;
    const int BINCOUNT = 50; // Note, this is on one axis, actual count will be ^2
    const float TERRBASE = 0.f;

    // View frustum settings
    const float fovy = glm::radians(45.f);
    const float zNear = 1.f;
    const float zFar = 2000.f;

    /*
    Unoptimised: The game slows down (~30 FPS) at around 100x100 = 10,000 bins, each with 20,000 Triangles
                 This is around 200,000,000 triangles per frame, or 6 billion triangles a second   
                 The game becomes unplayable (~10 FPS) at around 150x150 = 22,500 bins, or around 
                 450,000,000 triangles per frame
    */

    // Vulkan Members
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger; 
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR surface;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    std::vector<VkImageView> swapChainImageViews;

    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout terrainDescriptorSetLayout;
    std::vector<VkDescriptorSet> terrainDescriptorSets;

    VkDescriptorSetLayout skyboxDescriptorSetLayout;
    std::vector<VkDescriptorSet> skyboxDescriptorSets;

    VkRenderPass renderPass;

    VkPipelineLayout terrainPipelineLayout;
    VkPipeline terrainPipeline;

    VkPipelineLayout skyboxPipelineLayout; // Done
    VkPipeline skyboxPipeline;

    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;

    bool framebufferResized = false;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    std::vector<VkBuffer> terrainUBs;
    std::vector<VkDeviceMemory> terrainUBMemory;
    std::vector<VkBuffer> skyboxUBs;
    std::vector<VkDeviceMemory> skyboxUBMemory;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    VkImage heightMapImage;
    VkDeviceMemory heightMapImageMemory;
    VkImageView heightMapImageView;
    VkSampler heightMapSampler;

    VkImage skyboxImage;
    VkDeviceMemory skyboxImageMemory;
    VkImageView skyboxImageView;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    std::chrono::steady_clock::time_point fTimeStart = std::chrono::steady_clock::now();

    // Flight Controls
    float speed = 0.f; // Speed in units/second
    glm::vec3 planeHeading = glm::vec3(-0.4f, .9f, 0.f);
    glm::vec3 planeUp = glm::vec3(0.f, 0.f, 1.f);
    glm::vec3 position = glm::vec3(5000, 5000, 130.f);

    // Transform Matrices
    glm::mat4 viewMat;
    glm::mat4 projMat;

    // Members for ImGui
    VkRenderPass imGuiRenderPass;
    std::vector<VkCommandPool> imGuiCommandPools;
    std::vector<VkCommandBuffer> imGuiCommandBuffers;
    std::vector<VkFramebuffer> imGuiFrameBuffers;

    // ImGui Config Attributes
    // material Properties
    float m_specScale = .3f;
    float m_diffScale = .7f;
    float m_ambScale = .05f;
    float m_emissScale = .0f;
    float m_shinScale = 1.f;
    // Model/Light Transforms
    ImVec2 m_rotation = ImVec2(0.f, 0.f);
    ImVec2 m_translation = ImVec2(0.f, 0.f);
    ImVec2 l_rotation = ImVec2(70.f, 0.f);
    float m_zoom = 1.f;

    bool showMetrics = true;
    bool enableCulling = true;
    bool updateCulling = true;
    bool enableFreeFlight = false;

    // A list of all validation layers to be used in debugging mode
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // Enable or disable validation layers depending on if the program is run in debug mode or not
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif // NDEBUG

    // Main Vulkan Functions
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();
    void createInstance();
    void createSurface();
    std::vector<const char*> getRequiredExtensions();
    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createFramebuffers();
    void createSyncObjects();
    void drawFrame();
    void recreateSwapChain();
    void cleanupSwapChain(); 
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // Per-Pipeline
    void createTerrainPipeline(); 
    void createSkyboxPipeline();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createCommandPool(VkCommandPool* cPool, VkCommandPoolCreateFlags flags);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    // Per Model
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    void createCommandBuffers();
    void recordDrawCommands(size_t commBuffer_i);
    void updateTerrainUB(uint32_t currentImage);
    void updateSkyboxUB(uint32_t currentImage);
    void loadModel();
    void generateTerrain(float width, int splits);
    void generateIndexArrOnly(float width, int splits);
    void generateTerrainBins(int sqrtCount);
    void createTerrainDiscriptorSetLayout();
    void createSkyboxDiscriptorSetLayout();
    void createDescriptorPool();
    void createTerrainDescriptorSets();
    void createSkyboxDescriptorSets();
    // Per-Model Textures
    void createTextureImage(const char* fileName, VkImage &image, VkDeviceMemory &imageMemory, VkFormat imageFormat);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void loadImageToVulkan(const char* fileName, VkImage& image, VkImageView& imageView, VkDeviceMemory& imageMemory, VkFormat imageFormat);
    void createTextureSampler(VkSampler& imageSampler);
    void createHeightMapSampler(VkSampler& imageSampler);
    float getHMapTexel(stbi_uc* pixels, int texWidth, int texHeight, float u, float v);
    glm::vec3 getHMapNorm(stbi_uc* pixels, int texWidth, int texHeight, float u, float v);
    void createDepthResources();

    // Helper Functions
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    bool hasStencilComponent(VkFormat format);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    // Imgui Functions
    void initImGui();
    void createImGuiRenderPass();
    void drawImGui();
    void createImGuiCommandBuffers(VkCommandBuffer* _commandBuffer, uint32_t _commandBufferCount, VkCommandPool& _commandPool);
    void cleanupImGuiSwapChain();
    void recreateImGuiSwapChain();

    // Game-controling functions
    void updateTransformMats();
    void updatePhys();
    void turnPlane(float pitch, float yaw, float roll);
    void cullBins();

    // Debug functions
    bool checkValidationLayerSupport();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void setupDegubMessenger();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    // GLFW Callbacks
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};

// Proxy functions for calling extension functions
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
std::vector<char> readFile(const std::string& filename);