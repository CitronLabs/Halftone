module;
#include <__Vendor/ImGui/imgui.h>
#include <memory>
#include <vector>

module com.citronlabs.Halftone.Editor;

import com.citronlabs.Halftone.Core;

using namespace Halftone::Editor;
using namespace Halftone::Core;

SceneLayoutMode::SceneLayoutMode()
{
    AddPanel<ToolsPanel>();
    AddPanel<ViewportPanel>();
    AddPanel<OutlinerPanel>();
    AddPanel<PropertiesPanel>();
    AddPanel<TimelinePanel>();
}

bool SceneLayoutMode::Accepts(Context& ctx, ID /*focus*/) const { return ctx.Document() != nullptr; }

void SceneLayoutMode::OnEnter(Context& /*ctx*/) {}

void SceneLayoutMode::DrawMenus(Context& ctx)
{
    if (ImGui::BeginMenu("Add")) {
        AddNodeMenuItems(ctx);
        ImGui::EndMenu();
    }
}
