module;

export module com.citronlabs.Halftone.Editor:ViewportPanel;

import com.citronlabs.Halftone.Core;
import :Context;
import :Panel;

export namespace Halftone::Editor
{
    // Renders the current view (scene, canvas 2D/3D, model) and draws editor overlays: grid, node gizmos, layer planes, page frame. Handles view navigation and forwards drawing input to tools.
    class ViewportPanel final : public Panel {
    public:
        const char* Name() const override { return "Viewport"; }
        PanelSlot   Slot() const override { return PanelSlot::Center; }
        int         WindowFlags() const override;
        void        Draw(EditorContext& ctx) override;
    };
}
