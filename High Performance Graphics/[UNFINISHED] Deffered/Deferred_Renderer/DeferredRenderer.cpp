#include "DeferredRenderer.h"

int main() {
    DeferredRenderer app;

    //try {
        app.run();
    //}
    /*catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }*/

    return EXIT_SUCCESS;
}

// Entry point for the vulkan app class
void DeferredRenderer::run() {
    // The window must be initialised before Vulkan
    initWindow();
    initVulkan();
    mainLoop();
    // Once the mainLoop has exited, clean up all of the assets before closing the app
    cleanup();
}

// initialise the GLFW window
void DeferredRenderer::initWindow() {
    // Initialise the glfw library, enables use of glfw functionality
    glfwInit();

    // Remove the openGL api from the window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Create the glfw window instance
    window = glfwCreateWindow(WIDTH, HEIGHT, "A1 Vulkan App", nullptr, nullptr);

    // Set the pointer stored within the window object that points to the DeferredRenderer for use in static callbacks
    glfwSetWindowUserPointer(window, this);

    // Set the callback for window resize
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

// A callback for when the window is resized so that vulkan can react accordingly
void DeferredRenderer::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    // Get the DeferredRenderer instance that triggered the callback
    auto app = reinterpret_cast<DeferredRenderer*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

// Handles the entire creation of the vulkan backend
// The order of the function calls here is important as some functions depend on others
void DeferredRenderer::initVulkan() {
    createInstance();
    setupDegubMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();

    createSwapChain();
    createImageViews(); 
    createRenderPass(); 
    createDiscriptorSetLayout();
    createCommandPool(&commandPool, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    createDepthResources();
    createFramebuffers();
    createGeoFramebuffers();
    createPipelines();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    LOG("Load Start");
    loadModel();
    LOG("Load Finish");
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool(); 
    createDescriptorSets();
    createCommandBuffers();
    createGeoCMDBuffer();
    createSyncObjects();

    // Because ImGui relies on parts of the vulkan app, it must be initialised last
    initImGui();
}

// Function to initialise all of the assets that ImGui requires to run
void DeferredRenderer::initImGui() {
    IMGUI_CHECKVERSION();
    // Create ImGui Context
    ImGui::CreateContext();

    // Setup the IO for ImGui
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    // Enable Keyboard input
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    io.DisplaySize = ImVec2((float) swapChainExtent.width, (float)swapChainExtent.height);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    // Set Color Theme
    ImGui::StyleColorsDark();

    // Pre-reqs to init of ImplVulkan
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    createImGuiRenderPass();

    // Pass the vulkan window to ImGui
    ImGui_ImplGlfw_InitForVulkan(window, true);
    // Struct for initialising the ImGui Vulkan integration
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = instance;
    init_info.PhysicalDevice = physicalDevice;
    init_info.Device = device;
    init_info.QueueFamily = indices.graphicsFamily.value();
    init_info.Queue = graphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = descriptorPool;
    init_info.Allocator = nullptr;
    init_info.MinImageCount = static_cast<uint32_t>(swapChainImages.size());
    init_info.ImageCount = static_cast<uint32_t>(swapChainImages.size());
    init_info.CheckVkResultFn = nullptr;
    // Initialise the ImGui Vulkan Integration
    ImGui_ImplVulkan_Init(&init_info, imGuiRenderPass);

    // Create a temporary buffer to pass the fonts to GPU
    VkCommandBuffer command_buffer = beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
    endSingleTimeCommands(command_buffer);

    // Create the command pools and buffers
    imGuiCommandPools.resize(swapChainImageViews.size());
    imGuiCommandBuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        createCommandPool(&imGuiCommandPools[i], VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        createImGuiCommandBuffers(&imGuiCommandBuffers[i], 1, imGuiCommandPools[i]);
    }

    // Make room for one ImGui frame buffer per Vulkan swapchain image
    imGuiFrameBuffers.resize(swapChainImageViews.size());

    // Framebuffer creation info
    VkImageView attachment[1];
    VkFramebufferCreateInfo fbInfo = {};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = imGuiRenderPass;
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments = attachment;
    fbInfo.width = swapChainExtent.width;
    fbInfo.height = swapChainExtent.height;
    fbInfo.layers = 1;
    // Create a framebuffer per swapchain image, with the swapchain image view being the attachment for the new framebuffer
    for (uint32_t i = 0; i < swapChainImageViews.size(); i++)
    {
        attachment[0] = swapChainImageViews[i];
        auto err = vkCreateFramebuffer(device, &fbInfo, VK_NULL_HANDLE, &imGuiFrameBuffers[i]);
        if (err != VK_SUCCESS) throw std::runtime_error("Failed to create ImGui frame buffers");
    }

}

// Create the renderpass specific to ImGui
void DeferredRenderer::createImGuiRenderPass() {
    // Info struct for the ImGui renderpass attachment
    VkAttachmentDescription attachment = {};
    // Copy the image format for the standard render pass
    attachment.format = swapChainImageFormat;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    // The ImGui renderpass will run after the standard renderpass so we want the output of the first renderpass to be preserved
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    // Ensure the output of the ImGui render pass is preserved
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // This Renderpass will be immediately before presenting
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Specify the layout of the attachment
    VkAttachmentReference color_attachment = {};
    color_attachment.attachment = 0;
    color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Subpass specification
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment;

    // Ensure the ImGui render pass happens after the original renderpass has written to the frame buffer
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;  // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &attachment;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;
    if (vkCreateRenderPass(device, &info, nullptr, &imGuiRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("Could not create Dear ImGui's render pass");
    }
}

// Function to query the available physical devices and choose the first one that suits our needs
void DeferredRenderer::pickPhysicalDevice() {
    // Query how many physical devices are available that support vulkan
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    // If there are no vulkan supported devices available
    if (deviceCount == 0) {
        // The program cannot run so throw an error to halt execution
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }

    // Create a vector with all of the available devices
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // For each available device
    for (const auto& device : devices) {
        // If the device is suitable
        if (isDeviceSuitable(device)) {
            // Store the device
            physicalDevice = device;
            // Only one suitable device is needed so we break after finding one
            break; 
        }
    }

    // If a device is never stored
    if (physicalDevice == VK_NULL_HANDLE) {
        // There is no suitable GPU so throw error to halt execution
        throw std::runtime_error("Failed to find a suitable GPU");
    }
}

// Use the chosen physical device to create a logical device that vulkan can use to interface with it
void DeferredRenderer::createLogicalDevice() {
    // Get the queue indices for the physical device selected
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    // Create a vector of structs for creating the required queues, on instance per queue
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    // Fill a set with the physical device indices of the required queues
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    // For each required queue, fill a creation struct with the appropriate data
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        // The index of the queue, gotten from the set that we filled earlier
        queueCreateInfo.queueFamilyIndex = queueFamily;
        // For now we only define one queue of each type
        queueCreateInfo.queueCount = 1;
        // The priority is constant as there is only one queue
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // A struct to define the features we will need from the device
    VkPhysicalDeviceFeatures deviceFeatures{};
    // We want to enable anisotropic filtering
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    // Struct to defien the creation information for the logical device
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO; 
    // There is one queue per index in queueCreateInfos, pass them to the creation info struct
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    // Specify the required features
    createInfo.pEnabledFeatures = &deviceFeatures;
    // Specify the number and names of the device extensions to enable
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // Specify the validation layers for the device, like we did for the instance
    // Unneeded for modern version of vulkan but specified for backwards compatability
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    // Attempt to create the device
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        // If device creation fails throw an error
        throw std::runtime_error("failed to create logical device");
    }

    // Retrieve the device's queue handles for later use
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

// Test a given physical device and return whether or not it suits our needs
bool DeferredRenderer::isDeviceSuitable(VkPhysicalDevice device) {
    // Get the properties of the device (name, type, vulkan version ect.)
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // get the feature set of the device
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // Find the queue families for the device
    QueueFamilyIndices indices = findQueueFamilies(device);

    // Query whether the device supports the required extensions
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    // Query whether teh device has adequate swap chain features
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        // Query devie's swap chain support
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        // True if there is at least one surface format and presentation mode available
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    // Require a physical GPU with geometry shader capabilities and has a graphics capable queue family
    // NOTE: this would be bad practice as a physical device may not be available, in which case integrated graphics could be used
    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU and
        deviceFeatures.geometryShader and
        indices.isComplete() and
        extensionsSupported and
        swapChainAdequate and
        deviceFeatures.samplerAnisotropy;
}

// Return whether a physical device supports all the extensions we need
bool DeferredRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    // Query how many etensions the device supports
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    // Fill a vector with the supported device extensions
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    // Create a set of the requiered extensions from the deviceExtensions vector
    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    // For each available extension
    for (const auto& extension : availableExtensions) {
        // Remove the extension from the set fo required extensions
        requiredExtensions.erase(extension.extensionName);
    }

    // If the set is empty all required extensions are available
    return requiredExtensions.empty();
}

// Find the indices in the device's queue families for all of the queue families we need
DeferredRenderer::QueueFamilyIndices DeferredRenderer::findQueueFamilies(VkPhysicalDevice device) {
    // Return value
    QueueFamilyIndices indices;

    // Query how many queue families the device has
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    // Fill a vector with the queue families of the device
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    // i will be the index of the queue family within the for loop
    int i = 0;

    // For each queue family
    for (const auto& queueFamily : queueFamilies) {
        // If it supports graphics operations, set the graphics family to the current queue family index
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }

        // If the device has all the queue families required, there is no need to keep searching, so break
        if (indices.isComplete()) break;

        i++;
    }

    return indices;
}

// Test a device's swap chain support and return the supported features
DeferredRenderer::SwapChainSupportDetails DeferredRenderer::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    // Surface Capabilites

    // Query the device's surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    // Surface Formats

    // Query the number of surface formats available
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    // Fill the details.formats with each of the supported formats
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    // Presentation Modes

    // Query how many presentation modes are supported
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    // Fill presentModes with each of the supported presentation modes
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

// Of the formats available, return the one that suits our needs
VkSurfaceFormatKHR DeferredRenderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // For each available format
    for (const auto& availableFormat : availableFormats) {
        // Check if it has the BGRA-SRGB format and color space
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            // Return said format
            return availableFormat;
        }
    }

    // If no format with siad format and color space is found just return the first format
    return availableFormats[0];
}

