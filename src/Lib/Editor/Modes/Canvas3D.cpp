module;
#include <__Vendor/ImGui/imgui.h>
#include <memory>
#include <vector>

module com.citronlabs.Halftone.Editor;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Scene;

using namespace Halftone::Editor;
using namespace Halftone::Core;

Canvas3DMode::Canvas3DMode()
{
    AddPanel<ToolsPanel>();
    AddPanel<ViewportPanel>();
    AddPanel<LayersPanel>();
    AddPanel<PropertiesPanel>();
    AddPanel<TimelinePanel>();
}

bool Canvas3DMode::Accepts(EditorContext& ctx, ID focus) const
{
    auto* doc = ctx.Document();
    return doc and doc->FindAs<Halftone::Scene::Canvas>(focus);
}

void Canvas3DMode::OnEnter(EditorContext& ctx)
{
    // Orbit around the middle of the page, far enough to see the whole stack.
    if (auto* canvas = ctx.FocusedCanvas()) {
        auto& nav = ctx.Navigator();
        nav.Target   = { canvas->WorldSize.x * (0.5f - canvas->Anchor.x), canvas->WorldSize.y * (0.5f - canvas->Anchor.y), -1.0f };
        nav.Distance = std::max(canvas->WorldSize.x, canvas->WorldSize.y) * 1.6f + 2.0f;
        nav.Yaw      = 0.75f;
        nav.Pitch    = 0.25f;
    }
}

void Canvas3DMode::DrawMenus(EditorContext& ctx)
{
    if (ImGui::BeginMenu("Canvas")) {
        if (ImGui::MenuItem("Switch to 2D View")) ctx.RequestMode(EditorModeKind::Canvas2D, ctx.Focus);
        ImGui::EndMenu();
    }
}
