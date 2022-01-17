#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

/* STD INCLUDES */
#include <vector>

namespace JZvk
{
	namespace Create
	{
		GLFWwindow* GLFWWindow ( int width , int height , char const* title );

		VkInstance VKInstance ( char const* appName , bool validationLayersEnabled = true );

		VkDebugUtilsMessengerEXT VKDebugMessenger ( VkInstance instance );

		VkSurfaceKHR VKSurface ( VkInstance instance , GLFWwindow* window );

		VkPhysicalDevice VKPhysicalDevice ( VkInstance instance , VkSurfaceKHR surface );

		// logical device is a handle to the physical device
		VkDevice VKLogicalDevice ( VkPhysicalDevice physicalDevice , VkSurfaceKHR surface , bool validationLayersEnabled = 0 );

		VkQueue VKGraphicsQueue ( VkDevice logicalDevice , VkPhysicalDevice physicalDevice , VkSurfaceKHR surface );

		VkQueue VKPresentQueue ( VkDevice logicalDevice , VkPhysicalDevice physicalDevice , VkSurfaceKHR surface );

		VkSwapchainKHR VKSwapchain ( GLFWwindow* window , VkDevice logicalDevice , VkPhysicalDevice physicalDevice , VkSurfaceKHR surface );

		VkSurfaceFormatKHR VKSwapchainSurfaceFormat ( VkPhysicalDevice physicalDevice , VkSurfaceKHR surface );

		VkPresentModeKHR VKSwapchainPresentMode ( VkPhysicalDevice physicalDevice , VkSurfaceKHR surface );

		VkExtent2D VKSwapchainExtent2D ( GLFWwindow* window , VkPhysicalDevice physicalDevice , VkSurfaceKHR surface );

		std::vector<VkImage> VKSwapchainImages ( VkDevice logicalDevice , VkSwapchainKHR swapchain );

		std::vector<VkImageView> VKSwapchainImageViews ( VkDevice logicalDevice , std::vector<VkImage> const& swapchainImages , VkFormat swapchainImageFormat );
	}
}