#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace CsySmallVk
{
	inline VkInstanceCreateInfo instanceCreateInfo()
	{
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		return createInfo;
	}

	inline VkApplicationInfo applicationInfo()
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_APPLICATION_INFO;
		return appInfo;
	}

	inline VkDeviceCreateInfo deviceCreateInfo()
	{
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		return createInfo;
	}

	inline VkDeviceQueueCreateInfo deviceQueueCreateInfo()
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		return queueCreateInfo;
	}
}