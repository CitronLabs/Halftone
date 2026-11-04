module;
#include <__Vendor/Vulkan/vulkan/vulkan.hpp>
#include <string>
#include <memory>
#include <utility>

module com.citronlabs.Halftone.Graphics;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Bindings;
import com.citronlabs.Halftone.Scene;

using namespace Halftone::Graphics;
using namespace Halftone::Core;
using Halftone::Bindings::Vulkan;

bool Renderer::Initialize(StringView engineName)
{
    if (not _Viewport.Create(RenderTargetDesc { .Extent = _RequestedExtent })) {
        Log::Fatal("Graphics", "Could not create viewport render target");
        return false;
    }

    if (SetEngine(engineName)) return true;

    // Fall back to whatever is registered first.
    auto names = _Registry.Names();
    if (not names.empty() and SetEngine(names.front())) return true;

    Log::Fatal("Graphics", "No render engine could be initialized");
    return false;
}

void Renderer::Shutdown()
{
    if (_Engine) {
        Vulkan::GetBinding().WaitIdle();
        _Engine->Shutdown();
        _Engine.reset();
    }
    _Viewport.Destroy();
}

bool Renderer::SetEngine(StringView name)
{
    auto engine = _Registry.Create(name);
    if (not engine) {
        Log::Error("Graphics", "Render engine '{}' is not registered", name);
        return false;
    }

    Vulkan::GetBinding().WaitIdle();
    if (not engine->Initialize(_Viewport)) {
        Log::Error("Graphics", "Render engine '{}' failed to initialize", name);
        return false;
    }

    if (_Engine) _Engine->Shutdown();
    _Engine     = std::move(engine);
    _EngineName = String(name);
    Log::Info("Graphics", "Render engine: {}", _EngineName);
    return true;
}

void Renderer::RequestViewportSize(u32 width, u32 height)
{
    if (width  < 8) width  = 8;
    if (height < 8) height = 8;
    _RequestedExtent = vk::Extent2D { .width = width, .height = height };
}

void Renderer::BeginFrame()
{
    if (not _Viewport.IsValid() and not _Engine) return;    // not initialized yet (splash)
    auto current = _Viewport.Extent();
    if (current.width == _RequestedExtent.width and current.height == _RequestedExtent.height and _Viewport.IsValid())
        return;

    if (_Viewport.Resize(_RequestedExtent) and _Engine) _Engine->OnTargetResized(_Viewport);
}

void Renderer::RenderViewport(const Bindings::FrameContext& frame, const RenderView& view,
                              const Halftone::Scene::Scene& scene, Frame timelineFrame, f64 time)
{
    if (not _Engine or not _Viewport.IsValid()) return;

    Vulkan::TransitionImage(frame.Cmd, _Viewport.ColorImage(), vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eColorAttachmentOptimal);
    if (_Viewport.HasDepth())
        Vulkan::TransitionImage(frame.Cmd, _Viewport.DepthImage(), vk::ImageLayout::eUndefined,
                                vk::ImageLayout::eDepthAttachmentOptimal, vk::ImageAspectFlagBits::eDepth);

    RenderContext ctx {
        .Cmd            = frame.Cmd,
        .Target         = _Viewport,
        .Scene          = scene,
        .View           = view,
        .TimelineFrame  = timelineFrame,
        .Time           = time,
        .FrameInFlight  = frame.FrameIndex,
    };
    _Engine->Render(ctx);

    Vulkan::TransitionImage(frame.Cmd, _Viewport.ColorImage(), vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
}