// The same as Format (above) but for the present mode
VkPresentModeKHR DeferredRenderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // Look through the available present modes
    for (const auto& availablePresentMode : availablePresentModes) {
        // If the present current present mode is tripple buffering, return it
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    // If thripple buffering is not found return double buffering as it is always garunteed to be available
    return VK_PRESENT_MODE_FIFO_KHR;
}

// Set the swapchain extent (mainly size)
VkExtent2D DeferredRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    // If the width is not UINT32_MAX then the extent dimentions are not editable
    if (capabilities.currentExtent.width != UINT32_MAX) {
        // Return the current extent
        return capabilities.currentExtent;
    }
    // If the width is equal to UINT32_MAX then the dimentions can be edited
    else {
        // Get the size of the window in pixels from glfw
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        // Set the size of the extent to the pixel size of the window
        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        // Clamp the values of width and height between the min and max of each dimention respectively
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        // Return the extent
        return actualExtent;
    }
}

// Function for the creation of the swapchains needed for the vulkan to draw to/present
void DeferredRenderer::createSwapChain() {
    // Find the available configuratios for the swap chain
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    // Use the choose* functions to select the best available configuration for the swapchain
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    // Specify how many imagess in the swap chain
    // Using the minimum would mean sometimes we need to wait for drivers at times to be able to render to an image
    // Adding one removes this issue
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    // Ensure the image count does not exceed the maximum
    // if max == 0 then there is no maximum
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    // Swapchain creation info structure
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    // Specify surface swapchain is using
    createInfo.surface = surface;
    // Pass image count we have decided on
    createInfo.minImageCount = imageCount;
    // Specify swap chain configuration we decided on earlier
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    // Layer count per image, always 1 ecept for stereoscopic 3D apps
    createInfo.imageArrayLayers = 1;
    // Use case for the image in the swapchain
    // As we are rendering directly too them we use color attachment
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Get the indices of the queue families for the physical device
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    // Transfer the indices to an array
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    // If the graphics and present queue family are different queues
    if (indices.graphicsFamily != indices.presentFamily) {
        // Allow the swapchain image to be used by multiple queue families at the same time
        // This is less efficient but avoids the need for ownership transfer between queue families
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        // How many queue families will be using the image
        createInfo.queueFamilyIndexCount = 2;
        // The indices of the queue families that will use the image
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    // If the graphics and presentation queue family are the same
    else {
        // As there will only be one queue family using the image it is more efficient to use exclusive mode
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        // Unneeded for exclusive sharing mode
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    // No transformation is required in the swap chain
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

    // Ignore the alpha channel as we do not want the window to blend with other windows in the system
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    // Specify the present mode
    createInfo.presentMode = presentMode;
    // Clipping is an optimisation for when pixels in the window are obscured by other windows
    // Getting the colors of such obscured pixels will be unpredictable with clipping enabled
    createInfo.clipped = VK_TRUE;

    // Used in the case of a swapchain becoming invalid and being recreated
    // This variable would point to the old, now invalid, swapchain
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    // Attempt to create the swap chain
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        // If anything but VK_SUCCESS is returned, there is an error so throw an error to halt execution
        throw std::runtime_error("failed to create swap chain!");
    }

    // Reuse imageCount as it isnt reused after this point
    // Query the number of images in the swap chain
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    // Fill the vector with handles to the images in teh swap chain
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    // Store the format and extent of the swapchain in member functions
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

// Create the image views associated with the swap chain's images
void DeferredRenderer::createImageViews() {
    // Resize vectors to have one entry per swap chain image
    swapChainImageViews.resize(swapChainImages.size());

    // For each swapchain image
    for (uint32_t i = 0; i < swapChainImages.size(); i++) {
        // Create and store an image view, one per swapchain image
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }

}

// Function to simply fill the debug messenger with the configuration we desire
// Editing these values will change what messages the validation layer will print
void DeferredRenderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    // Info struct for creating the messenger
    createInfo = {};
    // sType is specified so the struct is handdled correctly if passed to a void pointer
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    // Specify what severities should the messenger be called for
    createInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    // Specify which message types should trigger a messenger callback
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    // Specify the callback function for the messenger
    createInfo.pfnUserCallback = debugCallback;
    // optional data pointers could be added that would help with debugging
    createInfo.pUserData = nullptr; // Optional
}

// Function for the initial setup of the debug messenger
void DeferredRenderer::setupDegubMessenger() {
    // If there are no validation layers active, there is no need for a debug messenger
    if (!enableValidationLayers) return;

    // Info struct for creating the messenger
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    // Populate the info struct with the approriate values
    populateDebugMessengerCreateInfo(createInfo);

    // Attempt to create the Debug Messenger using the proxy function, if VK_SUCCESS is not returned the creation failed
    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        // Throw error to halt execution
        throw std::runtime_error("Failed to set up debug messenger");
    }
}

// Function for creating a vulkan instance
void DeferredRenderer::createInstance() {
    // If validation layers are required and there is at least one required layer that is not supported
    if (enableValidationLayers and !checkValidationLayerSupport()) {
        // Throw an error to halt execution
        throw std::runtime_error("One or more requested validation layers are not supported");
    }

    // Init the app information struct
    VkApplicationInfo appInfo{};
    // sType is specified so the struct is handdled correctly if passed to a void pointer
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    // What information about the app that each field represents is self explanatory
    appInfo.pApplicationName = "A1 Vulkan App";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Init the struct to define settings for creating the instance
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo; // Information for the instance being created

    // get the extensions requied by the program
    auto extensions = getRequiredExtensions();

    // Set the desired extensions and count in the createInfo struct
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Info struct for debuging vkCreateInstance and vkDestroyInstance
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;

    // If validation layers are enabled (debug mode)
    if (enableValidationLayers) {
        // pecify how many layers are required
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        // Pass the names specified in the vector to the enabled validation layers
        createInfo.ppEnabledLayerNames = validationLayers.data();

        // Populate the info struct with the approriate values
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        // No validation layers are required outside of debug
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    // Create the vulkan instance using the setting specified in the structs
    // If vkCreateInstacne returns anything but VL_SUCCESS then the instance was not initialised correctly
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        // Throw an error to terminate the program
        throw std::runtime_error("failed to create vulkan instance");
    }

}

// Function to create an instance for vulkan to interface with the GLFW window
void DeferredRenderer::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

