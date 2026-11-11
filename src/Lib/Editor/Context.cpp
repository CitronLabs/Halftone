module;
#include <__Vendor/ImGui/imgui.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

module com.citronlabs.Halftone.Editor;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Scene;
import com.citronlabs.Halftone.Drawing;
import com.citronlabs.Halftone.Graphics;

using namespace Halftone::Editor;
using namespace Halftone::Core;
namespace SceneNS = Halftone::Scene;
namespace Gfx     = Halftone::Graphics;

const char* Halftone::Editor::EditorModeName(EditorModeKind mode)
{
    switch (mode) {
        case EditorModeKind::Scene:    return "Scene";
        case EditorModeKind::Canvas2D: return "Canvas 2D";
        case EditorModeKind::Canvas3D: return "Canvas 3D";
        case EditorModeKind::Model:    return "Model";
    }
    return "?";
}

Gfx::ViewKind Halftone::Editor::ViewKindFor(EditorModeKind mode)
{
    switch (mode) {
        case EditorModeKind::Scene:    return Gfx::ViewKind::Scene;
        case EditorModeKind::Canvas2D: return Gfx::ViewKind::CanvasFlat;
        case EditorModeKind::Canvas3D: return Gfx::ViewKind::CanvasDepth;
        case EditorModeKind::Model:    return Gfx::ViewKind::ModelFocus;
    }
    return Gfx::ViewKind::Scene;
}

const char* Halftone::Editor::SceneToolName(SceneTool tool)
{
    switch (tool) {
        case SceneTool::Select: return "Select";
        case SceneTool::Move:   return "Move";
        case SceneTool::Rotate: return "Rotate";
        case SceneTool::Scale:  return "Scale";
    }
    return "?";
}

const char* Halftone::Editor::ModelToolName(ModelTool tool)
{
    return tool == ModelTool::Select ? "Select" : "Texture Paint";
}

// --- ViewNavigator --------------------------------------------------------------

Gfx::ViewCamera ViewNavigator::Camera(Gfx::ViewKind kind) const
{
    Gfx::ViewCamera cam;
    if (kind == Gfx::ViewKind::CanvasFlat) {
        cam.Orthographic = true;
        cam.Target       = { PageCenter.x, PageCenter.y, 0.0f };
        cam.Position     = { PageCenter.x, PageCenter.y, 10.0f };
        cam.OrthoHeight  = PageHeight;
        return cam;
    }
    const f32 cp = std::cos(Pitch);
    cam.Target   = Target;
    cam.Position = Target + Vec3 { cp * std::sin(Yaw), std::sin(Pitch), cp * std::cos(Yaw) } * Distance;
    return cam;
}

void ViewNavigator::Orbit(f32 dx, f32 dy)
{
    Yaw  -= dx * 0.008f;
    Pitch = std::clamp(Pitch + dy * 0.008f, -1.45f, 1.45f);
}

void ViewNavigator::Pan(f32 dx, f32 dy, f32 viewportHeight, bool flat)
{
    if (viewportHeight < 1.0f) return;
    if (flat) {
        const f32 k = PageHeight / viewportHeight;
        PageCenter = PageCenter - Vec2 { dx * k, dy * k };
        return;
    }
    Gfx::ViewCamera cam = Camera(Gfx::ViewKind::Scene);
    const Vec3 forward = Normalize(cam.Target - cam.Position);
    const Vec3 right   = Normalize(Cross(forward, cam.Up));
    const Vec3 up      = Cross(right, forward);
    const f32  k       = 2.0f * Distance * std::tan(cam.FieldOfView * 0.5f) / viewportHeight;
    Target = Target - right * (dx * k) + up * (dy * k);
}

void ViewNavigator::Zoom(f32 steps, bool flat)
{
    const f32 factor = std::pow(0.88f, steps);
    if (flat) PageHeight = std::clamp(PageHeight * factor, 32.0f, 40000.0f);
    else      Distance   = std::clamp(Distance * factor, 0.3f, 400.0f);
}

void ViewNavigator::FrameCanvas(Vec2 pageSize)
{
    PageCenter = pageSize * 0.5f;
    PageHeight = std::max(pageSize.y, pageSize.x * 0.6f) * 1.15f;
}

// --- Context ----------------------------------------------------------------

