module;
#include <__Vendor/Vulkan/vulkan/vulkan.hpp>

export module com.citronlabs.Halftone.Chroma;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Graphics;

// Chroma - Halftone's default stylised (NPR) real-time render engine.
//
// Planned pass structure (each becomes its own partition, e.g.
// lib/Halftone.Chroma/Passes/LineArt.cppm, when implemented):
//
//   1. Environment   Environment-role canvases, card-mapped or camera-projected
//   2. Cards         Character/prop canvases: facing, anchor pinning, depth policy
//      Models        toon-shaded meshes with their texture-paint layers
//   3. Lighting      shared toon-banded lighting + card shadow casting
//   4. LineArt       stylised outlines (depth/normal/ID edges)
//   5. Composite     halftone / painterly colour passes into the target
//
export namespace Halftone::Chroma
{
    using namespace Halftone::Core;

    struct Settings {
        Vec4    SceneBackground = { 0.20f, 0.22f, 0.25f, 1.0f };   // Scene / CanvasDepth views
        Vec4    StudioBackground= { 0.30f, 0.30f, 0.32f, 1.0f };   // ModelFocus view
        Vec4    DeskColor       = { 0.13f, 0.14f, 0.16f, 1.0f };   // around the page in CanvasFlat
        Vec4    PaperColor      = { 0.92f, 0.90f, 0.85f, 1.0f };
        bool    LineArt         = true;
        f32     LineWidth       = 1.5f;
        bool    HalftoneShading = true;
        f32     HalftoneScale   = 6.0f;     // dot pitch in px
    };

    class Engine final : public Halftone::Graphics::RenderEngine {
    private:
        Settings _Settings;

    public:
        static constexpr const char* EngineName = "Chroma";

        const char* Name() const override { return EngineName; }

        bool Initialize(const Halftone::Graphics::RenderTarget& target) override;
        void Shutdown() override;

        void OnTargetResized(const Halftone::Graphics::RenderTarget& target) override;
        void Render(const Halftone::Graphics::RenderContext& ctx) override;
        void DrawSettings() override;

        Settings&       GetSettings()       { return _Settings; }
        const Settings& GetSettings() const { return _Settings; }
    };
}