// Function to create the graphics pipeline and all of the objects it requires
void DeferredRenderer::createGraphicsPipeline()  {
    // Read the shader files using the helper function
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");

    // Create shaderModule classes from each of the shaders
    // They are local inside the function as they are unneeded once the pipeline has been created
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    // Info struct for vertex shader pipeline stage creation
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // Specify which pipeline stage is being created
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    // Pass the shader data
    vertShaderStageInfo.module = vertShaderModule;
    // Specify the entrypoint of the shader
    vertShaderStageInfo.pName = "main";

    // Info struct for fragment shader pipeline stage creation
    // Identical to vertex besides the stage flag and shaderModule passed
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    // Add both structs to an array
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // Info Struct for defining the vertex input state
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    // get the binding and attribute discriptors for the vertex array
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    // Specify how many binding and attribute descriptors we have
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    // Pass the binding and attribute descriptors
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // Info struct to specify details of how vertices will be used
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    // Verts will be in groups of three, each group being a single triangle
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    // We are not using _STRIP primative types so this is unneeded
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Define the viewport (region of the frame buffer) the pipeline will draw to
    VkViewport viewport{};
    // Define the area of the viewport as the entire frame buffer
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    // As we will use swap chain images as the framebuffers, we will use their size
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    // Standard values for depth range
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Scissor defines what section of the framebuffer should be stored
    VkRect2D scissor{};
    // As we want to draw the entire frame buffer we will use the full size defined in the extent of the swapchain
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;

    // info struct for viewport state creation
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    // Define how many viewports we will use
    viewportState.viewportCount = 1;
    // Specify the viewport(s) to be used
    viewportState.pViewports = &viewport;
    // Deifine how many scissor rectangles to use
    viewportState.scissorCount = 1;
    // Specify the scissor rectangle(s)
    viewportState.pScissors = &scissor;

    // Info struct for rasterizer creation
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    // This setting is used to clamp depth values outside of the near/far range
    // It is not helpful for standard rendering
    rasterizer.depthClampEnable = VK_FALSE;
    // Rasterizer discard will disable all output from the rasterizer
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    // Polygon mode defines how the polygons are rendered
    // Here we will fill the polygons, other options include:
    // VK_POLYGON_MODE_LINE - Wireframe rendering
    // VK_POLYGON_MODE_POINT - Only render vertices
    // VK_POLYGON_MODE_FILL - Full polygon rendering
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    // Thickness of lines
    rasterizer.lineWidth = 1.0f;
    // Specify which faces to cull, e.g. we do not want to render back faces
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    // Specify that a trignale view from the front should be defined in CCW order
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    // Sometimes used for shadow mapping, not needed for standard rendering
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    // Info Struct for multisampling
    // We leave disabled for now
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    // Depth buffering config
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    // Enable depth testing and writing
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    // Specify we want depth test to pass if the fragment is closer than the current depth
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    // No depth bound testing or stencil
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // Info Struct for per framebuffer color blending configuration
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    // Pass through all color channels
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // No color blending needed, we will simply use the input color as output color
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    // Info Struct for global color blending configuration
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // Used for bitwise color blending, not needed here
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    // Specify the framebuffer blending mode for each framebuffer
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    // Info struct for pipeline layout creation
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // Define how many layouts we will need
    // Specify the number and pointers to the descriptor set layouts
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    // Attempt to create the pipeline layout
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        // Error (No VK_SUCCESS return) means we need to halt execution through throwing an error
        throw std::runtime_error("failed to create pipeline layout!");
    }

    // Creation info struct for a pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // Specify the number of custom shader stages
    pipelineInfo.stageCount = 2;
    // Pass a reference to the custom shader stages
    pipelineInfo.pStages = shaderStages;

    // Pass references to all of the structures defining the functionality of the fixed function shader stages
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional

    // Pass the pipeline layour specification
    pipelineInfo.layout = pipelineLayout;

    // Pass the information specified about the renderpass
    pipelineInfo.renderPass = renderPass;
    // Index of the subpass where the graphics pipeline is used
    pipelineInfo.subpass = 0;

    // Attempt to create the pipeline
    // Note: This function can create multiple pipelines at the same time, but we do not use that feature here
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &finalPipeline) != VK_SUCCESS) {
        // If VK_SUCCESS is not returned, throw an error to half execution
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    // Clean up the memory associated with the shader modules
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

// Create th epipeline for the geometric render pass
void DeferredRenderer::createPipelines() {

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    // This setting is used to clamp depth values outside of the near/far range
    // It is not helpful for standard rendering
    rasterizer.depthClampEnable = VK_FALSE;
    // Rasterizer discard will disable all output from the rasterizer
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    // Polygon mode defines how the polygons are rendered
    // Here we will fill the polygons, other options include:
    // VK_POLYGON_MODE_LINE - Wireframe rendering
    // VK_POLYGON_MODE_POINT - Only render vertices
    // VK_POLYGON_MODE_FILL - Full polygon rendering
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    // Thickness of lines
    rasterizer.lineWidth = 1.0f;
    // Specify which faces to cull, e.g. we do not want to render back faces
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    // Specify that a trignale view from the front should be defined in CCW order
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    // TODO: Shadow Settings
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    // Info Struct for per framebuffer color blending configuration
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState attachmentStates[] = { colorBlendAttachment , colorBlendAttachment , colorBlendAttachment , colorBlendAttachment };

    // Info Struct for global color blending configuration
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    // Used for bitwise color blending, not needed here
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 3;
    colorBlending.pAttachments = attachmentStates;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    // NOTE: This is an alto to the dynamic states, imma try it
    //// Define the viewport (region of the frame buffer) the pipeline will draw to
    //VkViewport viewport{};
    //// Define the area of the viewport as the entire frame buffer
    //viewport.x = 0.0f;
    //viewport.y = 0.0f;
    //// As we will use swap chain images as the framebuffers, we will use their size
    //viewport.width = (float)geoFrameBuffer.width;
    //viewport.height = (float)geoFrameBuffer.height;
    //// Standard values for depth range
    //viewport.minDepth = 0.0f;
    //viewport.maxDepth = 1.0f;
    //VkRect2D scissor{};
    //scissor.offset = { 0, 0 };
    //scissor.extent.width = geoFrameBuffer.width;
    //scissor.extent.height = geoFrameBuffer.height;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    //viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    //viewportState.pScissors = &scissor;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
    pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
    pipelineDynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());

    auto vertShaderCode = readFile("shaders/vert_geo.spv");
    auto fragShaderCode = readFile("shaders/frag_geo.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // Info Struct for defining the vertex input state
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // Creation info struct for a pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // Specify the number of custom shader stages
    pipelineInfo.stageCount = 2;
    // Pass a reference to the custom shader stages
    pipelineInfo.pStages = shaderStages;

    // Pass references to all of the structures defining the functionality of the fixed function shader stages
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &pipelineDynamicStateCreateInfo; // Optional

    // Pass the pipeline layour specification
    pipelineInfo.layout = pipelineLayout;

    // Pass the information specified about the renderpass
    pipelineInfo.renderPass = geoFrameBuffer.renderPass;
    // Index of the subpass where the graphics pipeline is used
    pipelineInfo.subpass = 0;

    // Attempt to create the geo pass pipeline
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &geoPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create geo pass pipeline!");
    }

    // Clean up the memory associated with the shader modules
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);

    // Final Render pass pipeline
    rasterizer.cullMode = VK_CULL_MODE_NONE;

    // Load the different shaders
    vertShaderCode = readFile("shaders/vert_final.spv");
    fragShaderCode = readFile("shaders/frag_final.spv");
    auto vertShaderModule2 = createShaderModule(vertShaderCode);
    auto fragShaderModule2 = createShaderModule(fragShaderCode);
    vertShaderStageInfo.module = vertShaderModule2;
    fragShaderStageInfo.module = fragShaderModule2;
    VkPipelineShaderStageCreateInfo shaderStages2[] = { vertShaderStageInfo, fragShaderStageInfo };
    pipelineInfo.pStages = shaderStages2;

    // Info Struct for defining an empty vertex input state
    VkPipelineVertexInputStateCreateInfo vertexInputInfoEmpty{};
    vertexInputInfoEmpty.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineInfo.pVertexInputState = &vertexInputInfoEmpty;

    // Info Struct for per framebuffer color blending configuration
    VkPipelineColorBlendAttachmentState colorBlendAttachmentFinal{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachmentFinal;

    pipelineInfo.renderPass = renderPass;

    // Attempt to create the geo pass pipeline
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &finalPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create final pass pipeline!");
    }
}

// Given a vector of chars that is the contents of a shader file
// Create and return a VkShaderModule with that code
VkShaderModule DeferredRenderer::createShaderModule(const std::vector<char>& code) {
    // Info struct for creating a shader module
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    // Spesify how many bytes of data there is
    createInfo.codeSize = code.size();
    // Pass the pointer to the buffer holding the shader data
    // code.data() is a char* and .pCode expects a uint32_t* so we must cast
    // This cast is valid as vector<T> alligns the data correctly
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    // Attempt to create the shader module
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        // If VK_SUCCESS is not returned, an error has occured so throw an error to halt execution
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

// Create the render pass
void DeferredRenderer::createRenderPass() {

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Reference to the depth buffer attachment
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Info struct for describing a subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    // Create an array of the attachments
    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment };
    //std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    // Render pass creation info struct
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    // Specify color attachment count
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    // Specify reference to color attachment(s)
    renderPassInfo.pAttachments = attachments.data();
    // Specify subpass count
    renderPassInfo.subpassCount = 1;
    // Specify reference to subpass(es)
    renderPassInfo.pSubpasses = &subpass;

    // Struct to specify the subpass dependency
    VkSubpassDependency dependency{};
    // These two lines specify a dependency between the implicit subpass before the render pass and the first subpass in the render pass
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    // Specify that we need to wait for the color attachment and depth attachment to be read before continuing
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;

    // Specify that it is the writing to the color attachments and depth attachment that must wait
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // Specify how many, and a reference to the description of dependencies for the render pass
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    // Attempt render pass creation
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        // If VK_SUCCESS is not returned throw an error and halt execution
        throw std::runtime_error("failed to create render pass!");
    }

}

// Check that all of validation layers that are required are supported
bool DeferredRenderer::checkValidationLayerSupport() {
    // Get the number of layers supported
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    // Fill a vector with all of the supported layers
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // For each layer that si required
    for (const char* layer: validationLayers) {
        // Whether the required layer has been found
        bool layerFound = false;

        // For each supported layer
        for (const auto& layerProp : availableLayers) {
            // If the supported layer ahs the same name as the required layer
            if (strcmp(layer, layerProp.layerName) == 0) {
                // required layer has been found
                layerFound = true;
                // stop searching for the required layer
                break;
            }
        }

        // If the required layer is never found
        if (!layerFound) return false;
    }
    return true;
}

// Create a framebuffer for each swapchain image 
void DeferredRenderer::createFramebuffers() {
    // Resize the vector to hold one frame buffer per swapchain images
    swapChainFramebuffers.resize(swapChainImageViews.size());

    // Per swapchain image
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        // Specify the attachment for the current framebuffer
        std::array<VkImageView, 2> attachments = {
            swapChainImageViews[i],
            depthImageView
        };

        // Creation info struct for framebuffer
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        // Specify the renderpass the framebuffer needs to be compatible with
        framebufferInfo.renderPass = renderPass;
        // Specify attachment count and references to said attachment(s)
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        // Specify the size of the framebuffer
        // Must match the swapchain images
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        // Specify the number of layers in the framebuffer
        // Again should match the swapchain images
        framebufferInfo.layers = 1;

        // Attempt to create the current framebuffer
        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            // If VK_SUCCESS not returned then throw an error to halt eecution
            throw std::runtime_error("failed to create framebuffer!");
        }
    }


}

