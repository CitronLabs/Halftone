module;
#include <memory>
#include <utility>
#include <vector>

export module com.citronlabs.Halftone.App.Editor:Mode;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Graphics;
import :Context;
import :Panel;

export namespace Halftone::App::Editor
{
    using namespace Halftone::Core;

    // A focus mode inside the scene editor: its own panel set, menus and
    // viewport view. Adding a mode = new partition under Modes/ + register it
    // in SceneEditorScreen.
    class Mode {
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

        virtual const char*     Name() const { return EditorModeName(Kind()); }

        // Whether this mode can be entered with the given focus node.
        virtual bool            Accepts(Context& ctx, ID focus) const = 0;

        virtual void            OnEnter(Context& ctx) {}
        virtual void            OnExit(Context& ctx)  {}

        // Extra top-level menus for this mode (drawn in the main menu bar).
        virtual void            DrawMenus(Context& ctx) {}

        List<Unique<Panel>>&    Panels() { return _Panels; }
    };
}
