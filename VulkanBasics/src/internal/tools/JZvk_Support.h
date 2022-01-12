/* CHECKS ALL KINDS OF SUPPORTS OF VARIOUS THINGS */
#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>
#include <vector>

namespace JZvk
{
	std::vector<char const*> GetValidationLayers ();
	std::vector<char const*> GetDeviceExtensions ();

	/* CHECK VARIOUS SUPPORTS */
	/*!
	 * @brief ___JZvk::CheckValidationLayerSupport()___
	 * **************************************************************
	 * Checks validation layers returned by GetValidationLayers()
	 * are supported by vulkan.
	 * **************************************************************
	 * @return bool
	 * : If supported.
	 * **************************************************************
	*/
	bool CheckValidationLayerSupport ();

	/*!
	 * @brief ___JZvk::CheckGLFWExtensionsSupport()___
	 * **************************************************************
	 * Checks if glfw required extensions are supported by vulkan.
	 * Extensions can be got with glfwGetRequiredInstanceExtensions().
	 * **************************************************************
	 * @return bool
	 * : If supported.
	 * **************************************************************
	*/
	bool CheckGLFWExtensionsSupport ( char const** glfwExtensions , unsigned int glfwExtensionCount );

	/*!
	 * @brief ___JZvk::CheckValidationLayerSupport()___
	 * **************************************************************
	 * Checks if device extensions required by GetDeviceExtensions()
	 * are supported by the device.
	 * **************************************************************
	 * @return bool
	 * : If supported.
	 * **************************************************************
	*/
	bool CheckDeviceExtensionsSupport ( VkPhysicalDevice device );

	/*!
	 * @brief ___JZvk::CheckSwapChainSupport()___
	 * **************************************************************
	 * Checks if swap chain is supported by device and surface.
	 * **************************************************************
	 * @return bool
	 * : If supported.
	 * **************************************************************
	*/
	bool CheckSwapChainSupport ( VkPhysicalDevice device , VkSurfaceKHR surface );


	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphics_family_;
		std::optional<uint32_t> present_family_;

		bool IsComplete ()
		{
			return graphics_family_.has_value ()
				&& present_family_.has_value ();
		}
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities_;
		std::vector<VkSurfaceFormatKHR> formats_;
		std::vector<VkPresentModeKHR> present_modes_;
	};

	QueueFamilyIndices FindQueueFamilies ( VkPhysicalDevice device , VkSurfaceKHR surface );

	bool IsDeviceSuitable ( VkPhysicalDevice device , VkSurfaceKHR surface );
}