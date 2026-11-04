module;
#include <__Vendor/Vulkan/vulkan/vulkan.hpp>

export module com.citronlabs.Halftone.Graphics:RenderEngine;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Scene;
import :RenderTarget;

export namespace Halftone::Graphics
{
    using namespace Halftone::Core;

    // What the viewport is currently looking at. Each editor mode maps to one.
    enum class ViewKind : u8 {
        Scene,          // the whole 3D scene (layout)
        CanvasFlat,     // one canvas as a flat 2D page (drawing / animation)
        CanvasDepth,    // one canvas with its layers spread in 3D (multiplane)
        ModelFocus,     // one model isolated (material / texture paint)
    };

    // The editor's viewing camera - independent of the scene's shot cameras.
    struct ViewCamera {
        Vec3    Position        = { 6.0f, 4.0f, 9.0f };
        Vec3    Target          = { 0.0f, 1.0f, 0.0f };
        Vec3    Up              = { 0.0f, 1.0f, 0.0f };
        f32     FieldOfView     = 0.75f;        // radians, vertical
        f32     NearClip        = 0.05f;
        f32     FarClip         = 500.0f;
        bool    Orthographic    = false;
        f32     OrthoHeight     = 10.0f;        // visible height (world units, or page px in CanvasFlat)

        Mat4 View() const { return LookAt(Position, Target, Up); }
        Mat4 Projection(f32 aspect) const {
            return Orthographic ? Halftone::Core::Orthographic(OrthoHeight, aspect, NearClip, FarClip)
                                : Perspective(FieldOfView, aspect, NearClip, FarClip);
        }
    };

    struct RenderView {
        ViewKind    Kind    = ViewKind::Scene;
        ID          Focus   = NullID;           // canvas/model node for focused views
        ViewCamera  Camera;
    };

    // CanvasFlat convention shared by engines and editor overlays: the camera
    // Target.xy is the page-space point (pixels, origin top-left) shown at the
    // viewport centre and OrthoHeight is how many page pixels fit vertically.
    // Returns the page rectangle in viewport pixels as {x, y, width, height}.
    inline Vec4 CanvasPageRect(const ViewCamera& camera, Vec2 pageSize, Vec2 viewport) {
        const f32 scale = viewport.y / (camera.OrthoHeight > 1.0f ? camera.OrthoHeight : 1.0f);
        return { viewport.x * 0.5f - camera.Target.x * scale,
                 viewport.y * 0.5f - camera.Target.y * scale,
                 pageSize.x * scale, pageSize.y * scale };
    }

    // Everything an engine gets for one viewport render.
    struct RenderContext {
        vk::CommandBuffer               Cmd;
        RenderTarget&                   Target;
        const Halftone::Scene::Scene&   Scene;
        const RenderView&               View;
        Frame                           TimelineFrame;  // integer frame being shown
        f64                             Time;           // seconds, for continuous effects
        u32                             FrameInFlight;  // 0..Vulkan::FramesInFlight-1, for per-frame buffers
    };

    // A pluggable real-time renderer (overview section 6). Halftone ships
    // with "Chroma" (Halftone.Chroma); new NPR engines implement this
    // interface in their own Halftone.<EngineName> module and register a
    // factory with Graphics::EngineRegistry in src/App/Main.cpp.
    //
    // The Vulkan device is available through Bindings::Vulkan::GetBinding().
    // Engines record with dynamic rendering into ctx.Target; they must leave
    // the colour attachment in eColorAttachmentOptimal (the Renderer handles
    // the transition to shader-read for the UI). Engines should handle every
    // ViewKind, even if only by drawing the scene view.
    class RenderEngine {
    public:
        virtual ~RenderEngine() = default;

        virtual const char* Name() const = 0;

        virtual bool Initialize(const RenderTarget& target) = 0;
        virtual void Shutdown() = 0;

        virtual void OnTargetResized(const RenderTarget& /*target*/) {}
        virtual void Render(const RenderContext& ctx) = 0;

        // Optional: draw engine-specific settings with ImGui (shown in the
        // Properties panel under "Render Engine").
        virtual void DrawSettings() {}
    };
}
