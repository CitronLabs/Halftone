module;

export module com.citronlabs.Halftone.Editor:MaterialPanel;

import com.citronlabs.Halftone.Core;
import :Context;
import :Panel;

export namespace Halftone::Editor
{
    // Focused model's stylised material and texture-paint layers.
    class MaterialPanel final : public Panel {
    public:
        const char* Name() const override { return "Material"; }
        PanelSlot   Slot() const override { return PanelSlot::Right; }
        int         WindowFlags() const override;
        void        Draw(EditorContext& ctx) override;
    };
}
