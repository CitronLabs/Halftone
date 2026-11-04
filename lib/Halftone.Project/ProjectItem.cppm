module;
#include <string>
#include <vector>
#include <memory>
#include <utility>

export module com.citronlabs.Halftone.Project:ProjectItem;

import com.citronlabs.Halftone.Core;
import :Resources;

export namespace Halftone::Project
{
    using namespace Halftone::Core;

    enum class ItemKind : u8 {
        Project,        // top-level folder in the library
        Subproject,     // nested folder inside a project
        Scene,          // .htscene file - what artists actually open
    };
    const char* ItemKindName(ItemKind kind);

    class ProjectItem;

    // A resource seen from some item, with where it was defined.
    template<typename T>
    struct Scoped {
        const T*            Resource;
        const ProjectItem*  Owner;
        bool                Inherited;      // defined by a parent, not the item itself
    };

    struct ResolvedResources {
        List<Scoped<AssetPack>>     AssetPacks;
        List<Scoped<BuildSystem>>   BuildSystems;
        List<Scoped<Macro>>         Macros;
        List<Scoped<Config>>        Configs;

        // Effective value of a config key after scope overrides (inner wins).
        json ConfigValue(StringView key, json fallback = nullptr) const;
    };

    // One node of the project tree shown in the Project Browser.
    class ProjectItem {
    private:
        ItemKind                    _Kind;
        String                      _Name;
        String                      _Path;      // folder (project/subproject) or file (scene)
        ProjectItem*                _Parent = nullptr;
        List<Unique<ProjectItem>>   _Children;

    public:
        String          Description;
        ResourceSet     Resources;

        ProjectItem(ItemKind kind, String name, String path)
            : _Kind(kind), _Name(std::move(name)), _Path(std::move(path)) {}

        ProjectItem(const ProjectItem&)            = delete;
        ProjectItem& operator=(const ProjectItem&) = delete;

        ProjectItem& AddChild(Unique<ProjectItem> child);
        void         SortChildren();            // folders first, then scenes, alphabetical

        ItemKind                            Kind()      const { return _Kind; }
        const String&                       Name()      const { return _Name; }
        const String&                       Path()      const { return _Path; }
        ProjectItem*                        Parent()    const { return _Parent; }
        const List<Unique<ProjectItem>>&    Children()  const { return _Children; }

        bool  IsContainer() const { return _Kind != ItemKind::Scene; }
        const ProjectItem& RootProject() const;
        String ManifestPath() const;            // project.htproj for folders, the .htscene itself for scenes
        String DisplayPath() const;             // "Project / Sub / Scene"

        // Own resources + everything exposed by parents, outermost first.
        ResolvedResources Resolve() const;
    };
}
