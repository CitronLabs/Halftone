module;
#include <__Vendor/ImGui/imgui.h>
#include <algorithm>
#include <memory>
#include <vector>

module com.citronlabs.Halftone.Editor;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Scene;

using namespace Halftone::Editor;
using namespace Halftone::Core;

ModelEditMode::ModelEditMode()
{
    AddPanel<ToolsPanel>();
    AddPanel<ViewportPanel>();
    AddPanel<MaterialPanel>();
    AddPanel<PropertiesPanel>();
}

bool ModelEditMode::Accepts(Context& ctx, ID focus) const
{
    auto* doc = ctx.Document();
    return doc and doc->FindAs<Halftone::Scene::Model>(focus);
}

void ModelEditMode::OnEnter(Context& ctx)
{
    if (auto* model = ctx.FocusedModel()) {
        auto& nav = ctx.Navigator();
        const auto& s = model->LocalTransform.Scale;
        nav.Target   = { 0.0f, 0.5f * s.y, 0.0f };
        nav.Distance = std::max({ s.x, s.y, s.z }) * 3.0f + 1.0f;
    }
}

void ModelEditMode::DrawMenus(Context& ctx)
{
    if (ImGui::BeginMenu("Model")) {
        if (ImGui::MenuItem("Texture Paint Tool")) ctx.ActiveModelTool = ModelTool::TexturePaint;
        ImGui::MenuItem("Import Mesh...", nullptr, false, false);   // TODO: mesh import add-on
        ImGui::EndMenu();
    }
}
