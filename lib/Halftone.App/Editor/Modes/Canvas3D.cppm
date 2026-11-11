module;

export module com.citronlabs.Halftone.App.Editor.Mode:Canvas3D;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.App.UI;

export namespace Halftone::App::Editor::Modes
{
    using namespace Halftone::App::UI;

    // A canvas's layers spread in 3D space, to set up multiplane depth and parallax.
    class Canvas3D final : public Mode {
    public:
        Canvas3D();
        bool            Accepts(Context& ctx, ID focus) const override;
        void            OnEnter(Context& ctx) override;
        void            DrawMenus(Context& ctx) override;
    };
}
