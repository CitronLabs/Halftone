module;

export module com.citronlabs.Halftone.Graphics.Animation:Cadence;

import com.citronlabs.Halftone.Core;

export namespace Halftone::Animation
{
    using namespace Halftone::Core;

    // Stepped timing for hand-drawn animation: a drawing is held for `Step`
    // frames ("on twos" = 2, "on threes" = 3) while cameras stay continuous.
    //
    // HeldFrame() is the primitive. The sync work from overview section 4
    // (keeping a stepped card planted while the camera sweeps every frame)
    // builds on it in the render engine / card pass - see CharacterCard::Anchor.
    struct Cadence {
        u32     Step    = 2;
        Frame   Offset  = 1;    // first frame of a hold (timelines start at 1); shift to stagger characters

        constexpr Frame HeldFrame(Frame frame) const {
            if (Step <= 1) return frame;
            Frame local = frame - Offset;
            Frame step  = Frame(Step);
            Frame held  = (local >= 0 ? local / step : (local - step + 1) / step) * step;
            return held + Offset;
        }

        static constexpr Cadence Ones()   { return { 1, 1 }; }
        static constexpr Cadence Twos()   { return { 2, 1 }; }
        static constexpr Cadence Threes() { return { 3, 1 }; }
    };
}