// Create a framebuffer for each swapchain image 
void DeferredRenderer::createGeoFramebuffers() {

    geoFrameBuffer.width = 2048;
    geoFrameBuffer.height = 2048;

    createGeoAttachments();

    // Specify the attachment for the current framebuffer
    std::array<VkImageView, 4> attachments = {
        geoFrameBuffer.col.view,
        geoFrameBuffer.normal.view,
        geoFrameBuffer.spec.view,
        geoFrameBuffer.depth.view
    };

    // Create the geo pass render pass
    std::array<VkAttachmentDescription, 4> attachmentDescs = {};


    // Init attachment properties
    for (int i = 0; i < 4; ++i)
    {
        attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // Depth attachment will be index 3
        if (i == 3)
        {
            attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        else
        {
            attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    }

    attachmentDescs[0].format = geoFrameBuffer.col.format;
    attachmentDescs[1].format = geoFrameBuffer.normal.format;
    attachmentDescs[2].format = geoFrameBuffer.spec.format;
    attachmentDescs[3].format = geoFrameBuffer.depth.format;

    // Create references for the attachments
    std::array<VkAttachmentReference, 3> colAttReferences = {
        VkAttachmentReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
        VkAttachmentReference{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
        VkAttachmentReference{ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
    };
    VkAttachmentReference depthReference{ 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    // Create the subpass
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pColorAttachments = colAttReferences.data();
    subpass.colorAttachmentCount = static_cast<uint32_t>(colAttReferences.size());
    subpass.pDepthStencilAttachment = &depthReference;

    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT; // Wait for the memory to be read before continuing
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Create the render pass itself
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pAttachments = attachmentDescs.data();
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &geoFrameBuffer.renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create geometric render pass!");
    }

    // Creation info struct for framebuffer
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    // Specify the renderpass the framebuffer needs to be compatible with
    framebufferInfo.renderPass = geoFrameBuffer.renderPass;
    // Specify attachment count and references to said attachment(s)
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    // Specify the size of the framebuffer
    // Must match the swapchain images
    framebufferInfo.width = geoFrameBuffer.width;
    framebufferInfo.height = geoFrameBuffer.height;
    // Specify the number of layers in the framebuffer
    // Again should match the swapchain images
    framebufferInfo.layers = 1;

    // Attempt to create the current framebuffer
    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &geoFrameBuffer.frameBuffer) != VK_SUCCESS) {
        // If VK_SUCCESS not returned then throw an error to halt eecution
        throw std::runtime_error("failed to create framebuffer!");
    }

    // Create the color attachment sampler
    VkSamplerCreateInfo sampler{};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // Use nearest neightbor filtering (Mapping from color attachment to output pixel is 1:1 so this is irrelivant
    sampler.magFilter = VK_FILTER_NEAREST;
    sampler.minFilter = VK_FILTER_NEAREST;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeV = sampler.addressModeU;
    sampler.addressModeW = sampler.addressModeU;
    sampler.mipLodBias = 0.0f;
    sampler.maxAnisotropy = 1.0f;
    sampler.minLod = 0.0f;
    sampler.maxLod = 1.0f;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    // Create the sampler itself
    if (vkCreateSampler(device, &sampler, nullptr, &colAttachmentSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create color attachment sampler!");
    }

}

void DeferredRenderer::createGeoAttachments() {
    createAttachment(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &geoFrameBuffer.normal);
    createAttachment(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &geoFrameBuffer.col);
    createAttachment(findDepthFormat(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &geoFrameBuffer.depth);
    createAttachment(VK_FORMAT_R16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &geoFrameBuffer.spec);
}

void DeferredRenderer::createGeoCMDBuffer() {
    // Command buffer for geo rendering
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    // Specify the command pool the buffers will belong to
    allocInfo.commandPool = commandPool;
    // Specify the type of command buffers
    // Primary are buffers that can be executed but not called by other buffers
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // Specify how many command buffers we want to create
    allocInfo.commandBufferCount = 1;

    // Attempt to create all the required command buffers
    if (vkAllocateCommandBuffers(device, &allocInfo, &geoCMDBuffer) != VK_SUCCESS) {
        // If the comand buffer creation is unsuccessful throw an error to halt execution
        throw std::runtime_error("failed to allocate geo command buffers!");
    }

    // Semaphore for geo rendering
    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &geoSemaphore) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate geo semaphore!");
    }

    // Record the command Buffer
    VkCommandBufferBeginInfo cmdBufInfo{};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    // Clear values for all attachments written in the fragment shader
    std::array<VkClearValue, 5> clearVals;
    clearVals[0].color = {{0.f,0.f,0.f,0.f}};
    clearVals[1].color = {{0.f,0.f,0.f,0.f}};
    clearVals[2].color = { {0.f,0.f,0.0f,0.0f} };
    clearVals[3].color = { {0.f,0.f,0.0f,0.0f} };
    clearVals[4].depthStencil = {1.f,0};

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = geoFrameBuffer.renderPass;
    renderPassBeginInfo.framebuffer = geoFrameBuffer.frameBuffer;
    renderPassBeginInfo.renderArea.extent.width = geoFrameBuffer.width;
    renderPassBeginInfo.renderArea.extent.height = geoFrameBuffer.height;
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearVals.size());
    renderPassBeginInfo.pClearValues = clearVals.data();

    if (vkBeginCommandBuffer(geoCMDBuffer, &cmdBufInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate geo semaphore!");
    }

    vkCmdBeginRenderPass(geoCMDBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)geoFrameBuffer.width;
    viewport.height = (float)geoFrameBuffer.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(geoCMDBuffer, 0, 1, &viewport);

    // Scissor defines what section of the framebuffer should be stored
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent.width = (float)geoFrameBuffer.width;
    scissor.extent.height = (float)geoFrameBuffer.height;
    vkCmdSetScissor(geoCMDBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(geoCMDBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, geoPipeline);

    // bind the uniform buffer for the current swapchain image
    vkCmdBindDescriptorSets(geoCMDBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &geoDescriptorSet, 0, nullptr);

    // Select the set of buffers and offsets to bind to the pipeline
    VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    // Bind the vertex buffer
    vkCmdBindVertexBuffers(geoCMDBuffer, 0, 1, vertexBuffers, offsets);

    // Bind the index buffer with no offset, out index buffer is 16 bit unsigned ints
    vkCmdBindIndexBuffer(geoCMDBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // Draw the vertices passed in the buffer, with indexing from the index buffer, one instance
    vkCmdDrawIndexed(geoCMDBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

    vkCmdEndRenderPass(geoCMDBuffer);

    if (vkEndCommandBuffer(geoCMDBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to end command buffer!");
    }
}

void DeferredRenderer::createAttachment(VkFormat format, VkImageUsageFlagBits usage, FBAttachment* attachment)
{
    VkImageAspectFlags aspectMask = 0;
    VkImageLayout imageLayout;

    attachment->format = format;

    if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    assert(aspectMask > 0);

    VkImageCreateInfo image{};
    image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = format;
    image.extent.width = geoFrameBuffer.width;
    image.extent.height = geoFrameBuffer.height;
    image.extent.depth = 1;
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;

    VkMemoryAllocateInfo memAlloc{};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memReqs;

    if (vkCreateImage(device, &image, nullptr, &attachment->image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create Geo Pass Image!");
    }
    vkGetImageMemoryRequirements(device, attachment->image, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(device, &memAlloc, nullptr, &attachment->mem) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate Geo Pass Image!");
    }
    if (vkBindImageMemory(device, attachment->image, attachment->mem, 0) != VK_SUCCESS) {
        throw std::runtime_error("failed to bind Geo Pass Image!");
    }

    attachment->view = createImageView(attachment->image, format, aspectMask);
}

// Function to create a command pool
void DeferredRenderer::createCommandPool(VkCommandPool* cPool, VkCommandPoolCreateFlags flags) {
    // Get the queue family indices from the device bieng used
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    // Info struct for command pool creation
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    // Pass the index of the graphics queue family
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    // Flags are for special functionality within the command pool that we do not need right now
    poolInfo.flags = flags; // Optional

    // Attempt command pool creation
    if (vkCreateCommandPool(device, &poolInfo, nullptr, cPool) != VK_SUCCESS) {
        // throw error if creation is not successful
        throw std::runtime_error("failed to create command pool!");
    }
}

// Create the command buffers and record the commands to draw the object to framebuffer using the graphics pipeline
void DeferredRenderer::createCommandBuffers() {
    // Resize the vector to hold one command buffer per framebuffer
    commandBuffers.resize(swapChainFramebuffers.size());

    // Command buffer allocation information struct
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    // Specify the command pool the buffers will belong to
    allocInfo.commandPool = commandPool;
    // Specify the type of command buffers
    // Primary are buffers that can be executed but not called by other buffers
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // Specify how many command buffers we want to create
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    // Attempt to create all the required command buffers
    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        // If the comand buffer creation is unsuccessful throw an error to halt execution
        throw std::runtime_error("failed to allocate command buffers!");
    }

    // For each command buffer
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        // Info used for beginning a command buffer recording
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        // Used to specify special use cases for command buffers
        beginInfo.flags = 0; // Optional
        // Only used in secondary buffers
        beginInfo.pInheritanceInfo = nullptr; // Optional

        // Attempt to begin recording to the command buffer
        // This will reset the command buffer, it is not possible to append an already recorded to buffer
        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            // If unsuccessful throw an error to halt execution
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        // Info for beginning the render pass
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        // Specify the render pass
        renderPassInfo.renderPass = renderPass;
        // Specify the attachment to bind
        renderPassInfo.framebuffer = swapChainFramebuffers[i];
        // Specify the area of the buffer to be rendered to
        renderPassInfo.renderArea.offset = { 0, 0 };
        // MAtching attachment size gives the best performance
        renderPassInfo.renderArea.extent = swapChainExtent;

        // Specify the clear color for the color and depth attachments
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };
        // Only one clear color is specified
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        // Record the command to begin the render pass for the current command buffer
        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffers[i], 0, 1, &viewport);

        // Scissor defines what section of the framebuffer should be stored
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffers[i], 0, 1, &scissor);

        // bind the graphics pipeline
        // VK_PIPELINE_BIND_POINT_GRAPHICS specifies a graphics pipelines, rather than compute
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, finalPipeline);


        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &finalDescriptorSet, 0, nullptr);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, finalPipeline);
        // Draw the entire screen
        // Each fragment in the frag shader in will reference the g-buffer
        vkCmdDraw(commandBuffers[i], 4, 1, 0, 0);

        // Signifies the end of the render pass
        vkCmdEndRenderPass(commandBuffers[i]);

        // End command buffer recording
        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            // If not successful throw an error
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

// Create the command buffers used specifically for ImGui
void DeferredRenderer::createImGuiCommandBuffers(VkCommandBuffer* _commandBuffer, uint32_t _commandBufferCount, VkCommandPool& _commandPool) {
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandPool = _commandPool;
    commandBufferAllocateInfo.commandBufferCount = _commandBufferCount;
    vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, _commandBuffer);
}

// General function to create a buffer and allocate the required memory on GPU, then bind the two
// The buffer and memory pointers are returned to the buffer, and bufferMemory arguments
void DeferredRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    // Info struct for buffer creation
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    // Specify the size of the buffer in bytes
    // Make it big enough for all of the data in the vertex array
    bufferInfo.size = size;
    // Specify that the buffer is to be used as a vertex buffer
    bufferInfo.usage = usage;
    // Since we only have one graphics queue so we can use exclusive mode 
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Attempt buffer creation
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        // if unsuccessfult throw a fatal error
        throw std::runtime_error("failed to create vertex buffer!");
    }

    // Structure for storing the requirements in memory of a buffer
    VkMemoryRequirements memRequirements;
    // Fill the struct with the requirements of the vertex buffer
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    // Info struct for allocating memory
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    // Specify how much memory is needed to be allocated
    allocInfo.allocationSize = memRequirements.size;
    // Specify the type index of the memory to assign to
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    // Attempt to allocate the memory for the buffer
    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        // If not successful, throw a fatal error
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    // Bind the buffer to the memory we allocated for it
    // The last param is the offset in the memory to bind the buffer, since the memory is only for the buffer we offset it by 0
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

