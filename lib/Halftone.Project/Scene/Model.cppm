module;
#include <string>
#include <utility>
#include <vector>

export module com.citronlabs.Halftone.Scene:Model;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Drawing;
import :Node;

export namespace Halftone::Scene
{
    using namespace Halftone::Core;

    // A painted layer on a model's texture (texture painting in Model mode).
    struct TexturePaintLayer {
        ID                          Id       = NullID;
        String                      Name     = "Paint";
        Halftone::Drawing::BlendMode Blend   = Halftone::Drawing::BlendMode::Normal;
        f32                         Opacity  = 1.0f;
        bool                        Visible  = true;
        ID                          Texture  = NullID;     // TODO(texture paint): GPU texture / tile store handle
    };

    // Stylised (NPR) surface description. Kept intentionally small; richer
    // material graphs can arrive as an add-on later.
    struct Material {
        String                      Name             = "Default";
        Vec4                        BaseColor        = { 0.80f, 0.78f, 0.74f, 1.0f };
        String                      BaseColorTexture;               // asset path, empty = none
        Vec4                        ShadowTint       = { 0.35f, 0.32f, 0.45f, 1.0f };
        u32                         ToonBands        = 3;
        bool                        Outline          = true;
        f32                         OutlineWidth     = 1.0f;
        List<TexturePaintLayer>     PaintLayers;
    };

    class Model : public Node {
    public:
        static constexpr NodeKind StaticKind = NodeKind::Model;

        String      MeshAsset;          // asset path inside an asset pack; empty = placeholder cube
        Material    Surface;
        bool        CastShadows    = true;
        bool        ReceiveShadows = true;

        Model(ID id, String name) : Node(StaticKind, id, std::move(name)) {}
    };
}
