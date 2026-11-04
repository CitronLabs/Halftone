module;

export module com.citronlabs.Halftone.Editor:ProjectBrowserScreen;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Project;
import :Context;
import :Screen;

export namespace Halftone::Editor
{
    using namespace Halftone::Core;

    // Explorer-style tree of every project in the library. Projects and
    // subprojects are folders, scenes are files. Selecting an item shows its
    // settings and scoped resources; scenes also get "Open Scene"
    // (double-clicking a scene opens it too).
    class ProjectBrowserScreen final : public Screen {
    private:
        enum class NewKind : u8 { None, Project, Subproject, Scene };

        Halftone::Project::ProjectItem* _Selected    = nullptr;
        String                          _SelectPath;            // re-select after rescans
        bool                            _Modified    = false;

        NewKind                         _NewKind     = NewKind::None;
        Halftone::Project::ProjectItem* _NewParent   = nullptr;
        char                            _NewName[128] = {};
        bool                            _OpenNewPopup = false;

        void DrawToolbar(EditorContext& ctx);
        void DrawTree(EditorContext& ctx);
        void DrawTreeItem(EditorContext& ctx, Halftone::Project::ProjectItem& item);
        void DrawInspector(EditorContext& ctx);
        void DrawEmptyState(EditorContext& ctx);
        void DrawNewItemPopup(EditorContext& ctx);
        void BeginCreate(NewKind kind, Halftone::Project::ProjectItem* parent);
        void Select(Halftone::Project::ProjectItem* item);

    public:
        ScreenKind  Kind() const override { return ScreenKind::ProjectBrowser; }
        void        OnEnter(EditorContext& ctx) override;
        void        Draw(EditorContext& ctx) override;

        void        SelectPath(StringView path) { _SelectPath = String(path); }
    };
}
