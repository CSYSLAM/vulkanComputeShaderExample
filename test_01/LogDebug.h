#pragma once
#include <vulkan/vulkan.h>

namespace CsySmallVk
{
	struct LogDebug
	{
        static void printInstanceExtensionProperties(const std::vector<VkExtensionProperties>& extensions)
        {
            for (const auto& extension : extensions)
            {
                std::cout << "Extension Name: " << extension.extensionName << std::endl;
                std::cout << "Spec Version: " << extension.specVersion << std::endl;
                std::cout << std::endl;
            }
        }

        static void printPhysicalDevicesInfo(const std::vector<VkPhysicalDevice>& devices)
        {
            for (const auto& device : devices)
            {
                VkPhysicalDeviceProperties deviceProperties;
                vkGetPhysicalDeviceProperties(device, &deviceProperties);

                std::cout << "Device Name: " << deviceProperties.deviceName << std::endl;
                std::cout << "API Version: " << VK_VERSION_MAJOR(deviceProperties.apiVersion) << "."
                    << VK_VERSION_MINOR(deviceProperties.apiVersion) << "."
                    << VK_VERSION_PATCH(deviceProperties.apiVersion) << std::endl;
                std::cout << "Driver Version: " << deviceProperties.driverVersion << std::endl;
                std::cout << "Vendor ID: " << deviceProperties.vendorID << std::endl;
                std::cout << "Device ID: " << deviceProperties.deviceID << std::endl;
                std::cout << "Device Type: " << deviceProperties.deviceType << std::endl;
                std::cout << std::endl;
            }
        }
	
        // ��VkQueueFamilyProperties��Ϣ�������׼���
        static void printQueueFamilyProperties(const std::vector<VkQueueFamilyProperties>& queueFamilies)
        {
            std::cout << "Found " << queueFamilies.size() << " queue families:" << std::endl;
            for (size_t i = 0; i < queueFamilies.size(); ++i) {
                std::cout << "Queue Family " << i << ":" << std::endl;
                std::cout << "\tQueue Count: " << queueFamilies[i].queueCount << std::endl;
                std::cout << "\tGraphic Support: " << (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ? "Yes" : "No") << std::endl;
                std::cout << "\tCompute Support: " << (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT ? "Yes" : "No") << std::endl;
                std::cout << "\tTransfer Support: " << (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT ? "Yes" : "No") << std::endl;
                std::cout << "\tSparse Binding Support: " << (queueFamilies[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT ? "Yes" : "No") << std::endl;
                std::cout << "\tProtected Support: " << (queueFamilies[i].queueFlags & VK_QUEUE_PROTECTED_BIT ? "Yes" : "No") << std::endl;
                std::cout << "\tTimestamp Valid Bits: " << queueFamilies[i].timestampValidBits << std::endl;
                std::cout << "\tMin Image Transfer Granularity: "
                    << queueFamilies[i].minImageTransferGranularity.width << "x"
                    << queueFamilies[i].minImageTransferGranularity.height << "x"
                    << queueFamilies[i].minImageTransferGranularity.depth << std::endl;
            }
        }
  
        // ��ȡ����������豸֧�ֵ���չ����
        static void printDeviceExtensionProperties(const std::vector<VkExtensionProperties>& extensions)
        {
            // ����豸��չ��Ϣ
            std::cout << "Found " << extensions.size() << " device extensions:" << std::endl;
            for (const auto& extension : extensions) {
                std::cout << "\tExtension Name: " << extension.extensionName << std::endl;
                std::cout << "\tSpec Version: " << extension.specVersion << std::endl;
            }
        }
    
    };
}
