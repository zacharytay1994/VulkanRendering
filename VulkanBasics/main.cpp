#define GLFW_INCLUDE_VULKAN
#define UNRERENCED_PARAMETER(P)(P)
#include <GLFW/glfw3.h>

#include <vector>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <fstream>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

/*!
 * VULKAN DEBUG FUNCTIONS - START
 * ****************************************************************
*/
// debug messenger call back function
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,     // severity of the message
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,                // message type
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,  // struct containing data relating to the callback
                                                    void* pUserData )                                           // custom data
{
    UNRERENCED_PARAMETER(messageSeverity);
    UNRERENCED_PARAMETER(messageType);
    UNRERENCED_PARAMETER(pUserData);

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    // should always return false, i.e. not abort function call that triggered this callback
    return VK_FALSE;
}

// proxy function that loads vkCreateDebugUtilsMessengerEXT and executes
// to create vulkan debug messenger
VkResult CreateDebugUtilsMessengerEXT(  VkInstance instance,
                                        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                        const VkAllocationCallbacks* pAllocator,
                                        VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// proxy function that loads vkDestroyDebugUtisMessengerEXT and executes it
// to clean up vulkan debug messenger
void DestroyDebugUtilsMessengerEXT( VkInstance instance,
                                    VkDebugUtilsMessengerEXT debugMessenger,
                                    const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
    else
    {
        std::cout << "vkDestroyDebugUtilsMessengerEXT func not loaded." << std::endl;
    }
}

// populates debug messenger create info
void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    // flag possible problems
    createInfo.messageSeverity =    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    createInfo.messageType =        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    // debug call back function defined above 
    createInfo.pfnUserCallback = debugCallback;

    createInfo.pUserData = nullptr;
}
/*!
 * VULKAN DEBUG FUNCTIONS - END
 * ****************************************************************
*/

/*!
 * SHADER READING FUNCTIONS - START
 * ****************************************************************
*/

static std::vector<char> readFile ( std::string const& filename )
{
    std::ifstream file ( filename , std::ios::ate | std::ios::binary );

    if ( !file.is_open () )
    {
        throw std::runtime_error ( "failed to open file!" );
    }

    size_t fileSize = ( size_t ) file.tellg ();
    std::vector<char> buffer ( fileSize );

    file.seekg ( 0 );
    file.read ( buffer.data () , fileSize );
    
    file.close ();

    return buffer;
}

/*!
 * SHADER READING FUNCTIONS - END
 * ****************************************************************
*/

