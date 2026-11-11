module;
#include <string>
#include <vector>

export module com.citronlabs.Halftone.Graphics.Drawing:Drawing;

import com.citronlabs.Halftone.Core;
import :Stroke;

export namespace Halftone::Drawing
{
    using namespace Halftone::Core;

    // One frame of hand-drawn art on one layer (a pose, an in-between, a
    // background painting). Which drawing shows on which frame is decided by
    // the owning Layer's exposure sheet.
    struct Drawing {
        ID              Id          = NullID;
        String          Name;
        List<Stroke>    Strokes;                // vector content
        ID              RasterTiles = NullID;   // TODO(raster engine): handle into the GPU tile store
    };
}
