module;

export module com.citronlabs.Halftone.App.Editor.Mode:Canvas2D;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.App.UI;

export namespace Halftone::App::Editor::Modes
{
    using namespace Halftone::App::UI;

    // A canvas as a 2D animation workspace: layers, blend modes, brushes, drawing modes, exposure strip.
    class Canvas2D final : public Mode {
    public:
        Canvas2D();
        bool            Accepts(Context& ctx, ID focus) const override;
        void            OnEnter(Context& ctx) override;
        void            DrawMenus(Context& ctx) override;
    };
}