class HelloTriangleApplication
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;                                 // glfw created window instance
    VkInstance instance;                                // vulkan instance
    VkDebugUtilsMessengerEXT debugMessenger;            // vulkan debug messenger, needed for vulkan debugging
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;   // vulkan physical device, i.e. gpu handle
    VkDevice device;                                    // logical device to interface with the physical device
    VkQueue graphicsQueue;                              // handle to the queues created with the logical device
    VkSurfaceKHR  surface;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;

    // vulkan sdk validation layers
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    /*!
    *   CHECKING DEVICE EXTENSIONS - START 
    */
    // vulkan sdk required device extensions
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    bool checkDeviceExtensionSupport ( VkPhysicalDevice device )
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties ( device , nullptr , &extensionCount , nullptr );

        std::vector<VkExtensionProperties> availableExtensions ( extensionCount );
        vkEnumerateDeviceExtensionProperties ( device , nullptr , &extensionCount , availableExtensions.data() );

        std::set<std::string> requiredExtensions ( deviceExtensions.begin () , deviceExtensions.end () );

        /*std::cout << "extension support check\nrequired: " << std::endl;
        for ( auto const& extension : requiredExtensions )
        {
            std::cout << extension << std::endl;
        }
        std::cout << "available:" << std::endl;
        for ( auto const& extension : availableExtensions )
        {
            std::cout << extension.extensionName << std::endl;
        }*/

        for ( auto const& extension : availableExtensions )
        {
            requiredExtensions.erase ( extension.extensionName );
        }

        return requiredExtensions.empty ();
    }

    /*!
    *   CHECKING DEVICE EXTENSIONS - END
    */

    /*!
        CHECKING SWAP CHAIN SUPPORT - START
    */

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    SwapChainSupportDetails querySwapChainSupport ( VkPhysicalDevice device )
    {
        SwapChainSupportDetails details;

        // check surface capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR ( device , surface , &details.capabilities );

        // check surface formats
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR ( device , surface , &formatCount , nullptr );
        if ( formatCount != 0 )
        {
            details.formats.resize ( formatCount );
            vkGetPhysicalDeviceSurfaceFormatsKHR ( device , surface , &formatCount , details.formats.data () );
        }

        // check surface presentation modes
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR ( device , surface , &presentModeCount , nullptr );
        if ( presentModeCount != 0 )
        {
            details.presentModes.resize ( presentModeCount );
            vkGetPhysicalDeviceSurfacePresentModesKHR ( device , surface , &presentModeCount , details.presentModes.data () );
        }
        return details;
    }
    
    VkSurfaceFormatKHR chooseSwapSurfaceFormat ( std::vector<VkSurfaceFormatKHR> const& availableFormats )
    {
        for ( auto const& availableFormat : availableFormats )
        {
            if ( availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
            {
                return availableFormat;
            }
        }
        return availableFormats[ 0 ];
    }

    VkPresentModeKHR chooseSwapPresentMode ( std::vector < VkPresentModeKHR> const& availablePresentModes )
    {
        for ( auto const& availablePresentMode : availablePresentModes )
        {
            if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
            {
                return availablePresentMode;
            }

        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent ( VkSurfaceCapabilitiesKHR const& capabilities )
    {
        if ( capabilities.currentExtent.width != UINT32_MAX )
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width , height;
            glfwGetFramebufferSize ( window , &width , &height );

            VkExtent2D actualExtent = { static_cast< uint32_t >( width ), static_cast< uint32_t >( height ) };

            actualExtent.width = std::clamp ( actualExtent.width , capabilities.minImageExtent.width , capabilities.maxImageExtent.width );
            actualExtent.height = std::clamp ( actualExtent.height , capabilities.minImageExtent.height , capabilities.maxImageExtent.height );

            return actualExtent;
        }
    }

    /*!
        CHECKING SWAP CHAIN SUPPORT - END
    */

    // validation layers are vulkan's debugging tool
    #ifdef NDEBUG
    const bool enableValidationLayers = false;
    #else
    const bool enableValidationLayers = true;
    #endif

    void initWindow()
    {
        // initialize glfw
        glfwInit();
        // tell glfw not to create opengl window
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // disable resize
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        // create glfw window
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan()
    {
        createInstance();
        setupDebugMessenger();
        createSurface ();
        pickPhysicalDevice();
        createLogicalDevice ();
        createSwapChain ();
        createImageViews ();
        createRenderPass ();
        createGraphicsPipeline ();
    }

    void createRenderPass ()
    {
        // single color buffer attachment from one of the images from the swap chain
        VkAttachmentDescription colorAttachment {};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // subpasses and attachment references, for postprocessing
        VkAttachmentReference colorAttachmentRef {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        // create render pass
        VkRenderPassCreateInfo renderPassInfo {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        if ( vkCreateRenderPass ( device , &renderPassInfo , nullptr , &renderPass ) != VK_SUCCESS )
        {
            throw std::runtime_error ( "failed to create render pass!" );
        }
    }

    VkShaderModule createShaderModule ( std::vector<char> const& code )
    {
        VkShaderModuleCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size ();
        createInfo.pCode = reinterpret_cast< uint32_t const* >( code.data () );

        VkShaderModule shaderModule;
        if ( vkCreateShaderModule ( device , &createInfo , nullptr , &shaderModule ) != VK_SUCCESS )
        {
            throw std::runtime_error ( "failed to create shader module!" );
        }

        return shaderModule;
    }

    void createGraphicsPipeline ()
    {
        auto vertShaderCode = readFile ( "shaders/vert.spv" );
        auto fragShaderCode = readFile ( "shaders/frag.spv" );

        std::cout << "size of vert read : " << vertShaderCode.size () << std::endl;
        std::cout << "size of frag read : " << fragShaderCode.size () << std::endl;

        VkShaderModule vertShaderModule = createShaderModule ( vertShaderCode );
        VkShaderModule fragShaderModule = createShaderModule ( fragShaderCode );

        // vertex shader stage creation
        VkPipelineShaderStageCreateInfo vertShaderStageInfo {};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";
        vertShaderStageInfo.pSpecializationInfo = nullptr;  // used to optimize constant variables

        // fragment shader stage creation
        VkPipelineShaderStageCreateInfo fragShaderStageInfo {};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        vertShaderStageInfo.module = fragShaderModule;
        vertShaderStageInfo.pName = "main";
        vertShaderStageInfo.pSpecializationInfo = nullptr;  // used to optimize constant variables

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        // fixed function pipeline setup - vertex input, no vertex data for now
        VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;

        // fixed function pipeline setup - input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // viewport
        VkViewport viewport {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = ( float ) swapChainExtent.width;
        viewport.height = ( float ) swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // scizzor rectangle
        VkRect2D scissor {};
        scissor.offset = { 0,0 };
        scissor.extent = swapChainExtent;

        // combine viewport and scizzor rectangle into a viewport state
        VkPipelineViewportStateCreateInfo viewportState {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        // rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.0f;

        // multisampling
        VkPipelineMultisampleStateCreateInfo multisampling {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        // color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment {};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        // non
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        // alpha blend
        /*colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;*/

        // color blend state
        VkPipelineColorBlendStateCreateInfo colorBlending {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[ 0 ] = 0.0f;
        colorBlending.blendConstants[ 1 ] = 0.0f;
        colorBlending.blendConstants[ 2 ] = 0.0f;
        colorBlending.blendConstants[ 3 ] = 0.0f;

        // setting dynamic states of the pipeline to modify it without recreating entire pipeline
        VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_LINE_WIDTH
        };

        VkPipelineDynamicStateCreateInfo dynamicState {};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        // uniform variables in shaders, pipeline layout
        VkPipelineLayout pipelineLayout;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        if ( vkCreatePipelineLayout ( device , &pipelineLayoutInfo , nullptr , &pipelineLayout ) != VK_SUCCESS )
        {
            throw std::runtime_error ( "failed to create pipeline layout!" );
        }

        // clean up local shader modules after compiling and linking
        vkDestroyShaderModule ( device , fragShaderModule , nullptr );
        vkDestroyShaderModule ( device , vertShaderModule , nullptr );
    }

    void createImageViews ()
    {
        swapChainImageViews.resize ( swapChainImages.size () );

        for ( size_t i = 0; i < swapChainImages.size (); ++i )
        {
            VkImageViewCreateInfo createInfo {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[ i ];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if ( vkCreateImageView ( device , &createInfo , nullptr , &swapChainImageViews[ i ] ) != VK_SUCCESS )
            {
                throw std::runtime_error ( "failed to create image views!" );
            }
        }
    }

    void createSwapChain ()
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport ( physicalDevice );

        // get swap chain formats
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat ( swapChainSupport.formats );

        // get swap chain present modes
        VkPresentModeKHR presentMode = chooseSwapPresentMode ( swapChainSupport.presentModes );

        // get swap chain extent from capabilities
        VkExtent2D extent = chooseSwapExtent ( swapChainSupport.capabilities );

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

        if ( swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount )
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // queue handling
        QueueFamilyIndices indices = findQueueFamilies ( physicalDevice );
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value (), indices.presentFamily.value () };
        if ( indices.graphicsFamily != indices.presentFamily )
        {
            // any queue can access the image even from a different queue
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            // only the owning queue can access the swap chain image, more efficient
            // most hardware have the same graphics and presentation queue family,
            // so exclusive if the most used case
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 1;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        // transform of the image in the swap chain, e.g. rotation
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

        // how the image blends with other windows
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        createInfo.presentMode = presentMode;
        // if true, pixels blocked by other windows are clipped
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if ( vkCreateSwapchainKHR ( device , &createInfo , nullptr , &swapChain ) != VK_SUCCESS )
        {
            throw std::runtime_error ( "failed to create swap chain!" );
        }

        // get swap chain images
        vkGetSwapchainImagesKHR ( device , swapChain , &imageCount , nullptr );
        swapChainImages.resize ( imageCount );
        vkGetSwapchainImagesKHR ( device , swapChain , &imageCount , swapChainImages.data () );

        // store swap chain format and extent
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createSurface ()
    {
        if ( glfwCreateWindowSurface ( instance , window , nullptr , &surface ) != VK_SUCCESS )
        {
            throw std::runtime_error ( "failed to create window surface!" );
        }
    }

    void createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

        // create set of queue families, set is used to guarantee a unique key
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value (), indices.presentFamily.value () };

        float queuePriority = 1.0f;

        // iterate over queue families and create queue info
        for ( auto const& queueFamily : uniqueQueueFamilies )
        {
            VkDeviceQueueCreateInfo queueCreateInfo { };
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back ( queueCreateInfo );
        }

        // no device features needed for logical device for now
        VkPhysicalDeviceFeatures deviceFeatures{};
        
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast< uint32_t >( queueCreateInfos.size () );
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast< uint32_t >( deviceExtensions.size () );
        createInfo.ppEnabledExtensionNames = deviceExtensions.data ();

        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        // create device
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create logical device!");
        }

        // get graphics handle
        vkGetDeviceQueue ( device , indices.graphicsFamily.value () , 0 , &graphicsQueue );

        // get present handle
        vkGetDeviceQueue ( device , indices.presentFamily.value () , 0 , &presentQueue );
    }

    void pickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0)
        {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }
        // get all devices
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        // prints out all devices
        /*std::cout << "physical devices:" << std::endl;
        for (const auto& physical_device : devices)
        {
            VkPhysicalDeviceProperties device_properties;
            vkGetPhysicalDeviceProperties(physical_device, &device_properties);

            std::cout << "\t" << device_properties.deviceName << std::endl;
        }*/

        // check if any devices are suitable for the operations that we want
        // selects first suitable device
        for (const auto& physical_device : devices)
        {
            if (isDeviceSuitable(physical_device))
            {
                physicalDevice = physical_device;
                VkPhysicalDeviceProperties physical_device_property;
                vkGetPhysicalDeviceProperties(physicalDevice, &physical_device_property);
                std::cout << "selected device:\n\t" << physical_device_property.deviceName << std::endl;
                break;
            }
        }
        if (physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("failed to find a suitable GPU for selected operations.");
        }
    }

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete()
        {
            return graphicsFamily.has_value()
                && presentFamily.has_value();
        }
    };

    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        // check if device has queue family and is suitable
        QueueFamilyIndices indices = findQueueFamilies(device);

        bool extensionsSupported = checkDeviceExtensionSupport ( device );

        // can only query swap chain support is extensions are supported
        bool swapChainAdequate = false;
        if ( extensionsSupported )
        {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport ( device );
            swapChainAdequate = !swapChainSupport.formats.empty () && !swapChainSupport.presentModes.empty ();
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;

        // get all device queue families
        uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyPropertyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamiliesProperties(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyPropertyCount, queueFamiliesProperties.data());

        // store them in self made queue family struct, i.e. QueueFamilyIndices
        int i = 0;
        for (const auto& queueFamilyProperty : queueFamiliesProperties)
        {
            // look for graphics bit
            if (queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = i;
            }

            // look for present support
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR ( device , i , surface , &presentSupport );
            if ( presentSupport )
            {
                indices.presentFamily = i;
            }

            // if all families filled, early exit from queue
            if (indices.isComplete())
            {
                break;
            }
            ++i;
        }

        return indices;
    }

    void setupDebugMessenger()
    {
        if (!enableValidationLayers) return;
        // debug messenger create info
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};

        // configure create info
        PopulateDebugMessengerCreateInfo(createInfo);

        // create debug messenger with proxy function defined above
        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void mainLoop()
    {
        // glfw window loop
        while (!glfwWindowShouldClose(window))
        {
            // process glfw events
            glfwPollEvents();
        }
    }

    void cleanup()
    {
        // clean up pipeline layout
        vkDestroyPipelineLayout ( device , pipelineLayout , nullptr );
        vkDestroyRenderPass ( device , renderPass , nullptr );

        // clean up image views created by us
        for ( auto imageView : swapChainImageViews )
        {
            vkDestroyImageView ( device , imageView , nullptr );
        }

        // cleanup swap chain before device
        vkDestroySwapchainKHR ( device , swapChain , nullptr );

        vkDestroyDevice(device, nullptr);

        // destroy debug messenger
        if (enableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        // destroy surface, happens before destroy instance
        vkDestroySurfaceKHR ( instance , surface , nullptr );

        // destroy vkinstance before program exits
        vkDestroyInstance(instance, nullptr);

        // clean up glfw
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void createInstance()
    {
        // check validation layers for debugging
        if (enableValidationLayers && !checkValidationLayerSupport())
        {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        //  lets driver optimize our application with this info
        VkApplicationInfo appInfo{};
        appInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName    = "Hello Triangle";
        appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName         = "No Engine";
        appInfo.engineVersion       = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion          = VK_API_VERSION_1_0;

        /*  non optional, tells vulkan driver which global extensionsand validation
            layers we want to use. */
        VkInstanceCreateInfo createInfo{};
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            // to check for errors in vkCreateInstance,
            // we have to pass in debug messenger create info as the pnext of instance create info
            PopulateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        /*  Since vulkan is cross platform, we have to specify the platform
        *   extension for the application, in this case we can get it from glfw
        */
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        //  get list of supported extensions from vulkan
        uint32_t extensionCount = 0;
        //  - get number of supported extensions
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        //  - allocate container for storing the extensions
        std::vector<VkExtensionProperties> extensions(extensionCount);
        //  - query supported extensions details
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        // check if all extensions available
        if (!checkExtensionsSupport(glfwExtensions, glfwExtensionCount, extensions))
        {
            throw std::runtime_error("required glfw extension not supported by vulkan.");
        }
        else
        {
            std::cout << "all required extensions found." << std::endl;
        }


        // add debug extensions if debug and has validation layers
        if (enableValidationLayers)
        {
            std::vector<const char*> debug_extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
            debug_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            createInfo.enabledExtensionCount = static_cast<uint32_t>(debug_extensions.size());
            createInfo.ppEnabledExtensionNames = debug_extensions.data();

            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            //  create vulkan instance
            if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create instance!");
            }
        }
        else
        {
            createInfo.enabledExtensionCount = glfwExtensionCount;
            createInfo.ppEnabledExtensionNames = glfwExtensions;

            createInfo.enabledLayerCount = 0;

            //  create vulkan instance
            if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create instance!");
            }
        }
    }

    bool checkExtensionsSupport(const char** glfwExtensions, int n,
        const std::vector<VkExtensionProperties>& vulkanExtensions)
    {
        // print out required extensions by glfw
        std::cout << "glfw required extensions:" << std::endl;
        for (int i = 0; i < n; ++i)
        {
            std::cout << "\t" << glfwExtensions[i] << std::endl;
        }
        // print out available extensions provided by vulkan
        std::cout << "availble extensions:" << std::endl;
        for (const auto& extension : vulkanExtensions)
        {
            std::cout << "\t" << extension.extensionName << std::endl;
        }

        for (int i = 0; i < n; ++i)
        {
            bool found{ false };
            for (auto& vk_extension : vulkanExtensions)
            {
                if (strcmp(vk_extension.extensionName, glfwExtensions[i]))
                {
                    found = true;
                }
            }
            if (!found)
            {
                return false;
            }
        }
        return true;
    }

    bool checkValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        // prints specified layers
        std::cout << "requested layers:" << std::endl;
        for (const auto& layer : validationLayers)
        {
            std::cout << "\t" << layer << std::endl;
        }
        // prints available layers
        std::cout << "available layers:" << std::endl;
        for (const auto& layer : availableLayers)
        {
            std::cout << "\t" << layer.layerName << std::endl;
        }

        for (const auto& layerName : validationLayers)
        {
            bool layerFound = false;
            for (const auto& layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName))
                {
                    layerFound = true;
                }
            }
            if (!layerFound)
            {
                return false;
            }
        }
        std::cout << "all required layers found." << std::endl;
        return true;
    }
};

int main()
{
    HelloTriangleApplication app;

    try
    {
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}