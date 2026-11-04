module;
#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

module com.citronlabs.Halftone.Project;

import com.citronlabs.Halftone.Core;

using namespace Halftone::Project;
using namespace Halftone::Core;

const char* Halftone::Project::ItemKindName(ItemKind kind)
{
    switch (kind) {
        case ItemKind::Project:    return "Project";
        case ItemKind::Subproject: return "Subproject";
        case ItemKind::Scene:      return "Scene";
    }
    return "?";
}

ProjectItem& ProjectItem::AddChild(Unique<ProjectItem> child)
{
    child->_Parent = this;
    _Children.push_back(std::move(child));
    return *_Children.back();
}

void ProjectItem::SortChildren()
{
    std::sort(_Children.begin(), _Children.end(), [](const Unique<ProjectItem>& a, const Unique<ProjectItem>& b) {
        if (a->IsContainer() != b->IsContainer()) return a->IsContainer();
        return a->Name() < b->Name();
    });
}

const ProjectItem& ProjectItem::RootProject() const
{
    const ProjectItem* item = this;
    while (item->_Parent) item = item->_Parent;
    return *item;
}

String ProjectItem::ManifestPath() const
{
    if (_Kind == ItemKind::Scene) return _Path;
    return (std::filesystem::path(_Path) / ManifestFileName).string();
}

String ProjectItem::DisplayPath() const
{
    String path = _Name;
    for (const ProjectItem* p = _Parent; p; p = p->_Parent) path = p->_Name + " / " + path;
    return path;
}

ResolvedResources ProjectItem::Resolve() const
{
    // Outermost scope first so inner definitions appear last (and win for configs).
    List<const ProjectItem*> chain;
    for (const ProjectItem* p = this; p; p = p->_Parent) chain.insert(chain.begin(), p);

    ResolvedResources out;
    for (const ProjectItem* scope : chain) {
        const bool inherited = scope != this;
        for (auto& r : scope->Resources.AssetPacks)   out.AssetPacks.push_back  ({ &r, scope, inherited });
        for (auto& r : scope->Resources.BuildSystems) out.BuildSystems.push_back({ &r, scope, inherited });
        for (auto& r : scope->Resources.Macros)       out.Macros.push_back      ({ &r, scope, inherited });
        for (auto& r : scope->Resources.Configs)      out.Configs.push_back     ({ &r, scope, inherited });
    }
    return out;
}

json ResolvedResources::ConfigValue(StringView key, json fallback) const
{
    json result = std::move(fallback);
    const String k(key);
    for (auto& c : Configs)
        if (c.Resource->Values.contains(k)) result = c.Resource->Values.at(k);
    return result;
}
