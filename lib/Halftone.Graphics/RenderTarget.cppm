module;
#include <__Vendor/Vulkan/vulkan/vulkan.hpp>

export module com.citronlabs.Halftone.Graphics:RenderTarget;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Bindings;

export namespace Halftone::Graphics
{
    using namespace Halftone::Core;

    struct RenderTargetDesc {
        vk::Extent2D    Extent      = { 1280, 720 };
        vk::Format      ColorFormat = vk::Format::eR8G8B8A8Unorm;
        vk::Format      DepthFormat = vk::Format::eD32Sfloat;   // eUndefined = no depth
    };

    // Offscreen colour (+ optional depth) image that a render engine draws the
    // scene into. Also registered with ImGui so the Viewport panel can show it.
    //
    // Layout contract per frame (managed by Graphics::Renderer):
    //   before RenderEngine::Render  color = eColorAttachmentOptimal, depth = eDepthAttachmentOptimal
    //   after                        color = eShaderReadOnlyOptimal (sampled by the UI)
    class RenderTarget {
    private:
        struct Attachment {
            vk::Image           Image   = VK_NULL_HANDLE;
            vk::DeviceMemory    Memory  = VK_NULL_HANDLE;
            vk::ImageView       View    = VK_NULL_HANDLE;
        };

        RenderTargetDesc        _Desc;
        Attachment              _Color;
        Attachment              _Depth;
        vk::Sampler             _Sampler    = VK_NULL_HANDLE;
        Bindings::UITexture     _UITexture  = 0;

        bool CreateAttachment(Attachment& out, vk::Format format, vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect);

    public:
        RenderTarget() = default;
        ~RenderTarget() { Destroy(); }

        RenderTarget(const RenderTarget&)            = delete;
        RenderTarget& operator=(const RenderTarget&) = delete;

        bool Create(const RenderTargetDesc& desc);
        bool Resize(vk::Extent2D extent);
        void Destroy();     // GPU objects are released once in-flight frames finish

        bool                IsValid()       const { return bool(_Color.Image); }
        bool                HasDepth()      const { return bool(_Depth.Image); }
        vk::Extent2D        Extent()        const { return _Desc.Extent; }
        vk::Format          ColorFormat()   const { return _Desc.ColorFormat; }
        vk::Format          DepthFormat()   const { return _Desc.DepthFormat; }
        vk::Image           ColorImage()    const { return _Color.Image; }
        vk::ImageView       ColorView()     const { return _Color.View; }
        vk::Image           DepthImage()    const { return _Depth.Image; }
        vk::ImageView       DepthView()     const { return _Depth.View; }
        Bindings::UITexture UITexture()     const { return _UITexture; }
    };
}
