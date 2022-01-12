#include "JZvk_Create.h"

/* PROJECT INCLUDES */
#include "../tools/JZvk_Support.h"
#include "../debug/JZvk_Debug.h"
#include "../debug/JZvk_Log.h"

/* STD INCLUDES */
#include <cstdint>

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
	}
}