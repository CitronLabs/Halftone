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
namespace Draw    = Halftone::Drawing;
namespace SceneNS = Halftone::Scene;

int MaterialPanel::WindowFlags() const { return 0; }

void MaterialPanel::Draw(EditorContext& ctx)
{
    auto* model = ctx.FocusedModel();
    if (not model) { ImGui::TextDisabled("No model focused"); return; }
    auto& mat = model->Surface;
    bool c = false;

    ImGui::TextDisabled("%s", model->Name().c_str());
    ImGui::SeparatorText("Material");
    c |= InputString("Name", mat.Name);
    c |= ImGui::ColorEdit3("Base Color", &mat.BaseColor.x);
    c |= InputString("Base Texture", mat.BaseColorTexture);
    c |= ImGui::ColorEdit3("Shadow Tint", &mat.ShadowTint.x);
    int bands = int(mat.ToonBands);
    if (ImGui::SliderInt("Toon Bands", &bands, 1, 8)) { mat.ToonBands = u32(bands); c = true; }
    c |= ImGui::Checkbox("Outline", &mat.Outline);
    ImGui::BeginDisabled(not mat.Outline);
    c |= ImGui::SliderFloat("Outline Width", &mat.OutlineWidth, 0.1f, 8.0f, "%.1f px");
    ImGui::EndDisabled();

    ImGui::SeparatorText("Texture Paint Layers");
    if (ImGui::Button("+ Paint Layer")) {
        SceneNS::TexturePaintLayer layer;
        layer.Id   = mat.PaintLayers.size() + 1;
        layer.Name = "Paint " + std::to_string(mat.PaintLayers.size() + 1);
        mat.PaintLayers.push_back(std::move(layer));
        c = true;
    }
    for (usize i = mat.PaintLayers.size(); i-- > 0;) {
        auto& layer = mat.PaintLayers[i];
        ImGui::PushID(int(i));
        c |= ImGui::Checkbox("##vis", &layer.Visible);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(110);
        c |= InputString("##name", layer.Name);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(90);
        if (ImGui::BeginCombo("##blend", Draw::BlendModeName(layer.Blend))) {
            for (u32 b = 0; b < Draw::BlendModeCount; b++)
                if (ImGui::Selectable(Draw::BlendModeName(Draw::BlendMode(b)))) { layer.Blend = Draw::BlendMode(b); c = true; }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("x")) { mat.PaintLayers.erase(mat.PaintLayers.begin() + i); c = true; ImGui::PopID(); break; }
        ImGui::PopID();
    }
    if (mat.PaintLayers.empty()) ImGui::TextDisabled("Add a paint layer, then use the Texture Paint tool.");

    if (c) ctx.MarkDirty();
}
