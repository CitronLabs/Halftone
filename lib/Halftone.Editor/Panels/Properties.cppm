module;

export module com.citronlabs.Halftone.Editor:PropertiesPanel;

import com.citronlabs.Halftone.Core;
import :Context;
import :Panel;

export namespace Halftone::Editor
{
    // Selected element's properties and the active render engine's settings.
    class PropertiesPanel final : public Panel {
    public:
        const char* Name() const override { return "Properties"; }
        PanelSlot   Slot() const override { return PanelSlot::RightBottom; }
        int         WindowFlags() const override;
        void        Draw(EditorContext& ctx) override;
    };
}
