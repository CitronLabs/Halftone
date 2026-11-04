module;
#include <fstream>
#include <filesystem>
#include <string>

module com.citronlabs.Halftone.App;

import com.citronlabs.Halftone.Core;

using namespace Halftone::App;
using namespace Halftone::Core;

using JSON = Halftone::Core::json;

namespace
{
    const JSON Default_Preferences = R"(
    {
        "StartUp": {
            "OpenLastProject": false
        },
        "Window": {
            "Width":  1600,
            "Height": 900,
            "VSync":  true
        },
        "Interface": {
            "Scale": 1.0,
            "Theme": "Dark"
        },
        "Project": {
            "FrameRate": { "Numerator": 24, "Denominator": 1 },
            "DefaultCadence": 2
        },
        "Rendering": {
            "Engine": "Chroma"
        },
        "Projects": {
            "Library": null
        }
    }
    )"_json;
}

Preferences::Preferences() : _Data(Default_Preferences) {}

const JSON& Preferences::Defaults() { return Default_Preferences; }

bool Preferences::Load(StringView path)
{
    _Path = String(path);
    _Data = Default_Preferences;

    std::ifstream file(_Path);
    if (not file.is_open()) {
        Log::Info("Preferences", "No preferences at '{}', writing defaults", _Path);
        Save();
        return true;
    }

    JSON user = JSON::parse(file, nullptr, /*allow_exceptions*/ false, /*ignore_comments*/ true);
    if (user.is_discarded()) {
        Log::Warn("Preferences", "'{}' is not valid JSON; using defaults", _Path);
        return false;
    }

    _Data.merge_patch(user);   // user values win, defaults fill the gaps
    Log::Info("Preferences", "Loaded '{}'", _Path);
    return true;
}

bool Preferences::Save() const
{
    if (_Path.empty()) return false;

    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(_Path).parent_path(), ec);

    std::ofstream file(_Path);
    if (not file.is_open()) {
        Log::Error("Preferences", "Cannot write '{}'", _Path);
        return false;
    }
    file << _Data.dump(4);
    return true;
}
