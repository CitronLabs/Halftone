module;
#include <string>
#include <vector>
#include <utility>

module com.citronlabs.Halftone.Project;

import com.citronlabs.Halftone.Core;

using namespace Halftone::Project;
using namespace Halftone::Core;

const char* Halftone::Project::ResourceKindName(ResourceKind kind)
{
    switch (kind) {
        case ResourceKind::AssetPack:   return "Asset Packs";
        case ResourceKind::BuildSystem: return "Build Systems";
        case ResourceKind::Macro:       return "Macros";
        case ResourceKind::Config:      return "Configs";
    }
    return "?";
}

usize ResourceSet::Count(ResourceKind kind) const
{
    switch (kind) {
        case ResourceKind::AssetPack:   return AssetPacks.size();
        case ResourceKind::BuildSystem: return BuildSystems.size();
        case ResourceKind::Macro:       return Macros.size();
        case ResourceKind::Config:      return Configs.size();
    }
    return 0;
}

json Halftone::Project::ResourcesToJson(const ResourceSet& set)
{
    json packs = json::array(), builds = json::array(), macros = json::array(), configs = json::array();
    for (auto& p : set.AssetPacks)   packs.push_back  ({ { "Name", p.Name }, { "Description", p.Description }, { "Assets", p.Assets } });
    for (auto& b : set.BuildSystems) builds.push_back ({ { "Name", b.Name }, { "Target", b.Target }, { "Settings", b.Settings } });
    for (auto& m : set.Macros)       macros.push_back ({ { "Name", m.Name }, { "Steps", m.Steps } });
    for (auto& c : set.Configs)      configs.push_back({ { "Name", c.Name }, { "Values", c.Values } });
    return { { "AssetPacks", packs }, { "BuildSystems", builds }, { "Macros", macros }, { "Configs", configs } };
}

ResourceSet Halftone::Project::ResourcesFromJson(const json& data)
{
    ResourceSet set;
    if (!data.is_object()) return set;

    for (auto& j : data.value("AssetPacks", json::array())) {
        AssetPack p;
        p.Name        = j.value("Name", p.Name);
        p.Description = j.value("Description", String());
        p.Assets      = j.value("Assets", List<String>());
        set.AssetPacks.push_back(std::move(p));
    }
    for (auto& j : data.value("BuildSystems", json::array())) {
        BuildSystem b;
        b.Name     = j.value("Name", b.Name);
        b.Target   = j.value("Target", b.Target);
        b.Settings = j.value("Settings", json::object());
        set.BuildSystems.push_back(std::move(b));
    }
    for (auto& j : data.value("Macros", json::array())) {
        Macro m;
        m.Name  = j.value("Name", m.Name);
        m.Steps = j.value("Steps", List<String>());
        set.Macros.push_back(std::move(m));
    }
    for (auto& j : data.value("Configs", json::array())) {
        Config c;
        c.Name   = j.value("Name", c.Name);
        c.Values = j.value("Values", json::object());
        set.Configs.push_back(std::move(c));
    }
    return set;
}
