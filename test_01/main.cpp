#include <iostream>
#include <optional>
#include <array>
#include"u_vk_csy.h"

using namespace std;

#ifndef MU_SHADER_PATH
#define MU_SHADER_PATH "./shader/"
#endif

class ComputeShaderExample
{
    std::array<float, 1024> inputData;
    std::array<float, 1024> outputData;
    //������һ���ڱ���ʱ���ܼ���� inputData ������С�ĳ�������
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
        // ȷ������ʹ�õĹ������С���ᳬ���豸֧�ֵ��������
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
        binding.binding = 0;
        binding.descriptorCount = 1;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        binding.pImmutableSamplers = nullptr;
        binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo createInfo = CsySmallVk::descriptorSetLayoutCreateInfo();
        createInfo.bindingCount = 1;
        createInfo.pBindings = &binding;

        if (vkCreateDescriptorSetLayout(
            device, &createInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptorSetLayout");
    }

    VkShaderModule createShaderModule(const std::vector<char>& code)
    {
        VkShaderModule shaderModule;
        VkShaderModuleCreateInfo createInfo = CsySmallVk::shaderModuleCreateInfo();
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
            throw std::runtime_error("fail to create shader module");
        return shaderModule;
    }

    VkPipelineLayout pipelineLayout;
    VkPipeline computePipeline;
    void createComputePipeline()
    {
        auto computeShaderCode = CsySmallVk::readFile(MU_SHADER_PATH "multiply.spv");
        auto computeShaderModule = createShaderModule(computeShaderCode);
        VkPipelineShaderStageCreateInfo shaderStageCreateInfo = CsySmallVk::pipelineShaderStageCreateInfo();
        shaderStageCreateInfo.module = computeShaderModule;
        shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStageCreateInfo.pName = "main";

        VkPipelineLayoutCreateInfo layoutCreateInfo = CsySmallVk::pipelineLayoutCreateInfo();
        layoutCreateInfo.setLayoutCount = 1;
        layoutCreateInfo.pSetLayouts = &descriptorSetLayout;
        layoutCreateInfo.pushConstantRangeCount = 0;
        layoutCreateInfo.pPushConstantRanges = nullptr;
        if (vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr, &pipelineLayout)
            != VK_SUCCESS)
            throw std::runtime_error("failed to create pipeline layout!");

        VkComputePipelineCreateInfo createInfo = CsySmallVk::computePipelineCreateInfo();
        createInfo.basePipelineHandle = VK_NULL_HANDLE;
        createInfo.basePipelineIndex = -1;
        createInfo.stage = shaderStageCreateInfo;
        createInfo.layout = pipelineLayout;

        if (vkCreateComputePipelines(device, nullptr, 1, &createInfo, nullptr, &computePipeline)
            != VK_SUCCESS)
            throw std::runtime_error("failed to create compute pipeline");
        vkDestroyShaderModule(device, computeShaderModule, nullptr);
    }

    VkDescriptorPool descriptorPool;
    void createDescriptorPool()
    {
        VkDescriptorPoolSize poolSize;
        poolSize.descriptorCount = 1;
        poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

        VkDescriptorPoolCreateInfo createInfo = CsySmallVk::descriptorPoolCreateInfo();
        createInfo.poolSizeCount = 1;
        createInfo.pPoolSizes = &poolSize;
        createInfo.maxSets = 1;

        if (vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool)
            != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor pool!");
    }

    VkDescriptorSet descriptorSet;
    void createDescriptorSet()
    {
        VkDescriptorSetAllocateInfo allocInfo = CsySmallVk::descriptorSetAllocateInfo();
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;

        if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet)
            != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor set!");

        VkDescriptorBufferInfo bufferInfo;
        bufferInfo.buffer = storageBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = inputDataSize();

        VkWriteDescriptorSet write = CsySmallVk::writeDescriptorSet();
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.dstSet = descriptorSet;
        write.pBufferInfo = &bufferInfo;
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

    VkCommandPool commandPool;
    void createCommandPool()
    {
        VkCommandPoolCreateInfo createInfo = CsySmallVk::commandPoolCreateInfo();
        createInfo.queueFamilyIndex = queueFamilyIndex.value();
        if (vkCreateCommandPool(device, &createInfo, nullptr, &commandPool)
            != VK_SUCCESS)
            throw std::runtime_error("failed to create command pool!");
    }

    VkCommandBuffer commandBuffer;
    void execute()
    {
        std::cout << "input data:\n";
        for (size_t i = 0; i < inputData.size(); ++i)
        {
            if (i % 64 == 0 && i != 0) std::cout << '\n';
            std::cout << inputData[i];
        }
        std::cout << "\n";
        VkCommandBufferAllocateInfo allocInfo = CsySmallVk::commandBufferAllocateInfo();
        allocInfo.commandBufferCount = 1;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer)
            != VK_SUCCESS)
            throw std::runtime_error("failed to create command buffer!");
        
        VkCommandBufferBeginInfo beginInfo = CsySmallVk::commandBufferBeginInfo();
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout,
            0, 1, &descriptorSet, 0, nullptr);
        vkCmdDispatch(commandBuffer,
            static_cast<uint32_t>(inputData.size() / computeShaderProcessUnit), //x
            1, //y
            1  //z
        );
        vkEndCommandBuffer(commandBuffer);
        VkSubmitInfo submitInfo = CsySmallVk::submitInfo();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.signalSemaphoreCount = 0;
        if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE)
            != VK_SUCCESS)
            throw std::runtime_error("failed to submit command buffer!");

        if (vkQueueWaitIdle(queue) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to wait queue idle!");
        }
        void* data;
        vkMapMemory(device, storageBufferMemory, 0, inputDataSize(), 0, &data);
        memcpy(outputData.data(), data, inputDataSize());
        vkUnmapMemory(device, storageBufferMemory);
        for (size_t i = 0; i < outputData.size(); ++i)
        {
            if (i % 64 == 0 && i != 0) std::cout << '\n';
            std::cout << outputData[i];
        }
        std::cout << '\n';
    }

    void cleanUp()
    {
        vkDestroyCommandPool(device, commandPool, nullptr);
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyPipeline(device, computePipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        vkDestroyBuffer(device, storageBuffer, nullptr);
        vkFreeMemory(device, storageBufferMemory, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
    }

    // main logical
    void Run()
    {
        createInstance();
        pickPhyscialDevice();
        createLogicalDevice();
        createStorageBuffer();
        writeMemoryFromHost();
        createDescriptorSetLayout();
        createComputePipeline();
        createDescriptorPool();
        createDescriptorSet();
        createCommandPool();
        execute();
        cleanUp();
    }
};


int main() {
    ComputeShaderExample program;
    program.Run();
    return 0;
}
