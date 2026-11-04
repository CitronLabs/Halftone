module;

export module com.citronlabs.Halftone.Editor:BrushPanel;

import com.citronlabs.Halftone.Core;
import :Context;
import :Panel;

export namespace Halftone::Editor
{
    // Active brush preset and drawing mode settings for the Canvas 2D view.
    class BrushPanel final : public Panel {
    public:
        const char* Name() const override { return "Brush"; }
        PanelSlot   Slot() const override { return PanelSlot::RightBottom; }
        int         WindowFlags() const override;
        void        Draw(EditorContext& ctx) override;
    };
}
