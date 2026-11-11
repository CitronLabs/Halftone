module;

export module com.citronlabs.Halftone.App.Editor.Mode:SceneLayout;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.App.UI;

export namespace Halftone::App::Editor::Modes
{
    using namespace Halftone::App::UI;

    // The scene as a whole: an empty 3D view to place canvases, models, lights and cameras.
    class SceneLayout final : public Mode {
    public:
        SceneLayout();
        bool            Accepts(Context& ctx, ID focus) const override;
        void            OnEnter(Context& ctx) override;
        void            DrawMenus(Context& ctx) override;
    };
}
