#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

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

#define LOG(name) std::cout << "DEBUG: " << name << std::endl;

// Concurrent frame count
const int MAX_FRAMES_IN_FLIGHT = 2;

class DeferredRenderer {
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

    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
        alignas(16) glm::vec4 l_pos;
        alignas(16) glm::vec4 l_ambient;
        alignas(16) glm::vec4 l_diffuse;
        alignas(16) glm::vec4 l_specular;
        alignas(16) glm::vec4 m_ambient;
        alignas(16) glm::vec4 m_diffuse;
        alignas(16) glm::vec4 m_specular;
        alignas(16) glm::vec4 m_emission;
        alignas(4)  float m_shininess;
        alignas(4)  bool lighting;
        alignas(4)  bool textures;
    };    
public:
    void run();

private:
    // GLFW members
    GLFWwindow* window;
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    const std::string MODEL_PATH = "resources/models/duck.obj";
    const std::string TEXTURE_PATH = "resources/textures/duck.jpeg";

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

    VkDescriptorSetLayout descriptorSetLayout; 
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;

    VkPipeline graphicsPipeline;

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
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory; 
    VkImageView textureImageView;
    VkSampler textureSampler;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    // Members for ImGui
    VkRenderPass imGuiRenderPass;
    std::vector<VkCommandPool> imGuiCommandPools;
    std::vector<VkCommandBuffer> imGuiCommandBuffers;
    std::vector<VkFramebuffer> imGuiFrameBuffers;

    // ImGui Config Attributes
    // material Properties
    float m_specScale = .6f;
    float m_diffScale = .8f;
    float m_ambScale = .2f;
    float m_emissScale = .0f;
    float m_shinScale = 1.f;
    // Model/Light Transforms
    ImVec2 m_rotation = ImVec2(0.f, 0.f);
    ImVec2 m_translation = ImVec2(0.f, 0.f);
    ImVec2 l_rotation = ImVec2(0.f, 0.f);
    float m_zoom = 1.f;
    // Render Settings
    bool enableTextures = false;
    bool enableLighting = false;

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
    void createDiscriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets();
    void createGraphicsPipeline();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createRenderPass();
    void createFramebuffers();
    void createCommandPool(VkCommandPool* cPool, VkCommandPoolCreateFlags flags);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    void createCommandBuffers();
    void createSyncObjects();
    void drawFrame();
    void recreateSwapChain();
    void cleanupSwapChain(); 
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void updateUniformBuffer(uint32_t currentImage);
    void createTextureImage();
    void createTextureImageView();
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void createTextureSampler();
    void createDepthResources();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    bool hasStencilComponent(VkFormat format);
    void loadModel();

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    void initImGui();
    void createImGuiRenderPass();
    void drawImGui();
    void createImGuiCommandBuffers(VkCommandBuffer* _commandBuffer, uint32_t _commandBufferCount, VkCommandPool& _commandPool);
    void cleanupImGuiSwapChain();
    void recreateImGuiSwapChain();

    // Debug functions
    bool checkValidationLayerSupport();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void setupDegubMessenger();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};

// Proxy functions for calling extension functions
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
std::vector<char> readFile(const std::string& filename);