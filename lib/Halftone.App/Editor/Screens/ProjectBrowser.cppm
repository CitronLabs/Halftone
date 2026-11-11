module;

export module com.citronlabs.Halftone.App.Editor.Screens:ProjectBrowser;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Project;
import com.citronlabs.Halftone.App.UI;

export namespace Halftone::App::Editor::Screens
{
    using namespace Halftone::Core;

    // Explorer-style tree of every project in the library. Projects and
    // subprojects are folders, scenes are files. Selecting an item shows its
    // settings and scoped resources; scenes also get "Open Scene"
    // (double-clicking a scene opens it too).
    class ProjectBrowser final : public Screen {
    private:
        enum class NewKind : u8 { None, Project, Subproject, Scene };

        Halftone::Project::Item*        _Selected    = nullptr;
        String                          _SelectPath;            // re-select after rescans
        bool                            _Modified    = false;

        NewKind                         _NewKind     = NewKind::None;
        Halftone::Project::Item*        _NewParent   = nullptr;
        char                            _NewName[128] = {};
        bool                            _OpenNewPopup = false;

        void DrawToolbar(Context& ctx);
        void DrawTree(Context& ctx);
        void DrawTreeItem(Context& ctx, Halftone::Project::Item& item);
        void DrawInspector(Context& ctx);
        void DrawEmptyState(Context& ctx);
        void DrawNewItemPopup(Context& ctx);
        void BeginCreate(NewKind kind, Halftone::Project::Item* parent);
        void Select(Halftone::Project::Item* item);

    public:
        void        OnEnter(Context& ctx) override;
        void        Draw(Context& ctx) override;

        void        SelectPath(StringView path) { _SelectPath = String(path); }
    };
}
