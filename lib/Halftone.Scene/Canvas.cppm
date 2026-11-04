module;
#include <utility>

export module com.citronlabs.Halftone.Scene:Canvas;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Drawing;
import :Node;

export namespace Halftone::Scene
{
    using namespace Halftone::Core;

    // What a canvas is for. Drives sensible defaults and how the render
    // engine treats it (characters billboard and receive lighting,
    // environments may be camera-projected, ...).
    enum class CanvasRole : u8 { Character, Environment, Prop, Free };
    const char* CanvasRoleName(CanvasRole role);

    // How a canvas orients itself relative to the viewing camera.
    enum class CardFacing : u8 {
        Fixed,              // authored orientation, no billboarding
        FaceCamera,         // full billboard
        FaceCameraUpright,  // yaw-only billboard (stays vertical)
    };

    enum class CanvasMapping : u8 {
        Card,               // layers shown as planes (classic multiplane)
        CameraProjection,   // painting projected from ProjectionCamera onto proxy geometry
    };

    // A 2D drawing surface living in the 3D scene. "Honing into" a canvas
    // opens its Document in the Canvas 2D view (layers, brushes, drawing
    // modes) or the Canvas 3D view (layers placed in depth).
    //
    // Pressure points from the overview handled here:
    //   - foreshortening / perspective mismatch -> Facing + engine card pass
    //   - sliding on stepped animation          -> Anchor (pinned contact point)
    //   - lighting / shadow harmony             -> ReceiveLighting / CastShadows
    //   - 2.5D painted environments             -> Mapping + ProjectionCamera + layer depth
    class Canvas : public Node {
    public:
        static constexpr NodeKind StaticKind = NodeKind::Canvas;

        CanvasRole                  Role             = CanvasRole::Free;
        CardFacing                  Facing           = CardFacing::Fixed;
        CanvasMapping               Mapping          = CanvasMapping::Card;
        Vec2                        WorldSize        = { 3.2f, 1.8f };   // world units the page spans
        Vec2                        Anchor           = { 0.5f, 0.0f };   // contact point in page UV (0,0 = bottom-left)
        ID                          ProjectionCamera = NullID;
        bool                        ReceiveLighting  = true;
        bool                        CastShadows      = true;

        Halftone::Drawing::CanvasDocument Document;

        Canvas(ID id, String name) : Node(StaticKind, id, std::move(name)) {}

        // Role presets used by the "Add > Canvas" menu.
        void ApplyRoleDefaults(CanvasRole role);
    };
}
