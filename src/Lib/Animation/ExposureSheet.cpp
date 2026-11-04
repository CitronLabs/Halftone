module;
#include <algorithm>
#include <vector>

module com.citronlabs.Halftone.Animation;

import com.citronlabs.Halftone.Core;

using namespace Halftone::Animation;
using namespace Halftone::Core;

void ExposureSheet::Expose(Frame start, ID drawing)
{
    auto it = std::lower_bound(_Exposures.begin(), _Exposures.end(), start,
        [](const Exposure& e, Frame f) { return e.Start < f; });

    if (it != _Exposures.end() and it->Start == start) it->Drawing = drawing;
    else _Exposures.insert(it, Exposure { .Start = start, .Drawing = drawing });
}

void ExposureSheet::Clear(Frame start)
{
    std::erase_if(_Exposures, [&](const Exposure& e) { return e.Start == start; });
}

ID ExposureSheet::DrawingAt(Frame frame) const
{
    const Frame held = _Cadence.HeldFrame(frame);

    auto it = std::upper_bound(_Exposures.begin(), _Exposures.end(), held,
        [](Frame f, const Exposure& e) { return f < e.Start; });

    if (it == _Exposures.begin()) return NullID;
    return std::prev(it)->Drawing;
}
