module;
#include <__Vendor/Vulkan/vulkan/vulkan.hpp>
#include <string>
#include <memory>

export module com.citronlabs.Halftone.Graphics:Renderer;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Bindings;
import com.citronlabs.Halftone.Scene;
import :RenderTarget;
import :RenderEngine;
import :EngineRegistry;

export namespace Halftone::Graphics
{
    using namespace Halftone::Core;

    // Engine-agnostic frontend: owns the viewport RenderTarget and the active
    // RenderEngine, and handles layout transitions around engine work.
    class Renderer {
    private:
        const EngineRegistry&   _Registry;
        Unique<RenderEngine>    _Engine;
        String                  _EngineName;
        RenderTarget            _Viewport;
        vk::Extent2D            _RequestedExtent = { 1280, 720 };

    public:
        explicit Renderer(const EngineRegistry& registry) : _Registry(registry) {}
        ~Renderer() { Shutdown(); }

        Renderer(const Renderer&)            = delete;
        Renderer& operator=(const Renderer&) = delete;

        bool Initialize(StringView engineName);
        void Shutdown();

        bool SetEngine(StringView name);

        // Viewport panel reports its size every frame; applied in BeginFrame.
        void RequestViewportSize(u32 width, u32 height);

        // Call once per frame before the UI is built (applies resizes so the
        // UI references this frame's texture).
        void BeginFrame();

        // Records the active engine's work into frame.Cmd. Must be called
        // between Vulkan::BeginFrame and Vulkan::BeginSwapchainPass.
        void RenderViewport(const Bindings::FrameContext& frame, const RenderView& view,
                            const Halftone::Scene::Scene& scene, Frame timelineFrame, f64 time);

        bool                    IsInitialized() const { return bool(_Engine); }

        RenderEngine*           Engine()        { return _Engine.get(); }
        const String&           EngineName()    const { return _EngineName; }
        const EngineRegistry&   Registry()      const { return _Registry; }
        const RenderTarget&     Viewport()      const { return _Viewport; }
    };
}
