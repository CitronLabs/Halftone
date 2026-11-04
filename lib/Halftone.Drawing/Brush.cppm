module;
#include <string>

export module com.citronlabs.Halftone.Drawing:Brush;

import com.citronlabs.Halftone.Core;

export namespace Halftone::Drawing
{
    using namespace Halftone::Core;

    enum class BrushKind : u8 {
        Vector,     // produces Stroke data
        Raster,     // stamps into a pixel layer (raster brush engine)
        Eraser,
    };

    // Parameter block for both engines. The raster brush engine
    // (stamp/dab generation, GPU compositing) consumes this.
    struct BrushPreset {
        ID          Id              = NullID;
        String      Name            = "Pencil";
        BrushKind   Kind            = BrushKind::Raster;
        Vec4        Color           = { 0.08f, 0.08f, 0.10f, 1.0f };
        f32         Size            = 4.0f;     // px at pressure 1
        f32         Opacity         = 1.0f;
        f32         Hardness        = 0.8f;
        f32         Spacing         = 0.1f;     // fraction of size between dabs
        bool        PressureSize    = true;
        bool        PressureOpacity = false;
        f32         Stabilizer      = 0.0f;     // 0..1 lazy-mouse smoothing
    };
}
