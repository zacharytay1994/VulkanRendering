#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

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
        if (!GLFWExtensionsFound(glfwExtensions, glfwExtensionCount, extensions))
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

    bool GLFWExtensionsFound(const char** glfwExtensions, int n,
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
            std::string glfw_extension_name{ glfwExtensions[i] };
            for (auto& vk_extension : vulkanExtensions)
            {
                std::string vk_extension_name{ vk_extension.extensionName };
                if (glfw_extension_name == vk_extension_name)
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