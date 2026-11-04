module;

export module com.citronlabs.Halftone.Animation:Rig;

import com.citronlabs.Halftone.Core;

export namespace Halftone::Animation
{
    using namespace Halftone::Core;

    // View-dependent pose request: how far the character is turned relative
    // to the camera, plus the timeline frame.
    struct RigPose {
        f32     Yaw     = 0.0f;     // radians, 0 = facing camera
        f32     Pitch   = 0.0f;
        Frame   AtFrame = 0;
    };

    // Smart 2D rig (overview section 3): lets a flat drawing turn/flex across
    // angles without a 3D model. Implementations produce a deformed mesh or a
    // blend of key drawings for the card pass to consume.
    class Rig {
    public:
        virtual ~Rig() = default;

        virtual const char* Name() const = 0;
        virtual void        Evaluate(const RigPose& pose) = 0;
    };
}
