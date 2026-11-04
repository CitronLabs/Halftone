module;
#include <memory>
#include <vector>

export module com.citronlabs.Halftone.Editor:Workspace;

import com.citronlabs.Halftone.Core;
import :Context;
import :Panel;

export namespace Halftone::Editor
{
    using namespace Halftone::Core;

    // Lays a set of panels out around the viewport using their PanelSlot.
    class Workspace {
    public:
        // `top` is the height already used above (menu bar + toolbars).
        static void Draw(EditorContext& ctx, List<Unique<Panel>>& panels, f32 top);
    };
}