// Use the tinyObj library to load an obj file from disk, and sotre the vertices, with normals and texture coordinates, in an indexed form
void DeferredRenderer::loadModel() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    // Use timy obj to load the object from file
    // Load obj will automaticall triangulate faces by default
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
        // Failure is fatal
        throw std::runtime_error(warn + err);
    }

    // For each shape in the file
    for (const auto& shape : shapes) {
        // For each index in the shape mesh
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            // Get the position of the vertex
            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            // get the texture coordinates of the vertex
            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]
            };

            // Set the color to white, as we are texturing anyway
            vertex.color = { 1.0f, 1.0f, 1.0f };

            auto iter = std::find(vertices.begin(), vertices.end(), vertex);

            // If the vertex is not in the vector already
            if (iter == vertices.end()) {
                // Add it to the vector
                vertices.emplace_back(vertex);
                indices.push_back(vertices.size() - 1);
            }
            else {
                // Push the index of the vertex to the indices array
                indices.push_back(iter - vertices.begin());
            }

        }

    }
}

// Create the vertex buffer on GPU local memory
// This requires creating a stagin buffer that is CPU accessible first, then using a command queue to copy from the staging buffer to the GPU local buffer
void DeferredRenderer::createVertexBuffer() {
    // Create a struct to hold the size of the buffer required
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    // Create a host accessible staging buffer
    // This will act as a host accessible passthrough to the gpu local memory
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    // The staging buffer will be used as the source for data transfer, it will be host visible and will be host coherent
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    // Create a data array in CPU memory that we map to the buffer in gpu memory
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    // Copy the data of the vertex array to the CPU memory that we mapped
    memcpy(data, vertices.data(), (size_t)bufferSize);
    // Unmap the cpu memory from gpu
    vkUnmapMemory(device, stagingBufferMemory);
    // NOTE: since we allocated memory in a host coherent memory type, we know the drivers are aware of the change to the data immediately
    // The specification is such that we know the memory will be transfered to GPU by the next call to vkQueueSumbit

    // Create the vertex buffer on gpu that is used for both: the destination of a data transfer, and as a vertex buffer. The only qualification is that the buffer must be in device local memory for performance
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

    // Copy the data from the staging buffer to the vertex buffer
    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    // Clean up the staging buffer and its memory
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

// Identical to createVertexBuffer but for the index buffer
void DeferredRenderer::createIndexBuffer() {
    // Create a struct to hold the size of the buffer required
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    // Create a host accessible staging buffer
    // This will act as a host accessible passthrough to the gpu local memory
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    // The staging buffer will be used as the source for data transfer, it will be host visible and will be host coherent
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    // Create a data array in CPU memory that we map to the buffer in gpu memory
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    // Copy the data of the vertex array to the CPU memory that we mapped
    memcpy(data, indices.data(), (size_t)bufferSize);
    // Unmap the cpu memory from gpu
    vkUnmapMemory(device, stagingBufferMemory);
    // NOTE: since we allocated memory in a host coherent memory type, we know the drivers are aware of the change to the data immediately
    // The specification is such that we know the memory will be transfered to GPU by the next call to vkQueueSumbit

    // Create the vertex buffer on gpu that is used for both: the destination of a data transfer, and as an index buffer. The only qualification is that the buffer must be in device local memory for performance
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    // Copy the data from the staging buffer to the vertex buffer
    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    // Clean up the staging buffer and its memory
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

// Create a CPU accessible uniform buffer on the GPU for passing uniform data to the shaders
void DeferredRenderer::createUniformBuffers() {
    VkDeviceSize geoVUBOSize = sizeof(geoVertUBO);
    VkDeviceSize finalFUBOSize = sizeof(finalFragUBO);

    createBuffer(geoVUBOSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, geoVertUBOBuffer, geoVertUBOBufferMemory);
    createBuffer(finalFUBOSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, finalFragUBOBuffer, finalFragUBOBufferMemory);
}

// Simply function to copy the contents of one buffer on GPU to another
void DeferredRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    // Create and begin recording to a temporary command buffer
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    // Queue a copy command from source to destination buffers
    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    // End the recording and run the command buffer, in turn freeing the command buffer
    endSingleTimeCommands(commandBuffer);

}

// Find the index of the memory type on GPU that matches the filter and properties specified
uint32_t DeferredRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    // Get information about the memory on the physical device we are using
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    // Check each memory type against a the bit field in typeFilter
    // Only return a memorytype if it matched the typeFilter and has the properties (also a bit field) that we require
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    // If the function hasn't returned yet there is no available memory that we can use so throw a fatal error
    throw std::runtime_error("failed to find suitable memory type!");
}

// Create a descriptor set layouts for the descriptors
void DeferredRenderer::createDiscriptorSetLayout() {
    // UBO for vertex shader (Final Pass)
    VkDescriptorSetLayoutBinding vUBOLayoutBinding{};
    vUBOLayoutBinding.binding = 0;
    vUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vUBOLayoutBinding.descriptorCount = 1;
    vUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // Color texture target
    VkDescriptorSetLayoutBinding colLayoutBinding{};
    colLayoutBinding.binding = 1;
    colLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    colLayoutBinding.descriptorCount = 1;
    colLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding normalLayoutBinding{};
    normalLayoutBinding.binding = 2;
    normalLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    normalLayoutBinding.descriptorCount = 1;
    normalLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding specLayoutBinding{};
    specLayoutBinding.binding = 3;
    specLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    specLayoutBinding.descriptorCount = 1;
    specLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding depthLayoutBinding{};
    depthLayoutBinding.binding = 4;
    depthLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    depthLayoutBinding.descriptorCount = 1;
    depthLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // UBO for fragment shader (Geo Pass)
    VkDescriptorSetLayoutBinding fUBOLayoutBinding{};
    fUBOLayoutBinding.binding = 5;
    fUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    fUBOLayoutBinding.descriptorCount = 1;
    fUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


    // Descriptor set binding info struct for the texture sampler
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 6;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Pack the bindings into an array
    std::array<VkDescriptorSetLayoutBinding, 7> bindings = { 
        vUBOLayoutBinding,
        colLayoutBinding,
        normalLayoutBinding,
        specLayoutBinding,
        depthLayoutBinding,
        fUBOLayoutBinding,
        samplerLayoutBinding
    };

    // Descriptor set creation configuration struct
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    // Specify the number, and an array containing, the descriptor sets to create
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    // Attempt to create the descriptor sets
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    // Info struct for pipeline layout creation
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    // Attempt to create the pipeline layout
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

}

#pragma region OldDescriptorSets

// Create the descriptor sets and allocate the memory required
/*
void DeferredRenderer::createDescriptorSets() {
    // Create a vector of the descriptorSetLayout structs we defined earlier, one per swap chain image
    std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);

    // Struct for allocating descriptor sets
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    // Specify the pool to allocate from
    allocInfo.descriptorPool = descriptorPool;
    // We will be allocating one set per swap chain image
    allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
    // PAss the layouts to be the layouts we defined earlier
    allocInfo.pSetLayouts = layouts.data();

    // Attempt to allocate the descriptor sets for each swap chain image
    descriptorSets.resize(swapChainImages.size());
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        // Failure is fatal so throw an error
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    // Populate the descriptors we just allocated
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        // Info struct for descriptor buffers
        VkDescriptorBufferInfo bufferInfo{};
        // Specify the buffer that the descriptor is associated with
        bufferInfo.buffer = uniformBuffers[i];
        // We want to read from the start of the buffer
        bufferInfo.offset = 0;
        // We want to read the data from a uniform buffer object
        bufferInfo.range = sizeof(UniformBufferObject);

        // Info for the image sampler descriptor
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        // Pass the image viewer to be sampled
        imageInfo.imageView = textureImageView;
        // Pass the sampler to be used
        imageInfo.sampler = textureSampler;

        // Struct for writing a descriptor set
        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        // UBO descriptor
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // Descriptor set to write
        descriptorWrites[0].dstSet = descriptorSets[i];
        // Binding within the shader to update
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        // Specify we are updating one uniform descriptor
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        // Specify the info of the description buffer
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        descriptorWrites[0].pImageInfo = nullptr; // Optional
        descriptorWrites[0].pTexelBufferView = nullptr; // Optional

        // Image Sampler descriptor
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        // Bind to 1 in the shader
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        // Specify we are binding one image sampler
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        // Specify the description of the image
        descriptorWrites[1].pImageInfo = &imageInfo;

        // Update the descriptor set
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}
*/

#pragma endregion OldDescriptorSets

