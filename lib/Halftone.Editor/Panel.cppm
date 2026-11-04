module;

export module com.citronlabs.Halftone.Editor:Panel;

import com.citronlabs.Halftone.Core;
import :Context;

export namespace Halftone::Editor
{
    using namespace Halftone::Core;

    // Where the Workspace lays a panel out. Slots that no panel uses collapse
    // so the viewport takes the space. (Stock Dear ImGui has no docking;
    // switching the vendored ImGui to the docking branch would replace this
    // with user-arrangeable dock nodes without touching panel code.)
    enum class PanelSlot : u8 { Left, Center, Right, RightBottom, Bottom };

    // One editor window. The Workspace owns Begin/End and layout; a panel
    // only draws its contents.
    class Panel {
    protected:
        bool _Open = true;

    public:
        virtual ~Panel() = default;

        virtual const char* Name() const = 0;
        virtual PanelSlot   Slot() const = 0;
        virtual void        Draw(EditorContext& ctx) = 0;

        // Extra ImGuiWindowFlags for this panel (0 = default)
        virtual int         WindowFlags() const { return 0; }

        bool  IsOpen() const    { return _Open; }
        bool& OpenFlag()        { return _Open; }
    };
}
