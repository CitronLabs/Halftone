module;

export module com.citronlabs.Halftone.App.Editor.Mode:ModelEdit;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.App.UI;

export namespace Halftone::App::Editor::Modes
{
    using namespace Halftone::App::UI;

    // A single model: stylised material and texture painting.
    class ModelEdit final : public Mode {
    public:
        ModelEdit();
        bool            Accepts(Context& ctx, ID focus) const override;
        void            OnEnter(Context& ctx) override;
        void            DrawMenus(Context& ctx) override;
    };
}
