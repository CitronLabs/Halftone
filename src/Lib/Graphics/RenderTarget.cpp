module;
#include <__Vendor/Vulkan/vulkan/vulkan.hpp>
#include <functional>
#include <utility>

module com.citronlabs.Halftone.Graphics;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Bindings;

using namespace Halftone::Graphics;
using namespace Halftone::Core;
using Halftone::Bindings::Vulkan;
using Halftone::Bindings::ImGUI;

bool RenderTarget::CreateAttachment(Attachment& out, vk::Format format, vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect)
{
    auto device = Vulkan::GetBinding().Device();

    vk::ImageCreateInfo imageInfo {
        .imageType      = vk::ImageType::e2D,
        .format         = format,
        .extent         = vk::Extent3D { .width = _Desc.Extent.width, .height = _Desc.Extent.height, .depth = 1 },
        .mipLevels      = 1,
        .arrayLayers    = 1,
        .samples        = vk::SampleCountFlagBits::e1,
        .tiling         = vk::ImageTiling::eOptimal,
        .usage          = usage,
        .sharingMode    = vk::SharingMode::eExclusive,
        .initialLayout  = vk::ImageLayout::eUndefined,
    };

    auto [resImage, image] = device.createImage(imageInfo);
    if (resImage != vk::Result::eSuccess) return false;
    out.Image = image;

    auto requirements = device.getImageMemoryRequirements(out.Image);
    auto memoryType   = Vulkan::GetBinding().FindMemoryType(requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
    if (not memoryType) return false;

    vk::MemoryAllocateInfo allocInfo {
        .allocationSize  = requirements.size,
        .memoryTypeIndex = *memoryType,
    };
    // TODO(memory): swap for a pooled allocator (e.g. VMA) once there are
    // many images (raster brush tiles, painted backgrounds).
    auto [resMem, memory] = device.allocateMemory(allocInfo);
    if (resMem != vk::Result::eSuccess) return false;
    out.Memory = memory;

    if (device.bindImageMemory(out.Image, out.Memory, 0) != vk::Result::eSuccess) return false;

    vk::ImageViewCreateInfo viewInfo {
        .image              = out.Image,
        .viewType           = vk::ImageViewType::e2D,
        .format             = format,
        .subresourceRange   = { .aspectMask = aspect, .levelCount = 1, .layerCount = 1 },
    };
    auto [resView, view] = device.createImageView(viewInfo);
    if (resView != vk::Result::eSuccess) return false;
    out.View = view;

    return true;
}

bool RenderTarget::Create(const RenderTargetDesc& desc)
{
    Destroy();
    _Desc = desc;

    if (_Desc.Extent.width == 0 or _Desc.Extent.height == 0) return false;

    if (not CreateAttachment(_Color, _Desc.ColorFormat,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eTransferSrc     | vk::ImageUsageFlagBits::eTransferDst,
            vk::ImageAspectFlagBits::eColor))
    {
        Log::Error("Graphics", "Failed to create render target colour attachment");
        Destroy();
        return false;
    }

    if (_Desc.DepthFormat != vk::Format::eUndefined and
        not CreateAttachment(_Depth, _Desc.DepthFormat,
            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
            vk::ImageAspectFlagBits::eDepth))
    {
        Log::Error("Graphics", "Failed to create render target depth attachment");
        Destroy();
        return false;
    }

    vk::SamplerCreateInfo samplerInfo {
        .magFilter      = vk::Filter::eLinear,
        .minFilter      = vk::Filter::eLinear,
        .mipmapMode     = vk::SamplerMipmapMode::eLinear,
        .addressModeU   = vk::SamplerAddressMode::eClampToEdge,
        .addressModeV   = vk::SamplerAddressMode::eClampToEdge,
        .addressModeW   = vk::SamplerAddressMode::eClampToEdge,
        .maxLod         = 1.0f,
    };
    auto [resSampler, sampler] = Vulkan::GetBinding().Device().createSampler(samplerInfo);
    if (resSampler != vk::Result::eSuccess) { Destroy(); return false; }
    _Sampler = sampler;

    // Give the colour image a defined, sampleable layout before the UI ever sees it.
    Vulkan::GetBinding().ImmediateSubmit([&](vk::CommandBuffer cmd) {
        Vulkan::TransitionImage(cmd, _Color.Image, vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal);
    });

    _UITexture = ImGUI::GetBinding().AddTexture(_Sampler, _Color.View, vk::ImageLayout::eShaderReadOnlyOptimal);
    return true;
}

bool RenderTarget::Resize(vk::Extent2D extent)
{
    if (extent.width == _Desc.Extent.width and extent.height == _Desc.Extent.height and IsValid()) return true;
    RenderTargetDesc desc = _Desc;
    desc.Extent = extent;
    return Create(desc);
}

void RenderTarget::Destroy()
{
    auto& vulkan = Vulkan::GetBinding();
    if (not vulkan.Device()) return;

    // Capture by value; the frame that may still be sampling these finishes first.
    vulkan.DeferDestroy([device = vulkan.Device(), color = _Color, depth = _Depth, sampler = _Sampler, texture = _UITexture]()
    {
        if (texture) ImGUI::GetBinding().RemoveTexture(texture);
        for (const Attachment& a : { color, depth }) {
            if (a.View)   device.destroyImageView(a.View);
            if (a.Image)  device.destroyImage(a.Image);
            if (a.Memory) device.freeMemory(a.Memory);
        }
        if (sampler) device.destroySampler(sampler);
    });

    _Color     = {};
    _Depth     = {};
    _Sampler   = VK_NULL_HANDLE;
    _UITexture = 0;
}
