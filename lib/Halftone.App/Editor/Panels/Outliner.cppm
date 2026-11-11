module;

export module com.citronlabs.Halftone.App.Editor.Panels:Outliner;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.App.UI;

export namespace Halftone::App::Editor::Panels
{
    using namespace Halftone::App::UI;

    // Scene element list. Double-click a canvas or model to hone into it.
    class Outliner final : public Panel {
    public:
        const char* Name() const override { return "Outliner"; }
        PanelSlot   Slot() const override { return PanelSlot::Right; }
        int         WindowFlags() const override;
        void        Draw(Context& ctx) override;
    };
}
