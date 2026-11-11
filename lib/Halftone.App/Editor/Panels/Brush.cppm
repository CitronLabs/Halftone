module;

export module com.citronlabs.Halftone.App.Editor.Panels:Brush;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.App.UI;

export namespace Halftone::App::Editor::Panels
{
    using namespace Halftone::App::UI;

    // Active brush preset and drawing mode settings for the Canvas 2D view.
    class Brush final : public Panel {
    public:
        const char* Name()        const override { return "Brush"; }
        PanelSlot   Slot()        const override { return PanelSlot::RightBottom; }
        int         WindowFlags() const override;
        void        Draw(Context& ctx)  override;
    };
}
