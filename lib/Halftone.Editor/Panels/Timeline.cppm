module;

export module com.citronlabs.Halftone.Editor:TimelinePanel;

import com.citronlabs.Halftone.Core;
import :Context;
import :Panel;

export namespace Halftone::Editor
{
    // Transport and scrubbing; in canvas modes also the active layer's exposure strip (X-sheet) and cadence.
    class TimelinePanel final : public Panel {
    public:
        const char* Name() const override { return "Timeline"; }
        PanelSlot   Slot() const override { return PanelSlot::Bottom; }
        int         WindowFlags() const override;
        void        Draw(EditorContext& ctx) override;
    };
}
