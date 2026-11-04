module;

export module com.citronlabs.Halftone.Editor:Canvas2DMode;

import com.citronlabs.Halftone.Core;
import :Context;
import :Mode;

export namespace Halftone::Editor
{
    // A canvas as a 2D animation workspace: layers, blend modes, brushes, drawing modes, exposure strip.
    class Canvas2DMode final : public EditorMode {
    public:
        Canvas2DMode();
        EditorModeKind  Kind() const override { return EditorModeKind::Canvas2D; }
        bool            Accepts(EditorContext& ctx, ID focus) const override;
        void            OnEnter(EditorContext& ctx) override;
        void            DrawMenus(EditorContext& ctx) override;
    };
}
