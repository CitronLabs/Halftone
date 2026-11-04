module;
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

module com.citronlabs.Halftone.Project;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Scene;

using namespace Halftone::Project;
using namespace Halftone::Core;
namespace fs = std::filesystem;

namespace
{
    json ReadJson(const String& path)
    {
        std::ifstream file(path);
        if (!file.is_open()) return json();
        json data = json::parse(file, nullptr, /*allow_exceptions*/ false, /*ignore_comments*/ true);
        return data.is_discarded() ? json() : data;
    }

    bool WriteJson(const String& path, const json& data)
    {
        std::ofstream file(path);
        if (!file.is_open()) {
            Log::Error("Project", "Cannot write '{}'", path);
            return false;
        }
        file << data.dump(2);
        return bool(file);
    }

    json ManifestFor(const ProjectItem& item)
    {
        return {
            { "Format",      ProjectFormatVersion },
            { "Kind",        ItemKindName(item.Kind()) },
            { "Name",        item.Name() },
            { "Description", item.Description },
            { "Resources",   ResourcesToJson(item.Resources) },
        };
    }

    // Picks "<base><ext>", "<base> 2<ext>", ... that does not exist yet.
    fs::path UniquePath(const fs::path& folder, const String& base, const String& ext)
    {
        fs::path candidate = folder / (base + ext);
        for (int n = 2; fs::exists(candidate); n++) candidate = folder / (base + " " + std::to_string(n) + ext);
        return candidate;
    }
}

String ProjectLibrary::SanitizeName(StringView name)
{
    String out;
    for (char c : name) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|' || (unsigned char)c < 32)
            out += '_';
        else
            out += c;
    }
    while (!out.empty() && (out.back() == ' ' || out.back() == '.')) out.pop_back();
    return out.empty() ? String("Untitled") : out;
}

bool ProjectLibrary::Open(StringView root)
{
    _Root = String(root);
    std::error_code ec;
    fs::create_directories(_Root, ec);
    if (ec) {
        Log::Error("Project", "Cannot create library folder '{}': {}", _Root, ec.message());
        return false;
    }
    Rescan();
    Log::Info("Project", "Library '{}' - {} project(s)", _Root, _Projects.size());
    return true;
}

void ProjectLibrary::Rescan()
{
    _Projects.clear();
    std::error_code ec;
    for (auto& entry : fs::directory_iterator(_Root, ec)) {
        if (!entry.is_directory()) continue;
        if (!fs::exists(entry.path() / ManifestFileName)) continue;
        if (auto project = LoadContainer(ItemKind::Project, entry.path().string())) _Projects.push_back(std::move(project));
    }
    std::sort(_Projects.begin(), _Projects.end(), [](auto& a, auto& b) { return a->Name() < b->Name(); });
}

Unique<ProjectItem> ProjectLibrary::LoadContainer(ItemKind kind, const String& folder)
{
    json manifest = ReadJson((fs::path(folder) / ManifestFileName).string());
    String name = manifest.is_object() ? manifest.value("Name", fs::path(folder).filename().string())
                                       : fs::path(folder).filename().string();

    auto item = MakeUnique<ProjectItem>(kind, name, folder);
    if (manifest.is_object()) {
        item->Description = manifest.value("Description", String());
        item->Resources   = ResourcesFromJson(manifest.value("Resources", json::object()));
    }
    ScanFolder(*item);
    return item;
}

Unique<ProjectItem> ProjectLibrary::LoadSceneItem(const String& file)
{
    json data = ReadJson(file);
    String name = data.is_object() ? data.value("Name", fs::path(file).stem().string()) : fs::path(file).stem().string();

    auto item = MakeUnique<ProjectItem>(ItemKind::Scene, name, file);
    if (data.is_object()) {
        item->Description = data.value("Description", String());
        item->Resources   = ResourcesFromJson(data.value("Resources", json::object()));
    }
    return item;
}

void ProjectLibrary::ScanFolder(ProjectItem& parent)
{
    std::error_code ec;
    for (auto& entry : fs::directory_iterator(parent.Path(), ec)) {
        if (entry.is_directory() && fs::exists(entry.path() / ManifestFileName))
            parent.AddChild(LoadContainer(ItemKind::Subproject, entry.path().string()));
        else if (entry.is_regular_file() && entry.path().extension() == SceneExtension)
            parent.AddChild(LoadSceneItem(entry.path().string()));
    }
    parent.SortChildren();
}

