module;
#include <string>
#include <vector>
#include <memory>

export module com.citronlabs.Halftone.Project:Library;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Scene;
import :Resources;
import :ProjectItem;

export namespace Halftone::Project
{
    using namespace Halftone::Core;

    inline constexpr const char* ManifestFileName = "project.htproj";
    inline constexpr const char* SceneExtension   = ".htscene";
    inline constexpr u32         ProjectFormatVersion = 1;

    // The folder that holds all of a studio's projects, mirrored as a tree.
    //
    //   <Root>/
    //     Spring Short/                  project      (project.htproj)
    //       Sequence 01/                 subproject   (project.htproj)
    //         Shot 010.htscene           scene
    //       Lookdev.htscene              scene
    class ProjectLibrary {
    private:
        String                      _Root;
        List<Unique<ProjectItem>>   _Projects;

        void ScanFolder(ProjectItem& parent);
        Unique<ProjectItem> LoadContainer(ItemKind kind, const String& folder);
        Unique<ProjectItem> LoadSceneItem(const String& file);

    public:
        // Creates the folder if needed, then scans it.
        bool Open(StringView root);
        void Rescan();

        ProjectItem* CreateProject(StringView name);
        ProjectItem* CreateSubproject(ProjectItem& parent, StringView name);
        ProjectItem* CreateScene(ProjectItem& parent, StringView name);

        // Writes an item's manifest (name, description, resources). For a
        // scene this preserves the stored scene document.
        bool Save(const ProjectItem& item);

        bool LoadScene(const ProjectItem& scene, Halftone::Scene::Scene& out);
        bool SaveScene(const ProjectItem& scene, const Halftone::Scene::Scene& document);

        ProjectItem* FindByPath(StringView path);

        const String&                       Root()      const { return _Root; }
        const List<Unique<ProjectItem>>&    Projects()  const { return _Projects; }

        // File-system-safe version of a display name.
        static String SanitizeName(StringView name);
    };
}
