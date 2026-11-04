module;
#include <memory>

export module com.citronlabs.Halftone.Editor:SceneEditorScreen;

import com.citronlabs.Halftone.Core;
import :Context;
import :Mode;
import :Screen;

export namespace Halftone::Editor
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

        void DrawMainMenu(EditorContext& ctx, EditorMode& mode);
        f32  DrawModeBar(EditorContext& ctx, f32 top);
        void DrawSceneSettings(EditorContext& ctx);
        void DrawCloseConfirmation(EditorContext& ctx);

    public:
        SceneEditorScreen();
        ~SceneEditorScreen() override;

        EditorMode& ModeFor(EditorModeKind kind) { return *_Modes[u32(kind)]; }

        ScreenKind  Kind() const override { return ScreenKind::SceneEditor; }
        void        Draw(EditorContext& ctx) override;
        bool        UsesViewport() const override { return true; }
    };
}
