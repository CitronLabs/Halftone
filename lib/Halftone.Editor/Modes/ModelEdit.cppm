module;

export module com.citronlabs.Halftone.Editor:ModelEditMode;

import com.citronlabs.Halftone.Core;
import :Context;
import :Mode;

export namespace Halftone::Editor
{
    // A single model: stylised material and texture painting.
    class ModelEditMode final : public EditorMode {
    public:
        ModelEditMode();
        EditorModeKind  Kind() const override { return EditorModeKind::Model; }
        bool            Accepts(EditorContext& ctx, ID focus) const override;
        void            OnEnter(EditorContext& ctx) override;
        void            DrawMenus(EditorContext& ctx) override;
    };
}
