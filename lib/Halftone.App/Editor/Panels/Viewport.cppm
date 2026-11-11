module;

export module com.citronlabs.Halftone.App.Editor.Panels:Viewport;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.App.UI;

export namespace Halftone::App::Editor::Panels
{
    using namespace Halftone::App::UI;

    // Renders the current view (scene, canvas 2D/3D, model) and draws editor overlays: 
    // grid, node gizmos, layer planes, page frame. Handles view navigation and forwards 
    // drawing input to tools.

    class Viewport final : public Panel {
    public:
        const char* Name() const override { return "Viewport"; }
        PanelSlot   Slot() const override { return PanelSlot::Center; }
        int         WindowFlags() const override;
        void        Draw(Context& ctx) override;
    };
}
