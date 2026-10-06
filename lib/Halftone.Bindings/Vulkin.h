#pragma once
#include <__Vendor/Vulkan/vulkan/vulkan.h>
#include <__Vendor/SDL3/SDL.h>

#include <Halftone.Core/lib.h>

#include <vector>;


using namespace Halftone::Core::Types;

namespace Halftone::Binding 
{
    class Vulkan 
    {
    public:
         Vulkan() = default;
        ~Vulkan() = default;

        bool Init(SDL_Window* window);
        void Cleanup();

        // Frame Lifecycle
        bool BeginFrame(uint32_t& outImageIndex);
        void EndFrame(uint32_t imageIndex);

        // Getters for UI and Engine integration
        auto Instance()         const { return m_Instance; }
        auto PhysicalDevice()   const { return m_PhysicalDevice; }
        auto Device()           const { return m_Device; }
        auto Queue()            const { return m_Queue; }
        auto QueueFamily()      const { return m_QueueFamily; }
        auto DescriptorPool()   const { return m_DescriptorPool; }
        auto CommandBuffer()    const { return m_CommandBuffer; }
        auto ImageCount()       const { return m_ImageCount; }
        auto SwapchainImageView(u32 index) const { return m_SwapchainImageViews[index]; }

    private:
        bool CreateInstance();
        bool SelectPhysicalDevice();
        bool CreateLogicalDevice();
        bool CreateSwapchain();
        bool CreateSyncAndCommandObjects();
        bool CreateDescriptorPool();

        VkInstance m_Instance = VK_NULL_HANDLE;
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
        VkQueue m_Queue = VK_NULL_HANDLE;
        uint32_t m_QueueFamily = 0;

        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        uint32_t m_ImageCount = 0;
        std::vector<VkImage> m_SwapchainImages;
        std::vector<VkImageView> m_SwapchainImageViews;

        VkCommandPool m_CommandPool = VK_NULL_HANDLE;
        VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

        VkSemaphore m_ImageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore m_RenderFinishedSemaphore = VK_NULL_HANDLE;
        VkFence m_InFlightFence = VK_NULL_HANDLE;

        SDL_Window* m_WindowHandle = nullptr;    };
}
