module;

export module com.citronlabs.Halftone.App.Editor:Screen;

import com.citronlabs.Halftone.Core;
import :Context;

export namespace Halftone::App::Editor
{
    using namespace Halftone::Core;

    // A full-window stage of the app: Splash -> Project Browser -> Scene Editor.
    class Screen {
    public:
        virtual ~Screen() = default;

        virtual void        OnEnter(Context& /*ctx*/) {}
        virtual void        OnExit(Context& /*ctx*/)  {}
        virtual void        Update(Context& /*ctx*/, f64 /*dt*/) {}
        virtual void        Draw(Context& ctx) = 0;

        // True if the 3D viewport should be rendered this frame.
        virtual bool        UsesViewport() const { return false; }
    };
}
