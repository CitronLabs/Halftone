module;
#include <string>
#include <vector>

export module com.citronlabs.Halftone.Project:Resources;

import com.citronlabs.Halftone.Core;

// Things a project, subproject or scene can define and expose to everything
// inside it. A scene sees its own resources plus everything defined by the
// subprojects and project above it (see ProjectItem::Resolve).
//
// Each type keeps a free-form `Settings`/`Values` JSON blob so features can
// add fields without changing the file format.
export namespace Halftone::Project
{
    using namespace Halftone::Core;

    enum class ResourceKind : u8 { AssetPack, BuildSystem, Macro, Config };
    inline constexpr u32 ResourceKindCount = 4;
    const char* ResourceKindName(ResourceKind kind);        // "Asset Packs", ...

    // A named bundle of reusable assets (brushes, canvases, models,
    // materials, palettes). Paths are relative to the owning project folder.
    struct AssetPack {
        String          Name        = "Asset Pack";
        String          Description;
        List<String>    Assets;
    };

    // How finished shots leave Halftone (image sequence, video, layered
    // export...). `Target` names the exporter that will run it.
    struct BuildSystem {
        String  Name        = "Build";
        String  Target      = "ImageSequence";
        json    Settings    = json::object();
    };

    // A recorded sequence of editor commands that can be replayed.
    struct Macro {
        String          Name    = "Macro";
        List<String>    Steps;          // command identifiers, executed in order
    };

    // Key/value overrides (frame rate, resolution, colour space, default
    // cadence...). Inner scopes override outer ones key by key.
    struct Config {
        String  Name    = "Config";
        json    Values  = json::object();
    };

    struct ResourceSet {
        List<AssetPack>     AssetPacks;
        List<BuildSystem>   BuildSystems;
        List<Macro>         Macros;
        List<Config>        Configs;

        usize Count(ResourceKind kind) const;
        bool  Empty() const { return AssetPacks.empty() and BuildSystems.empty() and Macros.empty() and Configs.empty(); }
    };

    json        ResourcesToJson(const ResourceSet& set);
    ResourceSet ResourcesFromJson(const json& data);
}
