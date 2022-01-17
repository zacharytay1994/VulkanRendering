#include "JZvk_Support.h"
#include <GLFW/glfw3.h>

/* STD INCLUDES */
#include <stdexcept>
#include <set>
#include <string>

/* PROJECT INCLUDES */
#include "../debug/JZvk_Log.h"

namespace JZvk
{
    std::vector<char const*> GetValidationLayers ()
    {
        return {
            "VK_LAYER_KHRONOS_validation"
        };
    }

    std::vector<char const*> GetDeviceExtensions ()
    {
        return {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
    }

    //std::vector<VkExtensionProperties> GetVulkanAvailableExtensions ()
    //{
    //    uint32_t extension_count { 0 };

    //    //  - get number of supported extensions
    //    vkEnumerateInstanceExtensionProperties ( nullptr , &extension_count , nullptr );

    //    //  - allocate container for storing the extensions
    //    std::vector<VkExtensionProperties> extensions ( extension_count );

    //    //  - query supported extensions details
    //    vkEnumerateInstanceExtensionProperties ( nullptr , &extension_count , extensions.data () );

    //    return extensions;
    //}

    bool CheckValidationLayerSupport ()
	{
        std::vector<const char*> const layers = GetValidationLayers ();

        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties ( &layer_count , nullptr );
        std::vector<VkLayerProperties> available_layers ( layer_count );
        vkEnumerateInstanceLayerProperties ( &layer_count , available_layers.data () );

        // prints specified layers
        Log ( LOG::INFO , "__________________________________________________" );
        Log ( LOG::INFO , "REQUESTED LAYERS:" );
        for ( const auto& layer : layers )
        {
            Log ( LOG::INFO , "\t" , layer );
        }
        // prints available layers
        Log ( LOG::INFO , "AVAILABLE LAYERS:" );
        for ( const auto& layer : available_layers )
        {
            Log ( LOG::INFO , "\t" , layer.layerName );
        }

        for ( const auto& layerName : layers )
        {
            bool layerFound = false;
            for ( const auto& layerProperty : available_layers )
            {
                if ( strcmp ( layerName , layerProperty.layerName ) )
                {
                    layerFound = true;
                }
            }
            if ( !layerFound )
            {
                Log ( LOG::ERROR , "Required GLFW extensions not supported by vulkan." );
                return false;
            }
        }
        Log ( LOG::INFO , "All required layers found." );
        Log ( LOG::INFO , "__________________________________________________" );
        return true;
	}

    bool CheckDeviceExtensionsSupport ( VkPhysicalDevice device )
    {
        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties ( device , nullptr , &extension_count , nullptr );
        std::vector<VkExtensionProperties> available_extensions ( extension_count );
        vkEnumerateDeviceExtensionProperties ( device , nullptr , &extension_count , available_extensions.data () );

        std::vector<char const*> device_extensions = GetDeviceExtensions ();
        std::set<std::string> required_extensions ( device_extensions.begin () , device_extensions.end () );

        Log ( LOG::INFO , "__________________________________________________" );
        Log ( LOG::INFO , "CHECKING DEVICE REQUIRED EXTENSIONS:" );
        Log ( LOG::INFO , "__________" );
        Log ( LOG::INFO , "Required device extensions:" );
        for ( auto const& extension : required_extensions )
        {
            Log ( LOG::INFO , "\t" , extension );
        }
        Log ( LOG::INFO , "Available vulkan extensions:" );
        for ( auto const& extension : available_extensions )
        {
            Log ( LOG::INFO , "\t" , extension.extensionName );
        }
        for ( auto const& extension : available_extensions )
        {
            required_extensions.erase ( extension.extensionName );
        }
        if ( !required_extensions.empty () )
        {
            Log ( LOG::ERROR , "Required device extensions not supported by device." );
        }
        Log ( LOG::INFO , "All device required extensions found." );
        Log ( LOG::INFO , "__________________________________________________" );
        return true;
    }

    SwapChainSupportDetails GetSwapChainSupport ( VkPhysicalDevice physicalDevice , VkSurfaceKHR surface )
    {
        SwapChainSupportDetails details;

        // check surface capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR ( physicalDevice , surface , &details.capabilities_ );

        // check surface formats
        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR ( physicalDevice , surface , &format_count , nullptr );
        if ( format_count != 0 )
        {
            details.formats_.resize ( format_count );
            vkGetPhysicalDeviceSurfaceFormatsKHR ( physicalDevice , surface , &format_count , details.formats_.data () );
        }

        // check surface present modes
        uint32_t present_modes_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR ( physicalDevice , surface , &present_modes_count , nullptr );
        if ( present_modes_count != 0 )
        {
            details.present_modes_.resize ( present_modes_count );
            vkGetPhysicalDeviceSurfacePresentModesKHR ( physicalDevice , surface , &present_modes_count , details.present_modes_.data () );
        }

        return details;
    }

    bool CheckSwapChainSupport ( VkPhysicalDevice physicalDevice , VkSurfaceKHR surface )
    {

        SwapChainSupportDetails details = GetSwapChainSupport ( physicalDevice , surface );
        return !details.formats_.empty () && !details.present_modes_.empty ();
    }

    bool CheckGLFWExtensionsSupport ( char const** glfwExtensions , unsigned int glfwExtensionCount )
    {
        uint32_t extension_count { 0 };
        //  - get number of supported extensions
        vkEnumerateInstanceExtensionProperties ( nullptr , &extension_count , nullptr );
        //  - allocate container for storing the extensions
        std::vector<VkExtensionProperties> vulkan_extensions ( extension_count );
        //  - query supported extensions details
        vkEnumerateInstanceExtensionProperties ( nullptr , &extension_count , vulkan_extensions.data () );

        // print out required extensions by glfw
        Log ( LOG::INFO , "__________________________________________________" );
        Log ( LOG::INFO , "CHECKING GLFW REQUIRED EXTENSIONS" );
        Log ( LOG::INFO , "__________" );
        Log ( LOG::INFO , "Required GLFW extensions:" );
        for ( int i = 0; i < glfwExtensionCount; ++i )
        {
            Log ( LOG::INFO , "\t" , "-" , glfwExtensions[i]);
        }

        // print out available extensions provided by vulkan
        Log ( LOG::INFO , "Availble Vulkan extensions:" );
        int i = 0;
        for ( const auto& extension : vulkan_extensions )
        {
            Log ( LOG::INFO , "\t" , "-" , extension.extensionName);
        }

        // check if all extensions are supported by vulkan
        for ( int i = 0; i < glfwExtensionCount; ++i )
        {
            bool found { false };
            for ( auto& vulkan_extension : vulkan_extensions )
            {
                if ( strcmp ( vulkan_extension.extensionName , glfwExtensions[ i ] ) )
                {
                    found = true;
                }
            }
            if ( !found )
            {
                return false;
            }
        }
        Log ( LOG::INFO , "All GLFW required extension supported." );
        Log ( LOG::INFO , "__________________________________________________" );
        return true;
    }

    QueueFamilyIndices FindQueueFamilies ( VkPhysicalDevice device , VkSurfaceKHR surface )
    {
        QueueFamilyIndices indices;

        // get all device queue families
        uint32_t qfp_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties ( device , &qfp_count , nullptr );
        std::vector<VkQueueFamilyProperties> queue_families_properties ( qfp_count );
        vkGetPhysicalDeviceQueueFamilyProperties ( device , &qfp_count , queue_families_properties.data () );

        // store them in self made queue family struct, i.e. QueueFamilyIndices
        // graphics and present family share the same index
        int i = 0;
        for ( const auto& qfp : queue_families_properties )
        {
            // look for graphics bit
            if ( qfp.queueFlags & VK_QUEUE_GRAPHICS_BIT )
            {
                indices.graphics_family_ = i;
            }

            // look for present support
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR ( device , i , surface , &presentSupport );
            if ( presentSupport )
            {
                indices.present_family_ = i;
            }

            // if all families filled, early exit from queue
            if ( indices.IsComplete () )
            {
                break;
            }
            ++i;
        }

        return indices;
    }

    bool IsDeviceSuitable ( VkPhysicalDevice device , VkSurfaceKHR surface )
    {
        return FindQueueFamilies ( device , surface ).IsComplete () &&
            CheckDeviceExtensionsSupport ( device ) &&
            CheckSwapChainSupport ( device , surface );
    }
}
