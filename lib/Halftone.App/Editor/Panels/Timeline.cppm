module;

export module com.citronlabs.Halftone.App.Editor.Panels:Timeline;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.App.UI;

export namespace Halftone::App::Editor::Panels
{
    using namespace Halftone::App::UI;

    // Transport and scrubbing; in canvas modes also the active layer's exposure strip (X-sheet) and cadence.
    class Timeline final : public Panel {
    public:
        const char* Name() const override { return "Timeline"; }
        PanelSlot   Slot() const override { return PanelSlot::Bottom; }
        int         WindowFlags() const override;
        void        Draw(Context& ctx) override;
    };
}
