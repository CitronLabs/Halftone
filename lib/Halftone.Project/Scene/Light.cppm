module;
#include <utility>

export module com.citronlabs.Halftone.Scene:Light;

import com.citronlabs.Halftone.Core;
import :Node;

export namespace Halftone::Scene
{
    using namespace Halftone::Core;

    enum class LightKind : u8 { Directional, Point, Rim };

    // Stylised light shared by 3D environment and 2D canvases so both are lit
    // by the same rig (overview section 1).
    class Light : public Node {
    public:
        static constexpr NodeKind StaticKind = NodeKind::Light;

        LightKind   Type            = LightKind::Directional;
        Vec3        Color           = { 1.0f, 0.96f, 0.9f };
        f32         Intensity       = 1.0f;
        u32         ToonBands       = 2;        // quantisation steps for NPR shading
        bool        AffectsCanvases = true;
        bool        CastsShadows    = true;

        Light(ID id, String name) : Node(StaticKind, id, std::move(name)) {}
    };
}
