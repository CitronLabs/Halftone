module;

export module com.citronlabs.Halftone.Editor:Screen;

import com.citronlabs.Halftone.Core;
import :Context;

export namespace Halftone::Editor
{
    using namespace Halftone::Core;

    enum class ScreenKind : u8 { Splash, ProjectBrowser, SceneEditor };

    // A full-window stage of the app: Splash -> Project Browser -> Scene Editor.
    class Screen {
    public:
        virtual ~Screen() = default;

        virtual ScreenKind  Kind() const = 0;
        virtual void        OnEnter(EditorContext& /*ctx*/) {}
        virtual void        OnExit(EditorContext& /*ctx*/)  {}
        virtual void        Update(EditorContext& /*ctx*/, f64 /*dt*/) {}
        virtual void        Draw(EditorContext& ctx) = 0;

        // True if the 3D viewport should be rendered this frame.
        virtual bool        UsesViewport() const { return false; }
    };
}
