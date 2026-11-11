module;
#include <vector>

export module com.citronlabs.Halftone.Graphics.Animation:ExposureSheet;

import com.citronlabs.Halftone.Core;
import :Cadence;

export namespace Halftone::Animation
{
    using namespace Halftone::Core;

    // One X-sheet entry: from `Start` onward show `Drawing` until the next entry.
    struct Exposure {
        Frame   Start   = 0;
        ID      Drawing = NullID;   // Drawing::Drawing
    };

    // Which drawing a CharacterCard shows on which frame (traditional
    // exposure sheet / X-sheet).
    class ExposureSheet {
    private:
        ID              _Id;
        List<Exposure>  _Exposures;     // sorted by Start
        Cadence         _Cadence;

    public:
        explicit ExposureSheet(ID id = NullID) : _Id(id) {}

        void Expose(Frame start, ID drawing);       // insert or replace at `start`
        void Clear(Frame start);

        // Drawing visible at `frame` after applying the sheet's cadence.
        ID   DrawingAt(Frame frame) const;

        ID                      Id()        const { return _Id; }
        const List<Exposure>&   Exposures() const { return _Exposures; }
        Cadence&                GetCadence()      { return _Cadence; }
        const Cadence&          GetCadence() const { return _Cadence; }
    };
}
