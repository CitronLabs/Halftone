module;
#include <__Vendor/ImGui/imgui.h>
#include <memory>
#include <vector>

module com.citronlabs.Halftone.Editor;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Drawing;

using namespace Halftone::Editor;
using namespace Halftone::Core;
namespace Draw = Halftone::Drawing;

int ToolsPanel::WindowFlags() const { return ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar; }

namespace
{
    bool ToolButton(const char* label, const char* tooltip, bool active, f32 w)
    {
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        bool pressed = ImGui::Button(label, { w, w });
        if (active) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
        return pressed;
    }
}

void ToolsPanel::Draw(EditorContext& ctx)
{
    const f32 w = ImGui::GetContentRegionAvail().x;

    switch (ctx.Mode)
    {
        case EditorModeKind::Scene:
        case EditorModeKind::Canvas3D: {
            struct Entry { SceneTool Tool; const char* Label; };
            static constexpr Entry Entries[] = {
                { SceneTool::Select, "Sel" }, { SceneTool::Move, "Mov" }, { SceneTool::Rotate, "Rot" }, { SceneTool::Scale, "Scl" },
            };
            for (const auto& e : Entries)
                if (ToolButton(e.Label, SceneToolName(e.Tool), ctx.ActiveSceneTool == e.Tool, w)) ctx.ActiveSceneTool = e.Tool;
            break;
        }
        case EditorModeKind::Canvas2D: {
            static constexpr const char* Labels[] = { "Brs", "Pcl", "Vec", "Ers", "Fil", "Las", "Xfm" };
            for (u32 i = 0; i < Draw::DrawingModeCount; i++) {
                auto mode = Draw::DrawingMode(i);
                if (ToolButton(Labels[i], Draw::DrawingModeName(mode), ctx.ActiveDrawingMode == mode, w)) ctx.ActiveDrawingMode = mode;
            }
            break;
        }
        case EditorModeKind::Model: {
            if (ToolButton("Sel", "Select", ctx.ActiveModelTool == ModelTool::Select, w))             ctx.ActiveModelTool = ModelTool::Select;
            if (ToolButton("Pnt", "Texture Paint", ctx.ActiveModelTool == ModelTool::TexturePaint, w)) ctx.ActiveModelTool = ModelTool::TexturePaint;
            break;
        }
    }
}
