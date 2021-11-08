#define GLFW_INCLUDE_VULKAN
#define UNRERENCED_PARAMETER(P)(P)
#include <GLFW/glfw3.h>

#include <vector>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <optional>

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

    // vulkan sdk validation layers
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

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
        pickPhysicalDevice();
    }

    void createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
        queueCreateInfo.queueCount = 1;
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        // no device features needed for logical device for now
        VkPhysicalDeviceFeatures deviceFeatures{};
        
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = 0;

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

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
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
        std::cout << "physical devices:" << std::endl;
        for (const auto& physical_device : devices)
        {
            VkPhysicalDeviceProperties device_properties;
            vkGetPhysicalDeviceProperties(physical_device, &device_properties);

            std::cout << "\t" << device_properties.deviceName << std::endl;
        }

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

        bool isComplete()
        {
            return graphicsFamily.has_value();
        }
    };

    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        // check if device has queue family and is suitable
        QueueFamilyIndices indices = findQueueFamilies(device);
        return indices.isComplete();
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
            if (queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = i;
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
        vkDestroyDevice(device, nullptr);

        // destroy debug messenger
        if (enableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

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