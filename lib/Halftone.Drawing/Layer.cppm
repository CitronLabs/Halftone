module;
#include <string>
#include <vector>

export module com.citronlabs.Halftone.Drawing:Layer;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Animation;
import :Drawing;

export namespace Halftone::Drawing
{
    using namespace Halftone::Core;

    enum class BlendMode : u8 { Normal, Multiply, Screen, Overlay, Add, Darken, Lighten, SoftLight };
    inline constexpr u32 BlendModeCount = 8;
    const char* BlendModeName(BlendMode mode);

    enum class LayerKind : u8 {
        Raster,     // painted pixels (raster brush engine)
        Vector,     // editable strokes
    };
    const char* LayerKindName(LayerKind kind);

    // One layer of a canvas. Besides the usual 2D properties a layer has a
    // placement in the canvas's local 3D space (DepthTransform) - that is
    // what the Canvas 3D view edits, turning a flat layer stack into a
    // multiplane setup (overview sections 1 and 2).
    struct Layer {
        ID                          Id              = NullID;
        String                      Name            = "Layer";
        LayerKind                   Kind            = LayerKind::Raster;
        BlendMode                   Blend           = BlendMode::Normal;
        f32                         Opacity         = 1.0f;
        bool                        Visible         = true;
        bool                        Locked          = false;

        Transform                   DepthTransform;     // canvas-local; Position.z = depth offset

        Animation::ExposureSheet    Exposures;          // frame -> Drawing::Id
        List<Drawing>               Drawings;

        Drawing*        FindDrawing(ID id);
        // Drawing visible at `frame` (creates none).
        Drawing*        DrawingAt(Core::Frame frame);
    };
}
