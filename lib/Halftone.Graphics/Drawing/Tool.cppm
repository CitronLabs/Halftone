module;

export module com.citronlabs.Halftone.Graphics.Drawing:Tool;

import com.citronlabs.Halftone.Core;
import :Brush;
import :Drawing;
import :Layer;
import :Canvas;

export namespace Halftone::Drawing
{
    using namespace Halftone::Core;

    // Drawing modes offered in the Canvas 2D view's tool strip.
    enum class DrawingMode : u8 { Brush, Pencil, VectorPen, Eraser, Fill, Lasso, Transform };
    inline constexpr u32 DrawingModeCount = 7;
    const char* DrawingModeName(DrawingMode mode);

    // Pointer/pen input already mapped into canvas page space (pixels).
    struct PointerEvent {
        Vec2    Position;
        f32     Pressure = 1.0f;
        f32     TiltX    = 0.0f;
        f32     TiltY    = 0.0f;
        f64     Time     = 0.0;
        bool    Eraser   = false;   // pen flipped / eraser button
    };

    // What a tool is allowed to modify for one stroke.
    struct ToolTarget {
        CanvasDocument&     Canvas;
        Layer&              ActiveLayer;
        Drawing&            ActiveDrawing;
        const BrushPreset&  Brush;
    };

    // Interactive drawing tool. The Canvas 2D view forwards viewport input to
    // the tool registered for the active DrawingMode.
    class Tool {
    public:
        virtual ~Tool() = default;

        virtual const char* Name() const = 0;
        virtual DrawingMode Mode() const = 0;

        virtual void OnActivate()   {}
        virtual void OnDeactivate() {}

        virtual void OnPointerDown(ToolTarget& /*target*/, const PointerEvent& /*e*/) {}
        virtual void OnPointerMove(ToolTarget& /*target*/, const PointerEvent& /*e*/) {}
        virtual void OnPointerUp  (ToolTarget& /*target*/, const PointerEvent& /*e*/) {}
    };
}