// Create the descriptor sets and allocate the memory required
void DeferredRenderer::createDescriptorSets() {
    std::vector<VkWriteDescriptorSet> writeSets;

    // <RENDERPASS 2>
    VkDescriptorSetAllocateInfo descAllocInfo{};
    descAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descAllocInfo.descriptorPool = descriptorPool;
    descAllocInfo.pSetLayouts = &descriptorSetLayout;
    descAllocInfo.descriptorSetCount = 1;

    VkDescriptorImageInfo colTexDescriptor{};
    colTexDescriptor.sampler = colAttachmentSampler;
    colTexDescriptor.imageView = geoFrameBuffer.col.view;
    colTexDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorImageInfo normalTexDescriptor{};
    normalTexDescriptor.sampler = colAttachmentSampler;
    normalTexDescriptor.imageView = geoFrameBuffer.normal.view;
    normalTexDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorImageInfo specTexDescriptor{};
    specTexDescriptor.sampler = colAttachmentSampler;
    specTexDescriptor.imageView = geoFrameBuffer.spec.view;
    specTexDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorImageInfo depthTexDescriptor{};
    depthTexDescriptor.sampler = colAttachmentSampler;
    depthTexDescriptor.imageView = geoFrameBuffer.spec.view;
    depthTexDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorBufferInfo finalFUBODescriptor{};
    finalFUBODescriptor.offset = 0;
    finalFUBODescriptor.range = VK_WHOLE_SIZE;
    finalFUBODescriptor.buffer = finalFragUBOBuffer;

    ASSERT_VK_SUCCESS(vkAllocateDescriptorSets(device, &descAllocInfo, &finalDescriptorSet));

    VkWriteDescriptorSet colWriteDescriptorSet{};
    colWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    colWriteDescriptorSet.dstSet = finalDescriptorSet;
    colWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    colWriteDescriptorSet.dstBinding = 1;
    colWriteDescriptorSet.pImageInfo = &colTexDescriptor;
    colWriteDescriptorSet.descriptorCount = 1;

    VkWriteDescriptorSet normalWriteDescriptorSet{};
    normalWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    normalWriteDescriptorSet.dstSet = finalDescriptorSet;
    normalWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    normalWriteDescriptorSet.dstBinding = 2;
    normalWriteDescriptorSet.pImageInfo = &normalTexDescriptor;
    normalWriteDescriptorSet.descriptorCount = 1;

    VkWriteDescriptorSet specWriteDescriptorSet{};
    specWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    specWriteDescriptorSet.dstSet = finalDescriptorSet;
    specWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    specWriteDescriptorSet.dstBinding = 3;
    specWriteDescriptorSet.pImageInfo = &specTexDescriptor;
    specWriteDescriptorSet.descriptorCount = 1;

    VkWriteDescriptorSet depthWriteDescriptorSet{};
    depthWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    depthWriteDescriptorSet.dstSet = finalDescriptorSet;
    depthWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    depthWriteDescriptorSet.dstBinding = 4;
    depthWriteDescriptorSet.pImageInfo = &depthTexDescriptor;
    depthWriteDescriptorSet.descriptorCount = 1;

    VkWriteDescriptorSet fUBOWriteDescriptorSet{};
    fUBOWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    fUBOWriteDescriptorSet.dstSet = finalDescriptorSet;
    fUBOWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    fUBOWriteDescriptorSet.dstBinding = 5;
    fUBOWriteDescriptorSet.pBufferInfo = &finalFUBODescriptor;
    fUBOWriteDescriptorSet.descriptorCount = 1;

    writeSets = {
        colWriteDescriptorSet,
        normalWriteDescriptorSet,
        specWriteDescriptorSet,
        depthWriteDescriptorSet,
        fUBOWriteDescriptorSet
    };

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
    // <!RENDERPASS 2>


    // <RENDERPASS 1>

    VkDescriptorImageInfo modelTexDescriptor{};
    modelTexDescriptor.sampler = textureSampler;
    modelTexDescriptor.imageView = textureImageView;
    modelTexDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkDescriptorBufferInfo geoVUBODescriptor{};
    geoVUBODescriptor.offset = 0;
    geoVUBODescriptor.range = VK_WHOLE_SIZE;
    geoVUBODescriptor.buffer = geoVertUBOBuffer;

    ASSERT_VK_SUCCESS(vkAllocateDescriptorSets(device, &descAllocInfo, &geoDescriptorSet));

    VkWriteDescriptorSet gUBOWriteDescriptorSet{};
    gUBOWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    gUBOWriteDescriptorSet.dstSet = geoDescriptorSet;
    gUBOWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    gUBOWriteDescriptorSet.dstBinding = 0;
    gUBOWriteDescriptorSet.pBufferInfo = &geoVUBODescriptor;
    gUBOWriteDescriptorSet.descriptorCount = 1;

    VkWriteDescriptorSet modelColorWriteDescriptorSet{};
    modelColorWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    modelColorWriteDescriptorSet.dstSet = geoDescriptorSet;
    modelColorWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    modelColorWriteDescriptorSet.dstBinding = 6;
    modelColorWriteDescriptorSet.pImageInfo = &modelTexDescriptor;
    modelColorWriteDescriptorSet.descriptorCount = 1;

    writeSets = {
        gUBOWriteDescriptorSet,
        modelColorWriteDescriptorSet
    };

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
}

// Create the descriptor pools
void DeferredRenderer::createDescriptorPool() {


    VkDescriptorPoolSize poolSizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        //{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(swapChainImages.size()) },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(swapChainImages.size()) + 1001 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(swapChainImages.size()) +1001 },
        //{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    // Pool creation configuration struct
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    // Specify how many, and the array containing the, descriptor pool sizes being created
    poolInfo.poolSizeCount = static_cast<uint32_t>(sizeof(poolSizes) / sizeof(poolSizes[0]));
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size()) + 1000;

    // Attempt descriptor pool creation
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        // Failure is fatal so throw an error
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

// Load the texture image from disk, then pass the image data to the GPU for use in the shader
void DeferredRenderer::createTextureImage() {
    // Load the image using the stb_image lobrary
    // We want a four channel image, starting at the pointer pixels
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    // If pixels is null then image failed to load
    if (!pixels) {
        // Failure is fatal so throw an error
        throw std::runtime_error("failed to load texture image!");
    }

    // Create a temporary buffer for passing the texture to the gpu
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    // This buffe rmust be host visible
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    // pass the image data to the staging buffer on GPU
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    // Now we can free the memory of the texture
    stbi_image_free(pixels);

    // USe the image creation function to create the texture image with the desired properties
    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

    // Transition the texture to be a transfer destination optimal layout
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy from the staging buffer to the texture on gpu memory
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    // Transition the texture to be optimal for reading from a shader
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Clean up the staging buffer and memory
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

// Create the image view from the texture image
void DeferredRenderer::createTextureImageView() {
    // Create an image veiw for the texture that is 4-channel srgb
    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

// General function to create an image view from an image with the format and aspect flags, specified
VkImageView DeferredRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    // Image view creation struct

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    // Specify the image format
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    // Attempt image view creation

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        // Failure is fatal, throw error
        throw std::runtime_error("failed to create texture image view!");
    }

    // Return the image view created
    return imageView;
}

// Genberal function to create an image with the attributes specified
// The function creates a VkImage and allocated the memeory required, then bind the two
void DeferredRenderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {

    // struct for creating an image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    // We want to make a 2d image
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    // Specify the size of the image, the same as the texture we loaded
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    // The texture is 2d so has only a depth of 1
    imageInfo.extent.depth = 1;
    // No extra mipmapping or array layers
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;

    // Since we read the file with four channels, we want the image to be a four channel image too
    imageInfo.format = format;
    // We want the image to be laid out in the most optimal way for reading
    imageInfo.tiling = tiling;
    // We dont care about the initial layout of the image as we dont need any data to be preserved until we transition the image
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // Specify that we will use the image as a transfer destination (From the staging buffer) and a image that is sampled by the shader
    imageInfo.usage = usage;
    // We have one wueue family so exlcusive mode is the efficient choice
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // No multisampling so only one sample needed
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    // Some flags, like sparse imaged can be used to improve performance in certain scenarios
    imageInfo.flags = 0; // Optional

    // Attempt image creation
    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        // Failure is fatal so throw an error
        throw std::runtime_error("failed to create image!");
    }

    // Query the requirements in memory for the image
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    // Info struct for allocating memory
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    // Specify the memory and the memory type index to store one, this memory should have the properties passed as a parameter
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    // Attempt to allocate the images memory
    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        // Failure is fatal
        throw std::runtime_error("failed to allocate image memory!");
    }

    // Bind the image to teh image memory we allocated
    vkBindImageMemory(device, image, imageMemory, 0);
}

// Use a barrier to transition an image with the specified format from one layout to another
// This is done using a temporary command buffer
void DeferredRenderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    // Create a temporary command buffer and begin recording
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    // Create an image barrier for transitioning the image
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    // Specify the old and new layouts in the transition
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    // Used for transition of queue family ownership, not needed for layout transition
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    // Specify the image to transition
    barrier.image = image;
    // Specify the image properties
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    // Structs for storing state flags for the transition
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    // If we are going from undefined to a trnsfer detination layout
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        // Specify in the barrier that this is the transition
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        // The transition does not need to wait for anything to happen
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        // Specify the transition states
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        // The transition must wait for the transfer stage, so that the image has been written to before transitioning
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        // The destination is the fragment shader as this is where the image is read
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        // If neither are the case then we do not know how to handle the specified transition
        // This is fatal and should halt execution through an error throw
        throw std::invalid_argument("unsupported layout transition!");
    }

    // Transition the image
    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    // End recording, sumbit and then free the temporary command buffer
    endSingleTimeCommands(commandBuffer);
}

// Create a texture sampler for use in the shader to query teture
void DeferredRenderer::createTextureSampler() {
    // Sampler creation configuration struct
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // We want to use linear interpolation for over and undersampling
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    // If we attempt to sample from outside of the image, just repeat the image
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    // GEt the propertied of the device in use
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    // Enable anisotropic filetering for undersampled textures
    samplerInfo.anisotropyEnable = VK_TRUE;
    // Use the best level of anisotrophy possible for the current device
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    // Uneeded as we use repead address mode but good to include in case that changes
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    // Setting this to false allows us to sample the texture (u,v) in the range [0,1] reglardless of texture size
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    // Disable texel comparison
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    // Configure mipmapping to be disabled
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    // Attempt sampler creation
    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        // Failure is fatal, throw error
        throw std::runtime_error("failed to create texture sampler!");
    }
}

// Copy the contents fo a buffer to an image image buffer on GPU
void DeferredRenderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    // Begin temp command buffer
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    // Specify the region of the buffer to copy from
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    // There is no padding in the buffer
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    // Describe the image we are copying to
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    // Describe where in the image to copy to
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        width,
        height,
        1
    };

    // Queue the command to copy the buffer contents to image
    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    // Execute temp command buffer
    endSingleTimeCommands(commandBuffer);
}