SceneNS::Canvas* Context::FocusedCanvas()
{
    auto* doc = Document();
    return doc ? doc->FindAs<SceneNS::Canvas>(Focus) : nullptr;
}

SceneNS::Model* Context::FocusedModel()
{
    auto* doc = Document();
    return doc ? doc->FindAs<SceneNS::Model>(Focus) : nullptr;
}

Halftone::Drawing::Tool* Context::ToolFor(Halftone::Drawing::DrawingMode mode)
{
    for (auto& tool : DrawingTools) if (tool->Mode() == mode) return tool.get();
    return nullptr;
}

// --- Shared UI helpers ----------------------------------------------------------

void Halftone::Editor::ApplySceneDefaults(Context& ctx, Halftone::Drawing::CanvasDocument& doc)
{
    if (not ctx.Open) return;
    for (auto& layer : doc.Layers()) layer.Exposures.GetCadence().Step = ctx.Open->DefaultCadence;
}

void Halftone::Editor::ApplySceneDefaults(Context& ctx, Halftone::Drawing::Layer& layer)
{
    if (ctx.Open) layer.Exposures.GetCadence().Step = ctx.Open->DefaultCadence;
}

void Halftone::Editor::AddNodeMenuItems(Context& ctx)
{
    auto* doc = ctx.Document();
    if (not doc) return;

    auto added = [&](SceneNS::Node& node) {
        ctx.Selection = node.Id();
        ctx.MarkDirty();
    };

    if (ImGui::BeginMenu("Canvas")) {
        const SceneNS::CanvasRole roles[] = { SceneNS::CanvasRole::Character, SceneNS::CanvasRole::Environment,
                                              SceneNS::CanvasRole::Prop, SceneNS::CanvasRole::Free };
        for (auto role : roles) {
            if (ImGui::MenuItem(SceneNS::CanvasRoleName(role))) {
                auto& canvas = doc->Create<SceneNS::Canvas>(String(SceneNS::CanvasRoleName(role)) + " Canvas");
                canvas.ApplyRoleDefaults(role);
                if (role == SceneNS::CanvasRole::Environment) canvas.LocalTransform.Position = { 0.0f, 0.0f, -9.0f };
                else if (role == SceneNS::CanvasRole::Character) canvas.LocalTransform.Position = { 0.0f, 0.0f, 0.0f };
                else canvas.LocalTransform.Position = { -2.0f, 0.0f, 1.0f };
                ApplySceneDefaults(ctx, canvas.Document);
                added(canvas);
            }
        }
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Model")) {
        auto& model = doc->Create<SceneNS::Model>("Model");
        model.LocalTransform.Position = { 2.0f, 0.5f, 0.0f };
        added(model);
    }
    if (ImGui::BeginMenu("Light")) {
        const char* names[] = { "Directional", "Point", "Rim" };
        for (int i = 0; i < 3; i++)
            if (ImGui::MenuItem(names[i])) {
                auto& light = doc->Create<SceneNS::Light>(String(names[i]) + " Light");
                light.Type = SceneNS::LightKind(i);
                light.LocalTransform.Position = { -3.0f, 5.0f, 3.0f };
                light.LocalTransform.Rotation = { -0.8f, 0.6f, 0.0f };
                added(light);
            }
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Camera")) {
        auto& cam = doc->Create<SceneNS::Camera>("Camera");
        cam.LocalTransform.Position = { 4.0f, 1.6f, 6.0f };
        added(cam);
    }
    if (ImGui::MenuItem("Group")) added(doc->Create<SceneNS::Group>("Group"));
}

bool Halftone::Editor::InputString(const char* label, String& value, bool multiline)
{
    char buffer[2048];
    const usize n = std::min(value.size(), sizeof(buffer) - 1);
    std::memcpy(buffer, value.data(), n);
    buffer[n] = 0;

    if (multiline and label[0] != '#') ImGui::TextDisabled("%s", label);
    bool changed = multiline
        ? ImGui::InputTextMultiline((String("##") + label).c_str(), buffer, sizeof(buffer), { -1.0f, ImGui::GetTextLineHeight() * 4.0f })
        : ImGui::InputText(label, buffer, sizeof(buffer));
    if (changed) value = buffer;
    return changed;
}
