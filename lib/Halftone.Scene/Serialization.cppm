module;
#include <string>

export module com.citronlabs.Halftone.Scene:Serialization;

import com.citronlabs.Halftone.Core;
import :Scene;

export namespace Halftone::Scene
{
    using namespace Halftone::Core;

    inline constexpr u32 SceneFormatVersion = 1;

    // Scene <-> JSON. The project layer stores this under "Scene" in a
    // .htscene file. Unknown keys are ignored and missing ones keep their
    // defaults, so fields can be added without breaking old files.
    json SceneToJson(const Scene& scene);
    bool SceneFromJson(const json& data, Scene& out, String* error = nullptr);
}
