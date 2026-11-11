module;
#include <__Vendor/ImGui/imgui.h>
#include <memory>
#include <string>
#include <vector>

module com.citronlabs.Halftone.Editor;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Drawing;
import com.citronlabs.Halftone.Scene;

using namespace Halftone::Editor;
using namespace Halftone::Core;
namespace Draw = Halftone::Drawing;

Canvas2DMode::Canvas2DMode()
{
    AddPanel<ToolsPanel>();
    AddPanel<ViewportPanel>();
    AddPanel<LayersPanel>();
    AddPanel<BrushPanel>();
    AddPanel<TimelinePanel>();
}

bool Canvas2DMode::Accepts(Context& ctx, ID focus) const
{
    auto* doc = ctx.Document();
    return doc and doc->FindAs<Halftone::Scene::Canvas>(focus);
}

void Canvas2DMode::OnEnter(Context& ctx)
{
    if (auto* canvas = ctx.FocusedCanvas()) {
        ctx.Navigator().FrameCanvas(canvas->Document.Size);
        if (not canvas->Document.Active() and not canvas->Document.Layers().empty())
            canvas->Document.ActiveLayer = canvas->Document.Layers().back().Id;
    }
}

void Canvas2DMode::DrawMenus(Context& ctx)
{
    auto* canvas = ctx.FocusedCanvas();
    if (not canvas) return;
    if (ImGui::BeginMenu("Canvas")) {
        if (ImGui::MenuItem("New Raster Layer")) { ApplySceneDefaults(ctx, canvas->Document.AddLayer("Layer", Draw::LayerKind::Raster)); ctx.MarkDirty(); }
        if (ImGui::MenuItem("New Vector Layer")) { ApplySceneDefaults(ctx, canvas->Document.AddLayer("Ink", Draw::LayerKind::Vector));   ctx.MarkDirty(); }
        ImGui::Separator();
        if (ImGui::MenuItem("Frame Page", "F")) ctx.Navigator().FrameCanvas(canvas->Document.Size);
        if (ImGui::MenuItem("Switch to 3D View")) ctx.RequestMode(EditorModeKind::Canvas3D, ctx.Focus);
        ImGui::EndMenu();
    }
}
