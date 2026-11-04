module;

export module com.citronlabs.Halftone.Animation:Timeline;

import com.citronlabs.Halftone.Core;

export namespace Halftone::Animation
{
    using namespace Halftone::Core;

    // The project clock. Everything that animates (camera sweeps, stepped
    // drawings, rigs) samples the same integer Frame from here.
    class Timeline {
    private:
        FrameRate   _Rate;
        FrameRange  _Range       = { 1, 120 };
        Frame       _Current     = 1;
        f64         _Accumulator = 0.0;
        bool        _Playing     = false;
        bool        _Loop        = true;

    public:
        Timeline() = default;
        Timeline(FrameRate rate, FrameRange range) : _Rate(rate), _Range(range), _Current(range.Start) {}

        // Advance by wall-clock seconds while playing (whole frames only).
        void Update(f64 deltaSeconds);

        void Play()                 { _Playing = true; }
        void Pause()                { _Playing = false; _Accumulator = 0.0; }
        void TogglePlay()           { _Playing ? Pause() : Play(); }
        void SetFrame(Frame frame);
        void Step(Frame delta)      { SetFrame(_Current + delta); }

        void SetRate(FrameRate rate)    { _Rate = rate; }
        void SetRange(FrameRange range) { _Range = range; SetFrame(_Current); }
        void SetLoop(bool loop)         { _Loop = loop; }

        Frame       Current()  const { return _Current; }
        FrameRate   Rate()     const { return _Rate; }
        FrameRange  Range()    const { return _Range; }
        bool        Playing()  const { return _Playing; }
        bool        Looping()  const { return _Loop; }
        f64         Seconds()  const { return _Rate.ToSeconds(_Current - _Range.Start); }
    };
}
