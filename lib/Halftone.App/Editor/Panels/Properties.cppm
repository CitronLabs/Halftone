module;

export module com.citronlabs.Halftone.App.Editor.Panels:Properties;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.App.UI;

export namespace Halftone::App::Editor::Panels
{
    using namespace Halftone::App::UI;

    // Selected element's properties and the active render engine's settings.
    class Properties final : public Panel {
    public:
        const char* Name() const override { return "Properties"; }
        PanelSlot   Slot() const override { return PanelSlot::RightBottom; }
        int         WindowFlags() const override;
        void        Draw(Context& ctx) override;
    };
}
