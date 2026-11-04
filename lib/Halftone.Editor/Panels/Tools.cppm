module;

export module com.citronlabs.Halftone.Editor:ToolsPanel;

import com.citronlabs.Halftone.Core;
import :Context;
import :Panel;

export namespace Halftone::Editor
{
    // Mode-dependent tool strip: scene tools, drawing modes, or model tools.
    class ToolsPanel final : public Panel {
    public:
        const char* Name() const override { return "Tools"; }
        PanelSlot   Slot() const override { return PanelSlot::Left; }
        int         WindowFlags() const override;
        void        Draw(EditorContext& ctx) override;
    };
}
