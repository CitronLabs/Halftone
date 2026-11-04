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

int LayersPanel::WindowFlags() const { return 0; }

void LayersPanel::Draw(EditorContext& ctx)
{
    auto* canvas = ctx.FocusedCanvas();
    if (not canvas) { ImGui::TextDisabled("No canvas focused"); return; }
    auto& doc = canvas->Document;
    bool changed = false;

    ImGui::TextDisabled("%s", canvas->Name().c_str());
    if (ImGui::Button("+ Raster")) { ApplySceneDefaults(ctx, doc.AddLayer("Layer " + std::to_string(doc.Layers().size() + 1), Draw::LayerKind::Raster)); changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("+ Vector")) { ApplySceneDefaults(ctx, doc.AddLayer("Ink " + std::to_string(doc.Layers().size() + 1), Draw::LayerKind::Vector)); changed = true; }
    ImGui::SameLine();
    if (ImGui::Button("Up"))    changed |= doc.MoveLayer(doc.ActiveLayer, +1);
    ImGui::SameLine();
    if (ImGui::Button("Down"))  changed |= doc.MoveLayer(doc.ActiveLayer, -1);
    ImGui::SameLine();
    if (ImGui::Button("Delete")) changed |= doc.RemoveLayer(doc.ActiveLayer);

    ImGui::Separator();

    // Top of the stack first, like every paint program.
    auto& layers = doc.Layers();
    if (ImGui::BeginTable("layers", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
    {
        ImGui::TableSetupColumn("vis");
        ImGui::TableSetupColumn("lock");
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("info");
        for (usize i = layers.size(); i-- > 0;) {
            auto& layer = layers[i];
            ImGui::PushID(int(layer.Id));
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); changed |= ImGui::Checkbox("##vis", &layer.Visible);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Visible");
            ImGui::TableNextColumn(); changed |= ImGui::Checkbox("##lock", &layer.Locked);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Locked");
            ImGui::TableNextColumn();
            if (ImGui::Selectable(layer.Name.c_str(), layer.Id == doc.ActiveLayer)) doc.ActiveLayer = layer.Id;
            ImGui::TableNextColumn();
            ImGui::TextDisabled("%s  %s  %d%%", Draw::LayerKindName(layer.Kind), Draw::BlendModeName(layer.Blend), int(layer.Opacity * 100 + 0.5f));
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    if (auto* layer = doc.Active())
    {
        ImGui::SeparatorText("Layer");
        changed |= InputString("Name", layer->Name);

        int blend = int(layer->Blend);
        if (ImGui::BeginCombo("Blend", Draw::BlendModeName(layer->Blend))) {
            for (u32 b = 0; b < Draw::BlendModeCount; b++)
                if (ImGui::Selectable(Draw::BlendModeName(Draw::BlendMode(b)), blend == int(b))) { layer->Blend = Draw::BlendMode(b); changed = true; }
            ImGui::EndCombo();
        }
        changed |= ImGui::SliderFloat("Opacity", &layer->Opacity, 0.0f, 1.0f, "%.2f");
        ImGui::TextDisabled("%s layer, %zu drawing(s)", Draw::LayerKindName(layer->Kind), layer->Drawings.size());

        if (ctx.Mode == EditorModeKind::Canvas3D)
        {
            ImGui::SeparatorText("Placement in 3D");
            changed |= ImGui::DragFloat("Depth (z)", &layer->DepthTransform.Position.z, 0.02f, -100.0f, 100.0f, "%.2f");
            changed |= ImGui::DragFloat2("Offset", &layer->DepthTransform.Position.x, 0.01f);
            changed |= ImGui::DragFloat3("Rotation", &layer->DepthTransform.Rotation.x, 0.01f);
            changed |= ImGui::DragFloat2("Scale", &layer->DepthTransform.Scale.x, 0.01f, 0.01f, 100.0f);
            if (ImGui::Button("Reset Placement")) { layer->DepthTransform = {}; changed = true; }
            ImGui::SameLine();
            if (ImGui::Button("Spread All Layers")) {
                // Bottom layer furthest away, 1 unit apart.
                for (usize i = 0; i < layers.size(); i++) layers[i].DepthTransform.Position.z = f32(i) - f32(layers.size() - 1);
                changed = true;
            }
        }
    }

    if (changed) ctx.MarkDirty();
}