// Create a temporary command buffer for single use and return it
// This will be called, then the commands to record, then endSingleTimeCommands() to execute the commands
VkCommandBuffer DeferredRenderer::beginSingleTimeCommands() {
    // Create a temporary primary command buffer within commandPool
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    // Allocate the temporary command buffer
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    // Begin recording to the command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

// Execute the commands recorded to the command buffer and then destroy the command buffer
void DeferredRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    // End recording to the command buffer passed
    vkEndCommandBuffer(commandBuffer);

    // Setup submission for the command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // Submit the commadn buffer to teh graphics queue and wait for it to finish
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    // Free the temporary command buffer
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

// Create an image and image view for use as a depth buffer in the pipeline
void DeferredRenderer::createDepthResources() {
    // Get the depth format that the GPU supports
    VkFormat depthFormat = findDepthFormat();

    // Create teh depth buffer image with the format we found earlier, and specify it as a depth stencil attachment in device local memory
    createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    // Create an image view from the image that is a depth image
    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

// Find the formats that are supported by the physical device and return them
VkFormat DeferredRenderer::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    // For each format in the candidates vector
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        // Check if the linear tiling mode of the current candidate has the features requested
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        // Check if the optimal tiling mode of the current candidate has the features requested
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    // If no return yet then the device does not support any of the candidates, this is fatal so we throw an error
    throw std::runtime_error("failed to find supported format!");
}

// Finda and return the format for the depth buffer
VkFormat DeferredRenderer::findDepthFormat() {
    // Find the format for depth buffering that the gpu supports
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

// Check for whether a format has a stencil component
bool DeferredRenderer::hasStencilComponent(VkFormat format) {
    // Return true if the format has a stencil component
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

// Function to create the sephamores and fences used to synchronise the vulkan app
void DeferredRenderer::createSyncObjects() {

    imageAvailableSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
    // Also done for fenses that facilitate GPU-CPU synchronisation
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    // Resize to hold a fence per swapchain image, but fill it with NULL handles
    imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

    // Sephamore creation info struct
    // Empty in the current api version
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Similat ro sephamore creation info but for fences
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // Set the fences to be signalled on creation
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // For each concurrent frame
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Attempt to create the two sephamores and one fence for that frame
        ASSERT_VK_SUCCESS(vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]));
        ASSERT_VK_SUCCESS(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore[i]));
        ASSERT_VK_SUCCESS(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore[i]));
    }
}

// Function that contains all of the per-frame functionality
void DeferredRenderer::mainLoop() {
    // This loop will run until the window is closed by the user
    // Either programatically, or through alt+f4 or pressing the window's 'x'
    while (!glfwWindowShouldClose(window)) {
        // Handle all of the events raised since the last call to glfwPollEvents
        // Call the appropriate callbacks ect.
        glfwPollEvents();
        // ImGui needs to be drawn before it can be rendered in the drawFrame function
        drawImGui();
        drawFrame();
    }

    // Wait for the device to finish all queued instructions before cleaning up assets
    vkDeviceWaitIdle(device);
}

// Draw a frame every itteration of the main loop
void DeferredRenderer::drawFrame() {
    //// Wait for the fence of the current frame to be signalled
    //// This prevents the same frame from being rendered simulatiouasly
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    //// Get the index of the next image to be drawn to screen, using the sephamore to wait till it is available
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore[currentFrame], VK_NULL_HANDLE, &imageIndex);

    // If the swapchain is unusable due to a change to the window
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // Recreate the swap chain
        recreateSwapChain();
        return;
    }
    //// If any other non-success value is returned then throw an error to halt execution
    ASSERT_VK_SUCCESS(result);

    // Update the uniform buffers 
    updateUniformBuffers();

    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being in use by this frame
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    // ImGui rendering function
    {
        // Reset the command pool
        auto err = vkResetCommandPool(device, imGuiCommandPools[imageIndex], 0);
        if (err != VK_SUCCESS) throw std::runtime_error("Could not reset the command pool!");

        VkCommandBufferBeginInfo commBufferInfo = {};
        commBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commBufferInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        // Begin recording to the command buffer for the current frame
        err = vkBeginCommandBuffer(imGuiCommandBuffers[imageIndex], &commBufferInfo);
        if (err != VK_SUCCESS) throw std::runtime_error("Could not reset the command pool!");

        // Begin the renderpass
        VkRenderPassBeginInfo rpInfo = {};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.renderPass = imGuiRenderPass;
        rpInfo.framebuffer = imGuiFrameBuffers[imageIndex]; 
        rpInfo.renderArea.extent = swapChainExtent;
        rpInfo.clearValueCount = 1;
        VkClearValue imGuiClearVal = { 0.,0.,0.,1. };
        rpInfo.pClearValues = &imGuiClearVal;
        vkCmdBeginRenderPass(imGuiCommandBuffers[imageIndex], &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Record Imgui Draw Data and draw funcs into command buffer
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), imGuiCommandBuffers[imageIndex]);

        // End the render pass
        vkCmdEndRenderPass(imGuiCommandBuffers[imageIndex]);

        // End the command buffer recording
        err = vkEndCommandBuffer(imGuiCommandBuffers[imageIndex]);
        if (err != VK_SUCCESS) throw std::runtime_error("Failed to submit ImGui command buffer!");

    }

    // <G-Pass Queue Submit>

    VkSubmitInfo submitInfo{};
    VkSemaphore waitSemaphores[] = { imageAvailableSemaphore[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore geoSemaphores[] = { geoSemaphore };
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = geoSemaphores;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &geoCMDBuffer;


    ASSERT_VK_SUCCESS(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));

    // </G-Pass Queue Submit>


    // <Final Pass + ImGui Queue Submit>

    std::array<VkCommandBuffer, 2> submissionBuffers = { commandBuffers[imageIndex], imGuiCommandBuffers[imageIndex] };
    VkSemaphore signalSems[] = { renderFinishedSemaphore[currentFrame] };
    submitInfo.pWaitSemaphores = geoSemaphores;
    submitInfo.pSignalSemaphores = signalSems;
    submitInfo.commandBufferCount = static_cast<uint32_t>(submissionBuffers.size());
    submitInfo.pCommandBuffers = submissionBuffers.data();

    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    
    ASSERT_VK_SUCCESS(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]));

    // </Fanal Pass Queue Submit>

    // Config struct for presentation
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    // Specify the sephamores that the presentation must wait for
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSems;

    // Specify the swapchain that is to be presented to
    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    // Submit the request to the swap chain
    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    // If the swapchain is unusable due to a change to the window or if the swapchain is suboptimal for the current window
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        // Set resized variable back to false
        framebufferResized = false;
        // Recreate the swapchain
        recreateSwapChain();
    }
    // If any other non-success value is returned then throw an error to halt execution
    ASSERT_VK_SUCCESS(result);

    // Increment the current frame counter, reseting to 0 if it exceeds the max number of concurrent frames
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

// Function responsible for drawing the GUI
void DeferredRenderer::drawImGui() {
    // Begin the imgui commands
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ImGui io is used for mouse controls
    auto io = ImGui::GetIO();

    ImGui::Begin("Controls");
    if (ImGui::CollapsingHeader("Material Properties"))
    {
        ImGui::SliderFloat("Specular", &m_specScale, 0.f, 1.f);
        ImGui::SliderFloat("Diffuse", &m_diffScale, 0.f, 1.f);
        ImGui::SliderFloat("Ambient", &m_ambScale, 0.f, 1.f);
        ImGui::SliderFloat("Shininess", &m_shinScale, 1.f, 20.f);
        ImGui::SliderFloat("Emission", &m_emissScale, 0.f, 1.f);
    }

    if (ImGui::CollapsingHeader("Model/Light Transformation"))
    {
        ImGui::Button("Rotate Model");
        if (ImGui::IsItemActive())
        {
            m_rotation.x = m_rotation.x + io.MouseDelta.x;
            m_rotation.y = m_rotation.y + io.MouseDelta.y;
        }
        ImGui::SameLine();
        ImGui::Button("Translate Model");
        if (ImGui::IsItemActive())
        {
            m_translation.x = m_translation.x + io.MouseDelta.x;
            m_translation.y = m_translation.y + io.MouseDelta.y;
        }
        ImGui::Button("Rotate Light");
        if (ImGui::IsItemActive())
        {
            l_rotation.x = l_rotation.x + io.MouseDelta.x;
            l_rotation.y = l_rotation.y + io.MouseDelta.y;
        }
        ImGui::Button("Zoom Model");
        if (ImGui::IsItemActive())
        {
            m_zoom = m_zoom - (io.MouseDelta.y/100.f);
        }
    }

    if (ImGui::CollapsingHeader("Render Settings"))
    {
        ImGui::Checkbox("Enable Textures", &enableTextures);
        ImGui::Checkbox("Enable Lighting", &enableLighting);
    }
    ImGui::End();
    // Render the imgui commands internally
    ImGui::Render();

}

