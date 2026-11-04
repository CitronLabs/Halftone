module;

export module com.citronlabs.Halftone.Core:Time;

import :Types;

// Time is kept as an exact rational frame rate + integer frames wherever
// possible. Stepped 2D animation (twos/threes) and continuous camera motion
// both resolve against the same FrameRate, which is what keeps them in sync.
export namespace Halftone::Core
{
    using Frame = i64;

    struct FrameRate
    {
        u32 Numerator   = 24;
        u32 Denominator = 1;

        constexpr f64 FramesPerSecond() const { return f64(Numerator) / f64(Denominator); }
        constexpr f64 SecondsPerFrame() const { return f64(Denominator) / f64(Numerator); }

        constexpr f64   ToSeconds(Frame frame)   const { return f64(frame) * SecondsPerFrame(); }
        constexpr Frame FloorFrame(f64 seconds)  const { return Frame(seconds * FramesPerSecond()); }

        static constexpr FrameRate Film()     { return { 24, 1 }; }
        static constexpr FrameRate NTSCFilm() { return { 24000, 1001 }; }
        static constexpr FrameRate PAL()      { return { 25, 1 }; }
    };

    struct FrameRange
    {
        Frame Start = 0;
        Frame End   = 0;   // inclusive

        constexpr Frame Length()                const { return End - Start + 1; }
        constexpr bool  Contains(Frame f)       const { return f >= Start and f <= End; }
    };
}
