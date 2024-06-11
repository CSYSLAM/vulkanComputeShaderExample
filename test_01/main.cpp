#include <iostream>
#include <optional>
#include <array>
#include"u_vk_csy.h"

using namespace std;

class ComputeShaderExample
{
    std::array<float, 1024> inputData;
    std::array<float, 1024> outputData;
    //定义了一个在编译时就能计算出 inputData 变量大小的常量函数
    constexpr VkDeviceSize inputDataSize() { return sizeof(inputData); }
    uint32_t computeShaderProcessUnit;
public:
    ComputeShaderExample()
    {
        inputData.fill(1.0f);
        outputData.fill(0.0f);
    }
    VkInstance instance;

    void createInstance()
    {
        VkApplicationInfo appInfo = CsySmallVk::applicationInfo();
        appInfo.pApplicationName = "Csy Compute Shader";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 1);
        appInfo.pEngineName = "None";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo instanceCreateInfo = CsySmallVk::instanceCreateInfo();
        instanceCreateInfo.pApplicationInfo = &appInfo;

        //get extension properties
        auto extensionProperties = CsySmallVk::Query::instanceExtensionProperties();
        CsySmallVk::LogDebug::printInstanceExtensionProperties(extensionProperties);

        VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
        if (result != VkResult::VK_SUCCESS)
            throw std::runtime_error("failed to create instance");

        std::cout << "successfully!" << std::endl;
    }

    VkPhysicalDevice physicalDevice;
    std::optional<uint32_t> queueFamilyIndex;

    void pickPhyscialDevice()
    {
        auto physicalDevices = CsySmallVk::Query::physicalDevices(instance);
        CsySmallVk::LogDebug::printPhysicalDevicesInfo(physicalDevices);
        for (const auto device : physicalDevices)
        {
            auto queueFamilies = CsySmallVk::Query::physicalDeviceQueueFamilyProperties(device);
            CsySmallVk::LogDebug::printQueueFamilyProperties(queueFamilies);
            for (size_t i = 0; i < queueFamilies.size(); ++i)
            {
                if (queueFamilies[i].queueFlags & (VK_QUEUE_COMPUTE_BIT))
                {
                    queueFamilyIndex = i;
                    physicalDevice = device;
                    break;
                }
            }
            if (queueFamilyIndex.has_value()) break;
        }
        if (!queueFamilyIndex.has_value())
            throw std::runtime_error("can't find a family that contains compute queue!");
        else
        {
            std::cout << "Select Physical Device:" << physicalDevice << std::endl;
            auto extensions = CsySmallVk::Query::deviceExtensionProperties(physicalDevice);
            CsySmallVk::LogDebug::printDeviceExtensionProperties(extensions);
            std::cout << "Select Queue Index:" << queueFamilyIndex.value() << std::endl;
        }
        auto p = CsySmallVk::Query::physicalDeviceProperties(physicalDevice);
        std::cout << "maxComputeWorkGroupInvocations:" << p.limits.maxComputeWorkGroupInvocations << std::endl;
        // 确保我们使用的工作组大小不会超过设备支持的最大限制
        computeShaderProcessUnit = std::min(p.limits.maxComputeWorkGroupInvocations, uint32_t(256));
    }

    VkDevice device;
    VkQueue queue;
    void createLogicalDevice()
    {
        VkDeviceCreateInfo createInfo = CsySmallVk::deviceCreateInfo();
        createInfo.enabledExtensionCount = 0;
        createInfo.ppEnabledExtensionNames = nullptr;
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
        createInfo.pEnabledFeatures = nullptr;

        float priority = 1.0f; //default
        VkDeviceQueueCreateInfo queueCreateInfo = CsySmallVk::deviceQueueCreateInfo();
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &priority;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex.value();

        createInfo.queueCreateInfoCount = 1;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create logical device");
        }
        vkGetDeviceQueue(device, queueFamilyIndex.value(), 0, &queue);
    }

    VkBuffer storageBuffer;
    VkDeviceMemory storageBufferMemory;

    void createStorageBuffer()
    {
        VkBufferCreateInfo createInfo = CsySmallVk::bufferCreateInfo();
        createInfo.size = inputDataSize();
        createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
        if (vkCreateBuffer(device, &createInfo, nullptr, &storageBuffer) != VK_SUCCESS)
            throw std::runtime_error("failed to create storage buffer!");
        VkMemoryRequirements requirements = CsySmallVk::Query::memoryRequirements(device, storageBuffer);
        CsySmallVk::LogDebug::printMemoryRequirements(requirements);

        VkMemoryAllocateInfo allocInfo = CsySmallVk::memoryAllocateInfo();
        allocInfo.allocationSize = requirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(requirements,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (vkAllocateMemory(device, &allocInfo, nullptr, &storageBufferMemory) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate storage buffer memory");
        vkBindBufferMemory(device, storageBuffer, storageBufferMemory, 0);
    }

    uint32_t findMemoryType(const VkMemoryRequirements& requirements, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties = CsySmallVk::Query::physicalDeviceMemoryProperties(physicalDevice);
        CsySmallVk::LogDebug::printPhysicalDeviceMemoryProperties(memProperties);
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
        {
            if (requirements.memoryTypeBits & (1 << i) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                std::cout << "pick memory type [" << i << "]\n";
                return i;
            }
        }
    }

    void writeMemoryFromHost()
    {
        void* data;
        if (vkMapMemory(device, storageBufferMemory, 0, inputDataSize(), 0, &data) != VK_SUCCESS)
            throw std::runtime_error("failed to map memory");
        memcpy(data, inputData.data(), inputDataSize());
        vkUnmapMemory(device, storageBufferMemory);
    }

    VkDescriptorSetLayout descriptorSetLayout;
    void createDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding binding;
    }

    // main logical
    void Run()
    {
        createInstance();
        pickPhyscialDevice();
        createLogicalDevice();
        createStorageBuffer();
    }
};


int main() {
    ComputeShaderExample program;
    program.Run();
    return 0;
}
