#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace JZvk
{
	namespace Create
	{
		GLFWwindow* GLFWWindow ( int width , int height , char const* title );

		VkInstance VKInstance ( char const* appName , bool validationLayersEnabled = true );

		VkDebugUtilsMessengerEXT VKDebugMessenger ( VkInstance instance );

		VkSurfaceKHR VKSurface ( VkInstance instance , GLFWwindow* window );

		VkPhysicalDevice VKPhysicalDevice ( VkInstance instance );
	}
}