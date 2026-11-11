module;
#include <memory>
#include <vector>

export module com.citronlabs.Halftone.App.Editor:Workspace;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.App.UI;

export namespace Halftone::App::Editor
{
    using namespace Halftone::Core;

    // Lays a set of panels out around the viewport using their PanelSlot.
    class Workspace {
    public:
        // `top` is the height already used above (menu bar + toolbars).
        static void Draw(Context& ctx, List<Unique<Panel>>& panels, f32 top);
    };
}
