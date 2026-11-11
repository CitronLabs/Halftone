module;
#include <__Vendor/ImGui/imgui.h>
#include <memory>
#include <string>
#include <vector>

module com.citronlabs.Halftone.Editor;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Scene;
import com.citronlabs.Halftone.Graphics;

using namespace Halftone::Editor;
using namespace Halftone::Core;
namespace SceneNS = Halftone::Scene;

int PropertiesPanel::WindowFlags() const { return 0; }

namespace
{
    bool DrawNodeSpecific(Context& ctx, SceneNS::Node& node)
    {
        bool c = false;
        switch (node.Kind())
        {
            case SceneNS::NodeKind::Camera: {
                auto& cam = static_cast<SceneNS::Camera&>(node);
                c |= ImGui::SliderAngle("Field of View", &cam.FieldOfView, 5.0f, 120.0f);
                c |= ImGui::DragFloatRange2("Clip", &cam.NearClip, &cam.FarClip, 0.1f, 0.01f, 10000.0f);
                break;
            }
            case SceneNS::NodeKind::Canvas: {
                auto& canvas = static_cast<SceneNS::Canvas&>(node);
                const char* roles[]   = { "Character", "Environment", "Prop", "Free" };
                const char* facing[]  = { "Fixed", "Face Camera", "Face Camera (Upright)" };
                const char* mapping[] = { "Card", "Camera Projection" };
                int r = int(canvas.Role), f = int(canvas.Facing), m = int(canvas.Mapping);
                if (ImGui::Combo("Role", &r, roles, 4))      { canvas.Role = SceneNS::CanvasRole(r); c = true; }
                if (ImGui::Combo("Facing", &f, facing, 3))   { canvas.Facing = SceneNS::CardFacing(f); c = true; }
                if (ImGui::Combo("Mapping", &m, mapping, 2)) { canvas.Mapping = SceneNS::CanvasMapping(m); c = true; }
                c |= ImGui::DragFloat2("World Size", &canvas.WorldSize.x, 0.01f, 0.01f, 1000.0f);
                c |= ImGui::SliderFloat2("Anchor", &canvas.Anchor.x, 0.0f, 1.0f);
                c |= ImGui::DragFloat2("Page (px)", &canvas.Document.Size.x, 1.0f, 16.0f, 16384.0f, "%.0f");
                c |= ImGui::Checkbox("Receive Lighting", &canvas.ReceiveLighting);
                c |= ImGui::Checkbox("Cast Shadows", &canvas.CastShadows);
                if (ImGui::Button("Edit in 2D")) ctx.RequestMode(EditorModeKind::Canvas2D, node.Id());
                ImGui::SameLine();
                if (ImGui::Button("Edit in 3D")) ctx.RequestMode(EditorModeKind::Canvas3D, node.Id());
                break;
            }
            case SceneNS::NodeKind::Model: {
                auto& model = static_cast<SceneNS::Model&>(node);
                c |= InputString("Mesh Asset", model.MeshAsset);
                c |= ImGui::Checkbox("Cast Shadows", &model.CastShadows);
                c |= ImGui::Checkbox("Receive Shadows", &model.ReceiveShadows);
                if (ctx.Mode != EditorModeKind::Model and ImGui::Button("Edit Model")) ctx.RequestMode(EditorModeKind::Model, node.Id());
                break;
            }
            case SceneNS::NodeKind::Light: {
                auto& light = static_cast<SceneNS::Light&>(node);
                const char* kinds[] = { "Directional", "Point", "Rim" };
                int k = int(light.Type);
                if (ImGui::Combo("Type", &k, kinds, 3)) { light.Type = SceneNS::LightKind(k); c = true; }
                c |= ImGui::ColorEdit3("Color", &light.Color.x);
                c |= ImGui::DragFloat("Intensity", &light.Intensity, 0.01f, 0.0f, 100.0f);
                int bands = int(light.ToonBands);
                if (ImGui::SliderInt("Toon Bands", &bands, 1, 8)) { light.ToonBands = u32(bands); c = true; }
                c |= ImGui::Checkbox("Affects Canvases", &light.AffectsCanvases);
                c |= ImGui::Checkbox("Casts Shadows", &light.CastsShadows);
                break;
            }
            case SceneNS::NodeKind::Group: break;
        }
        return c;
    }
}

void PropertiesPanel::Draw(Context& ctx)
{
    auto* doc  = ctx.Document();
    auto* node = doc ? doc->Find(ctx.Selection) : nullptr;

    if (node)
    {
        bool c = false;
        ImGui::SeparatorText(SceneNS::NodeKindName(node->Kind()));
        String name = node->Name();
        if (InputString("Name", name)) { node->SetName(name); c = true; }
        c |= ImGui::Checkbox("Visible", &node->Visible);
        c |= ImGui::DragFloat3("Position", &node->LocalTransform.Position.x, 0.01f);
        c |= ImGui::DragFloat3("Rotation", &node->LocalTransform.Rotation.x, 0.01f);
        c |= ImGui::DragFloat3("Scale",    &node->LocalTransform.Scale.x,    0.01f);
        c |= DrawNodeSpecific(ctx, *node);
        if (c) ctx.MarkDirty();
    }
    else
    {
        ImGui::TextDisabled("Nothing selected");
    }

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Render Engine")) {
        if (auto* engine = ctx.Renderer.Engine()) {
            ImGui::Text("%s", engine->Name());
            engine->DrawSettings();
        }
    }
}
