module;

export module com.citronlabs.Halftone.Editor:LayersPanel;

import com.citronlabs.Halftone.Core;
import :Context;
import :Panel;

export namespace Halftone::Editor
{
    // Canvas layer stack: visibility, lock, blend mode, opacity, order - and depth placement in the Canvas 3D view.
    class LayersPanel final : public Panel {
    public:
        const char* Name() const override { return "Layers"; }
        PanelSlot   Slot() const override { return PanelSlot::Right; }
        int         WindowFlags() const override;
        void        Draw(EditorContext& ctx) override;
    };
}
