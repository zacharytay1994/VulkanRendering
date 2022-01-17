#include "JZvk_Create.h"

/* PROJECT INCLUDES */
#include "../tools/JZvk_Support.h"
#include "../debug/JZvk_Debug.h"
#include "../debug/JZvk_Log.h"

/* STD INCLUDES */
#include <cstdint>
#include <set>
#include <algorithm>

namespace JZvk
{
	namespace Create
	{
		GLFWwindow* GLFWWindow ( int width , int height , char const* title )
		{
			// initialize glfw
			glfwInit ();
			// tell glfw not to create opengl window
			glfwWindowHint ( GLFW_CLIENT_API , GLFW_NO_API );
			// disable resize
			glfwWindowHint ( GLFW_RESIZABLE , GLFW_FALSE );
			// create glfw window
			return glfwCreateWindow ( width , height , title , nullptr , nullptr );
		}

		VkInstance VKInstance ( char const* appName , bool validationLayersEnabled )
		{
			if ( validationLayersEnabled && !CheckValidationLayerSupport () )
			{
				Log ( LOG::ERROR , "Creating instance, validation requested but not available." );
			}

			// create app info
			VkApplicationInfo app_info {};
			app_info.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
			app_info.pApplicationName	= appName;
			app_info.applicationVersion = VK_MAKE_VERSION ( 1 , 0 , 0 );
			app_info.pEngineName		= "Engine";
			app_info.engineVersion		= VK_MAKE_VERSION ( 1 , 0 , 0 );
			app_info.apiVersion			= VK_API_VERSION_1_0;

			// check glfw extensions and supported by vulkan
			uint32_t glfw_extension_count = 0;
			char const** glfw_extensions;
			glfw_extensions = glfwGetRequiredInstanceExtensions ( &glfw_extension_count );
			JZvk::CheckGLFWExtensionsSupport ( glfw_extensions , glfw_extension_count );

			VkInstanceCreateInfo create_info {};
			create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			create_info.pApplicationInfo = &app_info;

			VkInstance instance;
			if ( validationLayersEnabled )
			{
				// set validation layer settings
				std::vector<char const*> validation_layers = GetValidationLayers ();
				create_info.enabledLayerCount = static_cast< uint32_t >( validation_layers.size () );
				create_info.ppEnabledLayerNames = validation_layers.data ();

				// set debug info
				VkDebugUtilsMessengerCreateInfoEXT debug_create_info {};
				PopulateDebugMessengerCreateInfo ( debug_create_info );
				create_info.pNext = ( VkDebugUtilsMessengerCreateInfoEXT* ) &debug_create_info;

				// append debug extensions to glfw extensions and set
				std::vector<char const*> debug_extensions ( glfw_extensions , glfw_extensions + glfw_extension_count );
				debug_extensions.push_back ( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
				create_info.enabledExtensionCount = static_cast< uint32_t >( debug_extensions.size () );
				create_info.ppEnabledExtensionNames = debug_extensions.data ();

				if ( vkCreateInstance ( &create_info , nullptr , &instance ) != VK_SUCCESS )
				{
					Log ( LOG::ERROR , "Failed to create VkInstance." );
				}
			}
			else
			{
				// no validation layers
				create_info.enabledLayerCount = 0;

				// set glfw extensions
				create_info.enabledExtensionCount = glfw_extension_count;
				create_info.ppEnabledExtensionNames = glfw_extensions;

				if ( vkCreateInstance ( &create_info , nullptr , &instance ) != VK_SUCCESS )
				{
					Log ( LOG::ERROR , "Failed to create VkInstance." );
				}
			}

			return instance;
		}

		VkDebugUtilsMessengerEXT VKDebugMessenger ( VkInstance instance )
		{
			VkDebugUtilsMessengerCreateInfoEXT debug_create_info {};
			PopulateDebugMessengerCreateInfo ( debug_create_info );

			VkDebugUtilsMessengerEXT debug_messenger;
			if ( CreateDebugUtilsMessengerEXT ( instance , &debug_create_info , nullptr , &debug_messenger ) != VK_SUCCESS )
			{
				Log ( LOG::ERROR , "Failed to set up debug messenger." );
			}

			return debug_messenger;
		}

		VkSurfaceKHR VKSurface ( VkInstance instance , GLFWwindow* window )
		{
			VkSurfaceKHR surface;
			if ( glfwCreateWindowSurface ( instance , window , nullptr , &surface ) != VK_SUCCESS )
			{
				Log ( LOG::ERROR , "Failed to create window surface" );
			}

			return surface;
		}

		VkPhysicalDevice VKPhysicalDevice ( VkInstance instance , VkSurfaceKHR surface )
		{
			// pick a physical device
			uint32_t device_count { 0 };
			vkEnumeratePhysicalDevices ( instance , &device_count , nullptr );
			if ( device_count == 0 )
			{
				Log ( LOG::ERROR , "Failed to find GPUs with Vulkan support." );
			}
			std::vector<VkPhysicalDevice> devices ( device_count );
			vkEnumeratePhysicalDevices ( instance , &device_count , devices.data () );

			// print all devices
			Log ( LOG::INFO , "__________________________________________________" );
			Log ( LOG::INFO , "SELECTING PHYSICAL DEVICE" );
			Log ( LOG::INFO , "__________" );
			Log ( LOG::INFO , "Physical Devices:" );
			for ( auto const& physical_device : devices )
			{
				VkPhysicalDeviceProperties device_properties;
				vkGetPhysicalDeviceProperties ( physical_device , &device_properties );
				Log ( LOG::INFO , "\t" , device_properties.deviceName );
			}

			Log ( LOG::INFO , "Suitable Device Found:" );
			VkPhysicalDevice device_out { VK_NULL_HANDLE };
			for ( auto const& physical_device : devices )
			{
				if ( IsDeviceSuitable ( physical_device , surface ) )
				{
					// device found
					device_out = physical_device;
					VkPhysicalDeviceProperties device_properties;
					vkGetPhysicalDeviceProperties ( device_out , &device_properties );
					Log ( LOG::INFO , "\t" , device_properties.deviceName );
					break;
				}
			}

			if ( device_out == VK_NULL_HANDLE )
			{
				Log ( LOG::ERROR , "Failed to find a suitable GPU for selected operations." );
			}

			Log ( LOG::INFO , "__________________________________________________" );
			return device_out;
		}

		VkDevice VKLogicalDevice ( VkPhysicalDevice physicalDevice , VkSurfaceKHR surface , bool validationLayersEnabled  )
		{
			QueueFamilyIndices indices = FindQueueFamilies ( physicalDevice , surface );

			// create set of queue families to guarantee unique key
			std::set<uint32_t> unique_queue_families = { indices.graphics_family_.value (), indices.present_family_.value () };

			float queue_priority { 1.0f };

			// iterate over families and create queue info
			std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
			for ( auto const& queue_family : unique_queue_families )
			{
				VkDeviceQueueCreateInfo queue_create_info {};
				queue_create_info.sType				= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queue_create_info.queueFamilyIndex	= queue_family;
				queue_create_info.queueCount		= 1;
				queue_create_info.pQueuePriorities	= &queue_priority;

				queue_create_infos.push_back ( queue_create_info );
			}

			// device features for logical device
			VkPhysicalDeviceFeatures device_features {};

			// create logical device
			std::vector<const char*> device_extensions = GetDeviceExtensions ();
			std::vector<const char*> validation_layers = GetValidationLayers ();

			VkDeviceCreateInfo create_info {};
			create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			create_info.pQueueCreateInfos = queue_create_infos.data ();
			create_info.queueCreateInfoCount = static_cast< uint32_t >( queue_create_infos.size () );
			create_info.pEnabledFeatures = &device_features;
			create_info.enabledExtensionCount = static_cast< uint32_t >( device_extensions.size () );
			create_info.ppEnabledExtensionNames = device_extensions.data ();

			if ( validationLayersEnabled )
			{
				create_info.enabledLayerCount = static_cast< uint32_t >( validation_layers.size () );
				create_info.ppEnabledLayerNames = validation_layers.data ();
			}
			else
			{
				create_info.enabledLayerCount = 0;
			}

			VkDevice logical_device;
			if ( vkCreateDevice ( physicalDevice , &create_info , nullptr , &logical_device ) != VK_SUCCESS )
			{
				Log ( LOG::ERROR , "Failed to create logical device!" );
			}

			return logical_device;
		}

		VkQueue VKGraphicsQueue ( VkDevice logicalDevice , VkPhysicalDevice physicalDevice , VkSurfaceKHR surface )
		{
			QueueFamilyIndices indices = FindQueueFamilies ( physicalDevice , surface );
			VkQueue graphics_queue;
			vkGetDeviceQueue ( logicalDevice , indices.graphics_family_.value () , 0 , &graphics_queue );
			return graphics_queue;
		}

		VkQueue VKPresentQueue ( VkDevice logicalDevice , VkPhysicalDevice physicalDevice , VkSurfaceKHR surface )
		{
			QueueFamilyIndices indices = FindQueueFamilies ( physicalDevice , surface );
			VkQueue present_queue;
			vkGetDeviceQueue ( logicalDevice , indices.present_family_.value () , 0 , &present_queue );
			return present_queue;
		}

		VkSwapchainKHR VKSwapchain ( GLFWwindow* window , VkDevice logicalDevice , VkPhysicalDevice physicalDevice , VkSurfaceKHR surface )
		{
			SwapChainSupportDetails swapchain_support = GetSwapChainSupport ( physicalDevice , surface );

			// get swap chain formats
			VkSurfaceFormatKHR surface_format = VKSwapchainSurfaceFormat ( physicalDevice , surface );

			// get swap chain present modes
			VkPresentModeKHR present_mode = VKSwapchainPresentMode ( physicalDevice , surface );

			// get swap chain extent from capabilities
			VkExtent2D swapchain_extent = VKSwapchainExtent2D ( window , physicalDevice , surface );

			uint32_t image_count = swapchain_support.capabilities_.minImageCount + 1;

			if ( swapchain_support.capabilities_.maxImageCount > 0 && image_count > swapchain_support.capabilities_.maxImageCount )
			{
				image_count = swapchain_support.capabilities_.maxImageCount;
			}

			VkSwapchainCreateInfoKHR createInfo {};
			createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			createInfo.surface = surface;
			createInfo.minImageCount = image_count;
			createInfo.imageFormat = surface_format.format;
			createInfo.imageColorSpace = surface_format.colorSpace;
			createInfo.imageExtent = swapchain_extent;
			createInfo.imageArrayLayers = 1;
			createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			createInfo.presentMode = present_mode;
			// transform of the image in the swap chain, e.g. rotation
			createInfo.preTransform = swapchain_support.capabilities_.currentTransform;
			// how the image blends with other windows
			createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			// if true, pixels blocked by other windows are clipped
			createInfo.clipped = VK_TRUE;
			createInfo.oldSwapchain = VK_NULL_HANDLE;

			// queue handling
			QueueFamilyIndices indices = FindQueueFamilies ( physicalDevice , surface );
			uint32_t queueFamilyIndices[] = { indices.graphics_family_.value (), indices.present_family_.value () };
			if ( indices.graphics_family_ != indices.present_family_ )
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

			VkSwapchainKHR swapchain;
			if ( vkCreateSwapchainKHR ( logicalDevice , &createInfo , nullptr , &swapchain ) != VK_SUCCESS )
			{
				throw std::runtime_error ( "failed to create swap chain!" );
			}

			// store other swap chain information
			/*vkGetSwapchainImagesKHR ( logicalDevice , swapchain , &image_count , nullptr );
			swapchainImages.resize ( image_count );
			vkGetSwapchainImagesKHR ( logicalDevice , swapchain , &image_count , swapchainImages.data () );*/
			/*swapchainImageFormat = surface_format.format;
			swapchainExtent = swapchain_extent;*/
			return swapchain;
		}

		VkSurfaceFormatKHR VKSwapchainSurfaceFormat ( VkPhysicalDevice physicalDevice , VkSurfaceKHR surface )
		{
			std::vector<VkSurfaceFormatKHR> available_formats = GetSwapChainSupport ( physicalDevice , surface ).formats_;
			// if format specified found 
			for ( auto const& available_format : available_formats )
			{
				if ( available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
				{
					return available_format;
				}
			}
			// else return first one
			return available_formats[ 0 ];
		}

		VkPresentModeKHR VKSwapchainPresentMode ( VkPhysicalDevice physicalDevice , VkSurfaceKHR surface )
		{
			std::vector<VkPresentModeKHR> available_present_modes = GetSwapChainSupport ( physicalDevice , surface ).present_modes_;
			// if present mode specified found 
			for ( auto const& available_present_mode : available_present_modes )
			{
				if ( available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR )
				{
					return available_present_mode;
				}
			}
			// else return first in first out
			return VK_PRESENT_MODE_FIFO_KHR;
		}

		VkExtent2D VKSwapchainExtent2D ( GLFWwindow* window , VkPhysicalDevice physicalDevice , VkSurfaceKHR surface )
		{
			VkSurfaceCapabilitiesKHR capabilities = GetSwapChainSupport ( physicalDevice , surface ).capabilities_;
			if ( capabilities.currentExtent.width != UINT32_MAX )
			{
				return capabilities.currentExtent;
			}
			else
			{
				int width , height;
				glfwGetFramebufferSize ( window , &width , &height );

				VkExtent2D actual_extent = { static_cast< uint32_t >( width ), static_cast< uint32_t >( height ) };

				actual_extent.width = std::clamp ( actual_extent.width , capabilities.minImageExtent.width , capabilities.maxImageExtent.width );
				actual_extent.height = std::clamp ( actual_extent.height , capabilities.minImageExtent.height , capabilities.maxImageExtent.height );

				return actual_extent;
			}
		}

		std::vector<VkImage> VKSwapchainImages ( VkDevice logicalDevice , VkSwapchainKHR swapchain )
		{
			std::vector<VkImage> images;
			uint32_t image_count { 0 };
			vkGetSwapchainImagesKHR ( logicalDevice , swapchain , &image_count , nullptr );
			images.resize ( image_count );
			vkGetSwapchainImagesKHR ( logicalDevice , swapchain , &image_count , images.data () );
			return images;
		}

		std::vector<VkImageView> VKSwapchainImageViews ( VkDevice logicalDevice , std::vector<VkImage> const& swapchainImages , VkFormat swapchainImageFormat )
		{
			std::vector<VkImageView> image_views;
			image_views.resize ( swapchainImages.size () );

			for ( size_t i = 0; i < swapchainImages.size (); ++i )
			{
				VkImageViewCreateInfo createInfo {};
				createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				createInfo.image = swapchainImages[ i ];
				createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				createInfo.format = swapchainImageFormat;
				createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				createInfo.subresourceRange.baseMipLevel = 0;
				createInfo.subresourceRange.levelCount = 1;
				createInfo.subresourceRange.baseArrayLayer = 0;
				createInfo.subresourceRange.layerCount = 1;

				if ( vkCreateImageView ( logicalDevice , &createInfo , nullptr , &image_views[i] ) != VK_SUCCESS )
				{
					Log ( LOG::ERROR , "Failed to create image views." );
				}
			}

			return image_views;
		}
	}
}