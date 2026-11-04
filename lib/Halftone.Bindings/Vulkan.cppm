module;
#include <__Vendor/SDL3/SDL.h>
#include <__Vendor/Vulkan/vulkan/vulkan.hpp>

export module com.citronlabs.Halftone.Bindings:Vulkan;

import com.citronlabs.Halftone.Core;

export namespace Halftone::Bindings
{
    using namespace Halftone::Core;

    // Everything a renderer needs to record into the current frame.
    struct FrameContext {
        vk::CommandBuffer   Cmd;
        vk::Image           SwapchainImage;
        vk::ImageView       SwapchainView;
        vk::Extent2D        Extent;
        u32                 ImageIndex  = 0;    // swapchain image
        u32                 FrameIndex  = 0;    // frame-in-flight slot
    };

    struct VulkanDesc {
        bool VSync      = true;
        bool Validation =
#ifdef HALFTONE_DEBUG
            true;
#else
            false;
#endif
    };

    class Vulkan {
    public:
        static constexpr u32 FramesInFlight = 2;

    private:
        struct FrameSync {
            vk::CommandPool             CommandPool;
            vk::CommandBuffer           CommandBuffer;
            vk::Semaphore               ImageAvailable;
            vk::Fence                   InFlight;
            List<Function<void()>>      DeletionQueue;   // run once this slot's fence signals
        };

        vk::Instance        _Instance           = VK_NULL_HANDLE;
        vk::PhysicalDevice  _PhysicalDevice     = VK_NULL_HANDLE;
        vk::Device          _Device             = VK_NULL_HANDLE;
        vk::Queue           _Queue              = VK_NULL_HANDLE;
        u32                 _QueueFamily        = 0;

        vk::SurfaceKHR      _Surface            = VK_NULL_HANDLE;
        vk::SwapchainKHR    _Swapchain          = VK_NULL_HANDLE;
        vk::Format          _SwapchainFormat    = vk::Format::eB8G8R8A8Unorm;
        vk::ColorSpaceKHR   _SwapchainColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
        vk::Extent2D        _SwapchainExtent    = { 0, 0 };
        u32                 _ImageCount         = 0;
        u32                 _MinImageCount      = 2;
        List<vk::Image>     _SwapchainImages;
        List<vk::ImageView> _SwapchainImageViews;
        List<vk::Semaphore> _RenderFinishedSemaphores;  // one per swapchain image
        bool                _SwapchainDirty     = false;

        vk::CommandPool     _ImmediatePool      = VK_NULL_HANDLE;
        vk::DescriptorPool  _DescriptorPool     = VK_NULL_HANDLE;

        FrameSync           _Frames[FramesInFlight];
        u32                 _CurrentFrame       = 0;

        VkDebugUtilsMessengerEXT _DebugMessenger = VK_NULL_HANDLE;
        VulkanDesc          _Desc;

        SDL_Window*         _Window  = nullptr;

        Vulkan() = default;
        ~Vulkan();

        bool CreateInstance();
        bool SelectPhysicalDevice();
        bool CreateLogicalDevice();
        bool CreateSwapchain();
        void DestroySwapchain(bool keepHandle);
        bool CreateSyncAndCommandObjects();
        bool CreateDescriptorPool();

    public:

        // Delete copy constructor and copy assignment operator
        Vulkan(const Vulkan&)               = delete;
        Vulkan& operator=(const Vulkan&)    = delete;

        // Delete move constructor and move assignment operator
        Vulkan(Vulkan&&)                    = delete;
        Vulkan& operator=(Vulkan&&)         = delete;

        static Vulkan& GetBinding() {
            static Vulkan binding;
            return binding;
        }

        // Requires SDL::GetBinding().Initialize() to have succeeded.
        bool Initialize(const VulkanDesc& desc = {});
        void Shutdown();

        // Frame Lifecycle
        //   BeginFrame -> (record offscreen work) -> BeginSwapchainPass ->
        //   (UI) -> EndSwapchainPass -> EndFrame
        // BeginFrame returns false when there is nothing to draw into
        // (minimised, swapchain being rebuilt); just skip the frame.
        bool BeginFrame(FrameContext& outFrame);
        void BeginSwapchainPass(const FrameContext& frame, Vec4 clearColor);
        void EndSwapchainPass(const FrameContext& frame);
        void EndFrame(const FrameContext& frame);

        void RequestSwapchainRebuild() { _SwapchainDirty = true; }
        bool RebuildSwapchain();
        void SetVSync(bool enabled)    { if (_Desc.VSync != enabled) { _Desc.VSync = enabled; _SwapchainDirty = true; } }

        // Helpers
        void WaitIdle() const;
        bool ImmediateSubmit(const Function<void(vk::CommandBuffer)>& record);
        void DeferDestroy(Function<void()> destroy);   // after the GPU is done with the current frame
        void FlushDeferred();                          // wait idle + run every pending DeferDestroy now
        Optional<u32> FindMemoryType(u32 typeBits, vk::MemoryPropertyFlags properties) const;

        static void TransitionImage(vk::CommandBuffer cmd, vk::Image image,
                                    vk::ImageLayout from, vk::ImageLayout to,
                                    vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor);

        // Getters for UI and Engine integration
        auto Instance()         const { return _Instance; }
        auto PhysicalDevice()   const { return _PhysicalDevice; }
        auto Device()           const { return _Device; }
        auto Queue()            const { return _Queue; }
        auto QueueFamily()      const { return _QueueFamily; }
        auto DescriptorPool()   const { return _DescriptorPool; }
        auto CommandBuffer()    const { return _Frames[_CurrentFrame].CommandBuffer; }
        auto ImageCount()       const { return _ImageCount; }
        auto MinImageCount()    const { return _MinImageCount; }
        auto SwapchainFormat()  const { return _SwapchainFormat; }
        auto SwapchainExtent()  const { return _SwapchainExtent; }
        auto SwapchainImageView(u32 index) const { return _SwapchainImageViews[index]; }
    };
}
