module;

export module com.citronlabs.Halftone.App.Editor.Panels:Tools;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.App.UI;

export namespace Halftone::App::Editor::Panels
{
    using namespace Halftone::App::UI;

    // Mode-dependent tool strip: scene tools, drawing modes, or model tools.
    class Tools final : public Panel {
    public:
        const char* Name() const override { return "Tools"; }
        PanelSlot   Slot() const override { return PanelSlot::Left; }
        int         WindowFlags() const override;
        void        Draw(Context& ctx) override;
    };
}
