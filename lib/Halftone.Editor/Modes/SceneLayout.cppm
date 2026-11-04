module;

export module com.citronlabs.Halftone.Editor:SceneLayoutMode;

import com.citronlabs.Halftone.Core;
import :Context;
import :Mode;

export namespace Halftone::Editor
{
    // The scene as a whole: an empty 3D view to place canvases, models, lights and cameras.
    class SceneLayoutMode final : public EditorMode {
    public:
        SceneLayoutMode();
        EditorModeKind  Kind() const override { return EditorModeKind::Scene; }
        bool            Accepts(EditorContext& ctx, ID focus) const override;
        void            OnEnter(EditorContext& ctx) override;
        void            DrawMenus(EditorContext& ctx) override;
    };
}
