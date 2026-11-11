module;
#include <utility>

export module com.citronlabs.Halftone.Scene:Camera;

import com.citronlabs.Halftone.Core;
import :Node;

export namespace Halftone::Scene
{
    using namespace Halftone::Core;

    enum class Projection : u8 { Perspective, Orthographic };

    class Camera : public Node {
    public:
        static constexpr NodeKind StaticKind = NodeKind::Camera;

        Projection  Mode            = Projection::Perspective;
        f32         FieldOfView     = 0.6981f;      // vertical, radians (40 deg)
        f32         OrthoHeight     = 10.0f;
        f32         NearClip        = 0.1f;
        f32         FarClip         = 1000.0f;

        Camera(ID id, String name) : Node(StaticKind, id, std::move(name)) {}
    };
}
