#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

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
    GLFWwindow* window;
    VkInstance instance;

    // vulkan sdk validation layers
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

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
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        /*  non optional, tells vulkan driver which global extensionsand validation
            layers we want to use. */
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
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

        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        //  global validation layers to enable, leave empty for now
        createInfo.enabledLayerCount = 0;

        //  create vulkan instance
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create instance!");
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