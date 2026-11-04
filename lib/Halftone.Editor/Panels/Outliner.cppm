module;

export module com.citronlabs.Halftone.Editor:OutlinerPanel;

import com.citronlabs.Halftone.Core;
import :Context;
import :Panel;

export namespace Halftone::Editor
{
    // Scene element list. Double-click a canvas or model to hone into it.
    class OutlinerPanel final : public Panel {
    public:
        const char* Name() const override { return "Outliner"; }
        PanelSlot   Slot() const override { return PanelSlot::Right; }
        int         WindowFlags() const override;
        void        Draw(EditorContext& ctx) override;
    };
}
