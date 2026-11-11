module;
#include <__Vendor/ImGui/imgui.h>
#include <memory>
#include <string>
#include <vector>

module com.citronlabs.Halftone.Editor;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Drawing;

using namespace Halftone::Editor;
using namespace Halftone::Core;
namespace Draw = Halftone::Drawing;

int BrushPanel::WindowFlags() const { return 0; }

void BrushPanel::Draw(Context& ctx)
{
    auto& brush = ctx.Brush;

    ImGui::Text("Mode: %s", Draw::DrawingModeName(ctx.ActiveDrawingMode));
    ImGui::SeparatorText("Brush");
    InputString("Preset", brush.Name);

    const char* kinds[] = { "Vector", "Raster", "Eraser" };
    int k = int(brush.Kind);
    if (ImGui::Combo("Engine", &k, kinds, 3)) brush.Kind = Draw::BrushKind(k);

    ImGui::ColorEdit4("Color", &brush.Color.x, ImGuiColorEditFlags_AlphaBar);
    ImGui::SliderFloat("Size", &brush.Size, 0.5f, 200.0f, "%.1f px", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("Opacity", &brush.Opacity, 0.0f, 1.0f);
    ImGui::SliderFloat("Hardness", &brush.Hardness, 0.0f, 1.0f);
    ImGui::SliderFloat("Spacing", &brush.Spacing, 0.01f, 1.0f);
    ImGui::SliderFloat("Stabilizer", &brush.Stabilizer, 0.0f, 1.0f);
    ImGui::Checkbox("Pressure > Size", &brush.PressureSize);
    ImGui::SameLine();
    ImGui::Checkbox("Pressure > Opacity", &brush.PressureOpacity);

    // TODO: brush presets come from the scene's resolved asset packs.
}
