module;

export module com.citronlabs.Halftone.App.Editor.Panel:Material;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.App.UI;

export namespace Halftone::App::Editor::Panels
{
    using namespace Halftone::App::UI;

    // Focused model's stylised material and texture-paint layers.
    class Material final : public Panel {
    public:
        const char* Name() const override { return "Material"; }
        PanelSlot   Slot() const override { return PanelSlot::Right; }
        int         WindowFlags() const override;
        void        Draw(Context& ctx) override;
    };
}
