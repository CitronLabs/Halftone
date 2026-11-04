module;
#include <vector>

export module com.citronlabs.Halftone.Drawing:Stroke;

import com.citronlabs.Halftone.Core;

export namespace Halftone::Drawing
{
    using namespace Halftone::Core;

    // Raw stylus sample in canvas space.
    struct StrokePoint {
        Vec2    Position;
        f32     Pressure = 1.0f;
        f32     TiltX    = 0.0f;
        f32     TiltY    = 0.0f;
        f64     Time     = 0.0;     // seconds since stroke start
    };

    // Vector stroke: kept as samples so it can be re-fitted, re-brushed and
    // re-timed after drawing (overview section 3, vector stroke tools).
    struct Stroke {
        List<StrokePoint>   Points;
        Vec4                Color   = { 0.05f, 0.05f, 0.06f, 1.0f };
        f32                 Width   = 2.0f;
        ID                  Brush   = NullID;   // BrushPreset used to render it
    };
}
