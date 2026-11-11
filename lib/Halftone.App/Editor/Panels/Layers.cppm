module;

export module com.citronlabs.Halftone.App.Editor.Panels:Layers;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.App.UI;

export namespace Halftone::App::Editor::Panels
{
    using namespace Halftone::App::UI;

    // Canvas layer stack: visibility, lock, blend mode, opacity, order - and depth placement in the Canvas 3D view.
    class Layers final : public Panel {
    public:
        const char* Name() const override { return "Layers"; }
        PanelSlot   Slot() const override { return PanelSlot::Right; }
        int         WindowFlags() const override;
        void        Draw(EditorContext& ctx) override;
    };
}