ProjectItem* ProjectLibrary::CreateProject(StringView name)
{
    fs::path folder = UniquePath(_Root, SanitizeName(name), "");
    std::error_code ec;
    if (!fs::create_directories(folder, ec)) return nullptr;

    auto item = MakeUnique<ProjectItem>(ItemKind::Project, String(name), folder.string());

    // Sensible starting resources so new projects show what each section is for.
    item->Resources.Configs.push_back({ .Name = "Project Defaults", .Values = {
        { "FrameRate", { 24, 1 } }, { "Resolution", { 1920, 1080 } }, { "DefaultCadence", 2 }, { "ColorSpace", "sRGB" } } });
    item->Resources.BuildSystems.push_back({ .Name = "Preview Render", .Target = "ImageSequence",
        .Settings = { { "Format", "png" }, { "Scale", 0.5 } } });

    if (!Save(*item)) return nullptr;
    _Projects.push_back(std::move(item));
    std::sort(_Projects.begin(), _Projects.end(), [](auto& a, auto& b) { return a->Name() < b->Name(); });
    for (auto& p : _Projects) if (p->Path() == folder.string()) return p.get();
    return nullptr;
}

ProjectItem* ProjectLibrary::CreateSubproject(ProjectItem& parent, StringView name)
{
    if (!parent.IsContainer()) return nullptr;
    fs::path folder = UniquePath(parent.Path(), SanitizeName(name), "");
    std::error_code ec;
    if (!fs::create_directories(folder, ec)) return nullptr;

    auto& item = parent.AddChild(MakeUnique<ProjectItem>(ItemKind::Subproject, String(name), folder.string()));
    if (!Save(item)) return nullptr;
    parent.SortChildren();
    return FindByPath(folder.string());
}

ProjectItem* ProjectLibrary::CreateScene(ProjectItem& parent, StringView name)
{
    if (!parent.IsContainer()) return nullptr;
    fs::path file = UniquePath(parent.Path(), SanitizeName(name), SceneExtension);

    auto& item = parent.AddChild(MakeUnique<ProjectItem>(ItemKind::Scene, String(name), file.string()));
    if (!SaveScene(item, Halftone::Scene::Scene::CreateNew(String(name)))) return nullptr;
    parent.SortChildren();
    return FindByPath(file.string());
}

bool ProjectLibrary::Save(const ProjectItem& item)
{
    json manifest = ManifestFor(item);

    // Scenes keep their document in the same file; don't drop it.
    if (item.Kind() == ItemKind::Scene) {
        json existing = ReadJson(item.Path());
        if (existing.is_object() && existing.contains("Scene")) manifest["Scene"] = existing["Scene"];
    }
    return WriteJson(item.ManifestPath(), manifest);
}

bool ProjectLibrary::LoadScene(const ProjectItem& scene, Halftone::Scene::Scene& out)
{
    json data = ReadJson(scene.Path());
    if (!data.is_object() || !data.contains("Scene")) {
        Log::Error("Project", "'{}' has no scene document", scene.Path());
        return false;
    }
    String error;
    if (!Halftone::Scene::SceneFromJson(data["Scene"], out, &error)) {
        Log::Error("Project", "'{}': {}", scene.Path(), error);
        return false;
    }
    out.SetName(scene.Name());
    return true;
}

bool ProjectLibrary::SaveScene(const ProjectItem& scene, const Halftone::Scene::Scene& document)
{
    json manifest   = ManifestFor(scene);
    manifest["Scene"] = Halftone::Scene::SceneToJson(document);
    return WriteJson(scene.Path(), manifest);
}

ProjectItem* ProjectLibrary::FindByPath(StringView path)
{
    Function<ProjectItem*(ProjectItem&)> search = [&](ProjectItem& item) -> ProjectItem* {
        if (item.Path() == path) return &item;
        for (auto& child : item.Children())
            if (auto* found = search(*child)) return found;
        return nullptr;
    };
    for (auto& project : _Projects)
        if (auto* found = search(*project)) return found;
    return nullptr;
}
