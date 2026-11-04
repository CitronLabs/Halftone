module;
#include <memory>
#include <utility>
#include <vector>

export module com.citronlabs.Halftone.Editor:Mode;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Graphics;
import :Context;
import :Panel;

export namespace Halftone::Editor
{
    using namespace Halftone::Core;

    // A focus mode inside the scene editor: its own panel set, menus and
    // viewport view. Adding a mode = new partition under Modes/ + register it
    // in SceneEditorScreen.
    class EditorMode {
    protected:
        List<Unique<Panel>> _Panels;

        template<typename T, typename... Args>
        T& AddPanel(Args&&... args) {
            auto panel = MakeUnique<T>(std::forward<Args>(args)...);
            T&   ref   = *panel;
            _Panels.push_back(std::move(panel));
            return ref;
        }

    public:
        virtual ~EditorMode() = default;

        virtual EditorModeKind  Kind() const = 0;
        virtual const char*     Name() const { return EditorModeName(Kind()); }

        // Whether this mode can be entered with the given focus node.
        virtual bool            Accepts(EditorContext& ctx, ID focus) const = 0;

        virtual void            OnEnter(EditorContext& /*ctx*/) {}
        virtual void            OnExit(EditorContext& /*ctx*/)  {}

        // Extra top-level menus for this mode (drawn in the main menu bar).
        virtual void            DrawMenus(EditorContext& /*ctx*/) {}

        List<Unique<Panel>>&    Panels() { return _Panels; }
    };
}