// Update the uniform buffer to be passed to the shaders
// Should be called every frame
void DeferredRenderer::updateUniformBuffers() {

    //// Uniform buffer struct that we will pass to the shader
    //UniformBufferObject ubo{};

    //// Material Properties
    //ubo.m_ambient = glm::vec4(1.f, 1.f, 1.f, 1.f) * m_ambScale;
    //ubo.m_diffuse = glm::vec4(1.f, 1.f, 1.f, 1.f) * m_diffScale;
    //ubo.m_specular = glm::vec4(1.f, 1.f, 1.f, 1.f) * m_specScale;
    //ubo.m_emission = glm::vec4(1.f, 1.f, 1.f, 1.f) * m_emissScale;
    //ubo.m_shininess = 1.f * m_shinScale;

    //ubo.lighting = enableLighting;
    //ubo.textures = enableTextures;

    //// Copy the uniform data to the buffer
    //void* data;
    //vkMapMemory(device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
    //memcpy(data, &ubo, sizeof(ubo));
    //vkUnmapMemory(device, uniformBuffersMemory[currentImage]);

    // <G-Pass Vertex Shader UBO>

    glm::mat4 modelTransform = glm::mat4(1.f);
    modelTransform = glm::translate(modelTransform, glm::vec3(m_translation.x, 0.f, -m_translation.y));
    modelTransform = glm::rotate(modelTransform, glm::radians(m_rotation.x), glm::vec3(0.0f, 0.0f, 1.0f));
    modelTransform = glm::rotate(modelTransform, glm::radians(m_rotation.y), glm::vec3(1.0f, 0.0f, 0.0f));
    modelTransform = glm::scale(modelTransform, glm::vec3(m_zoom, m_zoom, m_zoom));

    geoVertUBO.model = modelTransform;
    geoVertUBO.view = glm::lookAt(glm::vec3(0.f, -100.f, 50.f), glm::vec3(0.0f, 0.0f, 25.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    geoVertUBO.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 200.0f);
    geoVertUBO.proj[1][1] *= -1;

    void* data;
    vkMapMemory(device, geoVertUBOBufferMemory, 0, sizeof(geoVertUBO), 0, &data);
    memcpy(data, &geoVertUBO, sizeof(geoVertUBO));
    vkUnmapMemory(device, geoVertUBOBufferMemory);

    // </G-Pass Vertex Shader UBO>


    // <Final Pass Fragment Shader UBO>

    glm::mat4 l_transform = glm::mat4(1.f);
    l_transform = glm::rotate(l_transform, glm::radians(-90.f), glm::vec3(0.0f, 1.0f, 0.0f));
    l_transform = glm::rotate(l_transform, glm::radians(l_rotation.x), glm::vec3(0.0f, 1.0f, 0.0f));
    l_transform = glm::rotate(l_transform, glm::radians(l_rotation.y), glm::vec3(1.0f, 0.0f, 0.0f));

    finalFragUBO.lights[0].pos = l_transform * glm::vec4(0., 1., 0., 1.);
    finalFragUBO.lights[0].color = glm::vec4(1.f, 1.f, 1.f, 1.f);
    finalFragUBO.viewPos = glm::vec4(0.f, -100.f, 50.f, 1.f);
    finalFragUBO.showDebugBuffers = 1;

    vkMapMemory(device, finalFragUBOBufferMemory, 0, sizeof(finalFragUBO), 0, &data);
    memcpy(data, &finalFragUBO, sizeof(finalFragUBO));
    vkUnmapMemory(device, finalFragUBOBufferMemory);

    // </Final Pass Fragment Shader UBO>
}

// When the swapchain needs to be recreated due to a change in the GLFW window
void DeferredRenderer::recreateSwapChain() {
    // Get the window size
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    // While the window size is 0 (minimised)
    // This loop pauses the program until it is not minimised
    while (width == 0 || height == 0) {
        // Re-get the window size
        glfwGetFramebufferSize(window, &width, &height);
        // Wait until a glfw event is queued
        glfwWaitEvents();
    }

    // Wait till all resources are no longer in use
    vkDeviceWaitIdle(device);

    // Clean up the resources of the old swap chain
    cleanupSwapChain();

    // Call the commands to recreate the swap chain and all resources that depend onf the swap chain
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createDepthResources();
    createFramebuffers();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();

    cleanupImGuiSwapChain();
    recreateImGuiSwapChain();
}

// Cleanup all the objects associated with the swapchain
void DeferredRenderer::cleanupSwapChain() {
    // Clean up the depth buffer and its memory
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    // Clean up all frame buffers
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    // Free the command pool buffers but not the pool itself
    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    // Clean up the graphics pipeline
    vkDestroyPipeline(device, finalPipeline, nullptr);
    // Clean up the pipeline layout
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    // Clean up the render pass object
    vkDestroyRenderPass(device, renderPass, nullptr);

    // For each image view, cleanup the memory for that view
    // As we created each image view seperately, we must clean them up in the same way
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        vkDestroyImageView(device, swapChainImageViews[i], nullptr);
    }

    // Clean up the memory of the swapchain
    vkDestroySwapchainKHR(device, swapChain, nullptr);

    //// Clean up the per-swapchain uniform buffers
    //for (size_t i = 0; i < swapChainImages.size(); i++) {
    //    vkDestroyBuffer(device, uniformBuffers[i], nullptr);
    //    vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    //}

    // Destroy the descriptor set
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

// Recreate the ImGui elements that are depenent on the swapchain
void DeferredRenderer::recreateImGuiSwapChain() {
    createImGuiRenderPass();

    // Create the command pools and buffers
    imGuiCommandPools.resize(swapChainImageViews.size());
    imGuiCommandBuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        createCommandPool(&imGuiCommandPools[i], VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        createImGuiCommandBuffers(&imGuiCommandBuffers[i], 1, imGuiCommandPools[i]);
    }

    imGuiFrameBuffers.resize(swapChainImageViews.size());

    VkImageView attachment[1];
    VkFramebufferCreateInfo fbInfo = {};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = imGuiRenderPass;
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments = attachment;
    fbInfo.width = swapChainExtent.width;
    fbInfo.height = swapChainExtent.height;
    fbInfo.layers = 1;
    for (uint32_t i = 0; i < swapChainImageViews.size(); i++)
    {
        attachment[0] = swapChainImageViews[i];
        auto err = vkCreateFramebuffer(device, &fbInfo, VK_NULL_HANDLE, &imGuiFrameBuffers[i]);
        if (err != VK_SUCCESS) throw std::runtime_error("Failed to create ImGui frame buffers");
    }
    ImGui_ImplVulkan_SetMinImageCount(static_cast<uint32_t>(swapChainImages.size()));
}

// Cleanup the ImGui elements that are swapchain dependent
void DeferredRenderer::cleanupImGuiSwapChain() {
    for (auto framebuffer : imGuiFrameBuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    vkDestroyRenderPass(device, imGuiRenderPass, nullptr);

    for (size_t i = 0; i < imGuiCommandPools.size(); i++)
    {
        vkFreeCommandBuffers(device, imGuiCommandPools[i], 1, &imGuiCommandBuffers[i]);
        vkDestroyCommandPool(device, imGuiCommandPools[i], nullptr);
    }
}

// Cleanup all of the memory used by the vulkan app
void DeferredRenderer::cleanup() {

    // Cleanup ImGui
    cleanupImGuiSwapChain();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    cleanupSwapChain();

    // Clean up the image sampler
    vkDestroySampler(device, textureSampler, nullptr);
    // Clean up the texture image view
    vkDestroyImageView(device, textureImageView, nullptr);

    // Clean up the texture and its memory
    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);

    // Destroy the descriptor set layout
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    // Destroy the vertex buffer
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    // Free the memory on the GPU that the buffer occupied
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    // Destroy the index buffer
    vkDestroyBuffer(device, indexBuffer, nullptr);
    // Free the memory on the GPU that the buffer occupied
    vkFreeMemory(device, indexBufferMemory, nullptr);

    // Clean up the sephamores and fence for every frame
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }
    //vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    //vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);

    // Clean up the command pool
    // The pool is emptied in cleanupSwapChain()
    vkDestroyCommandPool(device, commandPool, nullptr);

    // Clean up the logical device memory
    vkDestroyDevice(device, nullptr);

    // If the validation layers are active
    if (enableValidationLayers) {
        // Clean up the memory associated with the debug messenger using the proxy function
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    // Clean up the memory associated with the window surface
    vkDestroySurfaceKHR(instance, surface, nullptr);

    // Clean up the memory associated with the vulkan instance
    vkDestroyInstance(instance, nullptr);

    // Clean up the resources associated with the window
    glfwDestroyWindow(window);

    // Terminate and clean up the memory assicaited with the running glfw library
    glfwTerminate();
}

void DeferredRenderer::ASSERT_VK_SUCCESS(VkResult res) {
    if (res != VK_SUCCESS)
        throw std::runtime_error("VK_SUCCESS Assert Failed!");
}

// Return a vector fo the extensions required by glfw
std::vector<const char*> DeferredRenderer::getRequiredExtensions() {
    // Get a list of all extensions required by glfw
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Store these required extensions in a vector
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // If validation layers are enabled add the debug extension to the list
    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Return vector of required extensions
    return extensions;
}

// messageServerity has a value representing how serious the error is: Verbose, Info, Warning or Error
// messageType specifies the purpose of the message: General, Validation or Performance
// pCallbackData details of the message itself, contains :pMessage, pObjects and objectCount
// pUserData is a pointer specified during callback creation by the programmer for additional data
VKAPI_ATTR VkBool32 VKAPI_CALL DeferredRenderer::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

// Proxy function for finding the vkCreateDebugUtilsMessengerEXT function address and running the function
// Helps us avoid having to use vkGetInstanceProcAddr in out class 
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    // Get a pointer to the function vkCreateDebugUtilsMessengerEXT
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    // If pointer was successfully returned
    if (func != nullptr) {
        // Return the result of the function with the appropriate parameters
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        // If no pointer could be found return an error
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// Proxy function for the destruction of a Debug Messenger
// Functionally identical to CreateDebugUtilsMessengerEXT but for finding and running the vkDestroyDebugUtilsMessengerEXT funciton
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
    // Else is unneeded here as if the vkCreateDebugUtilsMessengerEXT is found then vkDestroyDebugUtilsMessengerEXT must also be available
}

// Function to load .spv files and return as vector of chars
std::vector<char> readFile(const std::string& filename) {
    // Open the file and read as binary and seek to eof
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    // If the file failed to open
    if (!file.is_open()) {
        // Throw error to halt execution
        throw std::runtime_error("failed to open file!");
    }

    // Find the file size by the position of the eof (hence the seek when opening file)
    // tellg returns the position in the current stream
    size_t fileSize = (size_t)file.tellg();
    // Create a buffer that can hold the file contents
    // One entry per byte
    std::vector<char> buffer(fileSize);

    // Return to the start fo the file
    file.seekg(0);
    // Read the entire file into the buffer vector
    file.read(buffer.data(), fileSize);

    // Close the file
    file.close();

    // Return the contents of the file in the vector
    return buffer;
}
