module;
#include <algorithm>

module com.citronlabs.Halftone.Animation;

import com.citronlabs.Halftone.Core;

using namespace Halftone::Animation;
using namespace Halftone::Core;

void Timeline::Update(f64 deltaSeconds)
{
    if (not _Playing) return;

    _Accumulator += deltaSeconds;
    const f64 spf = _Rate.SecondsPerFrame();

    while (_Accumulator >= spf) {
        _Accumulator -= spf;

        if (_Current >= _Range.End) {
            if (_Loop) _Current = _Range.Start;
            else       { Pause(); return; }
        } else {
            _Current++;
        }
    }
}

void Timeline::SetFrame(Frame frame)
{
    _Current = std::clamp(frame, _Range.Start, _Range.End);
}
