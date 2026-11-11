module;
#include <memory>

export module com.citronlabs.Halftone.App.Editor:SceneEditorScreen;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.App.UI;

export namespace Halftone::App::Editor::Screens
{
    using namespace Halftone::Core;

    // The main UI once a scene is open: menu bar, a breadcrumb/mode bar and
    // the active EditorMode's panels around the viewport.
    class SceneEditorScreen final : public Screen {
    private:
        Unique<EditorMode>  _Modes[EditorModeCount];
        bool                _ShowSceneSettings = false;
        bool                _ShowImGuiDemo     = false;
        bool                _ConfirmClose      = false;

        void DrawMainMenu(Context& ctx, EditorMode& mode);
        f32  DrawModeBar(Context& ctx, f32 top);
        void DrawSceneSettings(Context& ctx);
        void DrawCloseConfirmation(Context& ctx);

    public:
        SceneEditorScreen();
        ~SceneEditorScreen() override;

        EditorMode& ModeFor(EditorModeKind kind) { return *_Modes[u32(kind)]; }

        void        Draw(Context& ctx) override;
        bool        UsesViewport() const override { return true; }
    };
}
