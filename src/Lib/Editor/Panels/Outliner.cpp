module;
#include <__Vendor/ImGui/imgui.h>
#include <memory>
#include <string>
#include <vector>

module com.citronlabs.Halftone.Editor;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Scene;

using namespace Halftone::Editor;
using namespace Halftone::Core;
namespace SceneNS = Halftone::Scene;

int OutlinerPanel::WindowFlags() const { return 0; }

void OutlinerPanel::Draw(Context& ctx)
{
    auto* doc = ctx.Document();
    if (not doc) return;

    if (ImGui::Button("+ Add")) ImGui::OpenPopup("add-node");
    if (ImGui::BeginPopup("add-node")) { AddNodeMenuItems(ctx); ImGui::EndPopup(); }
    ImGui::SameLine();
    ImGui::TextDisabled("double-click a canvas or model to edit it");
    ImGui::Separator();

    auto* active = doc->ActiveCamera();
    ID pendingDelete = NullID;

    if (ImGui::BeginTable("outliner", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch, 0.55f);
        ImGui::TableSetupColumn("kind", ImGuiTableColumnFlags_WidthStretch, 0.45f);
        for (const auto& node : doc->Nodes())
        {
            ImGui::PushID(int(node->Id()));
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            const bool selected = ctx.Selection == node->Id();
            if (ImGui::Selectable(node->Name().c_str(), selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick))
            {
                ctx.Selection = node->Id();
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    if (node->Kind() == SceneNS::NodeKind::Canvas) ctx.RequestMode(EditorModeKind::Canvas2D, node->Id());
                    if (node->Kind() == SceneNS::NodeKind::Model)  ctx.RequestMode(EditorModeKind::Model, node->Id());
                }
            }

            if (ImGui::BeginPopupContextItem("node-menu")) {
                ctx.Selection = node->Id();
                if (node->Kind() == SceneNS::NodeKind::Canvas) {
                    if (ImGui::MenuItem("Edit Canvas (2D)")) ctx.RequestMode(EditorModeKind::Canvas2D, node->Id());
                    if (ImGui::MenuItem("Edit Canvas (3D)")) ctx.RequestMode(EditorModeKind::Canvas3D, node->Id());
                }
                if (node->Kind() == SceneNS::NodeKind::Model and ImGui::MenuItem("Edit Model")) ctx.RequestMode(EditorModeKind::Model, node->Id());
                if (node->Kind() == SceneNS::NodeKind::Camera and ImGui::MenuItem("Set as Shot Camera")) { doc->SetActiveCamera(node->Id()); ctx.MarkDirty(); }
                ImGui::Separator();
                if (ImGui::MenuItem("Delete")) pendingDelete = node->Id();
                ImGui::EndPopup();
            }

            String detail = SceneNS::NodeKindName(node->Kind());
            if (node->Kind() == SceneNS::NodeKind::Canvas) {
                auto& c = static_cast<const SceneNS::Canvas&>(*node);
                detail = String(SceneNS::CanvasRoleName(c.Role)) + " - " + std::to_string(c.Document.Layers().size()) + " layers";
            }
            if (active and active->Id() == node->Id()) detail += " (shot)";
            ImGui::TableNextColumn();
            ImGui::TextDisabled("%s", detail.c_str());
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    if (pendingDelete != NullID) {
        doc->Remove(pendingDelete);
        if (ctx.Selection == pendingDelete) ctx.Selection = NullID;
        ctx.MarkDirty();
    }
}
