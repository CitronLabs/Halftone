module;

export module com.citronlabs.Halftone.Editor:Canvas3DMode;

import com.citronlabs.Halftone.Core;
import :Context;
import :Mode;

export namespace Halftone::Editor
{
    // A canvas's layers spread in 3D space, to set up multiplane depth and parallax.
    class Canvas3DMode final : public EditorMode {
    public:
        Canvas3DMode();
        EditorModeKind  Kind() const override { return EditorModeKind::Canvas3D; }
        bool            Accepts(EditorContext& ctx, ID focus) const override;
        void            OnEnter(EditorContext& ctx) override;
        void            DrawMenus(EditorContext& ctx) override;
    };
}
