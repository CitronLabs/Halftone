module;
#include <__Vendor/ImGui/imgui.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
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
namespace Draw    = Halftone::Drawing;
namespace Gfx     = Halftone::Graphics;

// Editor overlays are drawn with ImGui on top of the engine's image. They are
// editor gizmos (grid, node markers, layer planes, page frame), not part of
// the final render - render engines never see them.
namespace
{
    constexpr ImU32 Grid        = IM_COL32(255, 255, 255, 28);
    constexpr ImU32 GridAxisX   = IM_COL32(230, 90, 90, 140);
    constexpr ImU32 GridAxisZ   = IM_COL32(90, 140, 240, 140);
    constexpr ImU32 Selected    = IM_COL32(255, 170, 40, 255);
    constexpr ImU32 Label       = IM_COL32(235, 235, 240, 230);

    ImU32 RoleColor(SceneNS::CanvasRole role, int alpha)
    {
        switch (role) {
            case SceneNS::CanvasRole::Character:   return IM_COL32(240, 150, 80, alpha);
            case SceneNS::CanvasRole::Environment: return IM_COL32(90, 170, 220, alpha);
            case SceneNS::CanvasRole::Prop:        return IM_COL32(180, 140, 230, alpha);
            case SceneNS::CanvasRole::Free:        return IM_COL32(200, 200, 200, alpha);
        }
        return IM_COL32(200, 200, 200, alpha);
    }

    ImU32 LayerColor(usize index, int alpha)
    {
        static const ImU32 hues[] = { IM_COL32(90,170,220,255), IM_COL32(240,150,80,255), IM_COL32(150,210,120,255),
                                      IM_COL32(200,130,220,255), IM_COL32(240,210,90,255) };
        ImU32 c = hues[index % 5];
        return (c & 0x00FFFFFF) | (ImU32(alpha) << 24);
    }

    struct Projector {
        Mat4    ViewProj;
        ImVec2  Origin;
        ImVec2  Size;

        bool operator()(Vec3 p, ImVec2& out) const {
            Vec4 c = ViewProj * Vec4 { p.x, p.y, p.z, 1.0f };
            if (c.w <= 0.02f) return false;
            out = { Origin.x + (c.x / c.w * 0.5f + 0.5f) * Size.x, Origin.y + (c.y / c.w * 0.5f + 0.5f) * Size.y };
            return true;
        }
    };

    void Line3(ImDrawList* dl, const Projector& P, Vec3 a, Vec3 b, ImU32 col, f32 thickness = 1.0f)
    {
        ImVec2 sa, sb;
        if (P(a, sa) and P(b, sb)) dl->AddLine(sa, sb, col, thickness);
    }

    void Quad3(ImDrawList* dl, const Projector& P, const Vec3 (&q)[4], ImU32 fill, ImU32 edge, f32 thickness)
    {
        ImVec2 s[4];
        for (int i = 0; i < 4; i++) if (not P(q[i], s[i])) return;
        dl->AddQuadFilled(s[0], s[1], s[2], s[3], fill);
        dl->AddQuad(s[0], s[1], s[2], s[3], edge, thickness);
    }

    void LabelAt(ImDrawList* dl, const Projector& P, Vec3 p, const char* text, ImU32 col = Label)
    {
        ImVec2 s;
        if (P(p, s)) dl->AddText({ s.x + 6, s.y - 18 }, col, text);
    }

    void DrawGrid(ImDrawList* dl, const Projector& P, int half, f32 alphaScale = 1.0f)
    {
        const ImU32 g = (Grid & 0x00FFFFFF) | (ImU32(28 * alphaScale) << 24);
        for (int i = -half; i <= half; i++) {
            Line3(dl, P, { f32(i), 0, f32(-half) }, { f32(i), 0, f32(half) }, i == 0 ? GridAxisZ : g);
            Line3(dl, P, { f32(-half), 0, f32(i) }, { f32(half), 0, f32(i) }, i == 0 ? GridAxisX : g);
        }
    }

    // Corners of a canvas page in canvas-local space (anchor at the origin).
    void CanvasCorners(const SceneNS::Canvas& c, const Mat4& m, Vec3 (&out)[4])
    {
        const f32 x0 = -c.WorldSize.x * c.Anchor.x, x1 = c.WorldSize.x * (1.0f - c.Anchor.x);
        const f32 y0 = -c.WorldSize.y * c.Anchor.y, y1 = c.WorldSize.y * (1.0f - c.Anchor.y);
        out[0] = TransformPoint(m, { x0, y0, 0 }); out[1] = TransformPoint(m, { x1, y0, 0 });
        out[2] = TransformPoint(m, { x1, y1, 0 }); out[3] = TransformPoint(m, { x0, y1, 0 });
    }

    // World matrix of a canvas including its billboarding toward the view camera.
    Mat4 CanvasWorld(const SceneNS::Canvas& c, const Gfx::ViewCamera& cam)
    {
        Transform t = c.LocalTransform;
        if (c.Facing != SceneNS::CardFacing::Fixed) {
            const Vec3 d = cam.Position - t.Position;
            t.Rotation.y = std::atan2(d.x, d.z);
            if (c.Facing == SceneNS::CardFacing::FaceCamera)
                t.Rotation.x = -std::atan2(d.y, std::sqrt(d.x * d.x + d.z * d.z));
        }
        return t.Matrix();
    }

    void DrawCube(ImDrawList* dl, const Projector& P, const Mat4& m, ImU32 col, f32 thickness)
    {
        Vec3 v[8];
        for (int i = 0; i < 8; i++)
            v[i] = TransformPoint(m, { (i & 1) ? 0.5f : -0.5f, (i & 2) ? 0.5f : -0.5f, (i & 4) ? 0.5f : -0.5f });
        const int e[12][2] = { {0,1},{2,3},{4,5},{6,7},{0,2},{1,3},{4,6},{5,7},{0,4},{1,5},{2,6},{3,7} };
        for (auto& edge : e) Line3(dl, P, v[edge[0]], v[edge[1]], col, thickness);
    }

    void DrawSceneOverlay(EditorContext& ctx, ImDrawList* dl, const Projector& P, ImVec2 origin, ImVec2 size)
    {
        auto& doc = *ctx.Document();
        DrawGrid(dl, P, 10);

        bool hasContent = false;
        for (const auto& node : doc.Nodes())
        {
            if (not node->Visible) continue;
            const bool  sel = node->Id() == ctx.Selection;
            const Mat4  m   = node->LocalTransform.Matrix();
            const Vec3  pos = node->LocalTransform.Position;

            switch (node->Kind())
            {
                case SceneNS::NodeKind::Camera: {
                    const ImU32 col = sel ? Selected : IM_COL32(200, 200, 210, 200);
                    Vec3 apex = TransformPoint(m, { 0, 0, 0 });
                    Vec3 b[4] = { TransformPoint(m, { -0.4f, -0.25f, -0.8f }), TransformPoint(m, { 0.4f, -0.25f, -0.8f }),
                                  TransformPoint(m, { 0.4f, 0.25f, -0.8f }),   TransformPoint(m, { -0.4f, 0.25f, -0.8f }) };
                    for (int i = 0; i < 4; i++) { Line3(dl, P, apex, b[i], col); Line3(dl, P, b[i], b[(i + 1) % 4], col); }
                    LabelAt(dl, P, pos, node->Name().c_str());
                    break;
                }
                case SceneNS::NodeKind::Light: {
                    hasContent = true;
                    const ImU32 col = sel ? Selected : IM_COL32(255, 230, 140, 230);
                    ImVec2 s;
                    if (P(pos, s)) { dl->AddCircle(s, 8.0f, col, 16, 2.0f); dl->AddCircleFilled(s, 3.0f, col); }
                    Line3(dl, P, pos, TransformPoint(m, { 0, 0, -1.5f }), col, 1.5f);
                    LabelAt(dl, P, pos, node->Name().c_str());
                    break;
                }
                case SceneNS::NodeKind::Canvas: {
                    hasContent = true;
                    auto& canvas = static_cast<const SceneNS::Canvas&>(*node);
                    const Mat4 world = CanvasWorld(canvas, ctx.View.Camera);
                    // Ghost planes for layers pushed off the page plane (multiplane depth).
                    for (auto& layer : canvas.Document.Layers()) {
                        if (layer.DepthTransform.Position.z == 0.0f and layer.DepthTransform.Position.x == 0.0f
                            and layer.DepthTransform.Position.y == 0.0f) continue;
                        Vec3 q[4]; CanvasCorners(canvas, world * layer.DepthTransform.Matrix(), q);
                        Quad3(dl, P, q, RoleColor(canvas.Role, 18), RoleColor(canvas.Role, 90), 1.0f);
                    }
                    Vec3 q[4]; CanvasCorners(canvas, world, q);
                    Quad3(dl, P, q, RoleColor(canvas.Role, sel ? 70 : 45), sel ? Selected : RoleColor(canvas.Role, 220), sel ? 2.5f : 1.5f);
                    LabelAt(dl, P, q[3], node->Name().c_str());
                    break;
                }
                case SceneNS::NodeKind::Model: {
                    hasContent = true;
                    auto& model = static_cast<const SceneNS::Model&>(*node);
                    const auto& bc = model.Surface.BaseColor;
                    DrawCube(dl, P, m, sel ? Selected : IM_COL32(int(bc.x * 255), int(bc.y * 255), int(bc.z * 255), 230), sel ? 2.5f : 1.5f);
                    LabelAt(dl, P, TransformPoint(m, { 0.5f, 0.5f, 0.5f }), node->Name().c_str());
                    break;
                }
                case SceneNS::NodeKind::Group: break;
            }
        }

        if (not hasContent) {
            const char* hint = "Empty scene - use Add > Canvas, Model or Light to start";
            ImVec2 ts = ImGui::CalcTextSize(hint);
            dl->AddText({ origin.x + (size.x - ts.x) * 0.5f, origin.y + size.y * 0.5f + 40 }, IM_COL32(220, 220, 230, 160), hint);
        }
    }

    void DrawCanvasDepthOverlay(EditorContext& ctx, ImDrawList* dl, const Projector& P)
    {
        auto* canvas = ctx.FocusedCanvas();
        if (not canvas) return;
        DrawGrid(dl, P, 6, 0.6f);

        // The canvas in isolation, in its own local space.
        auto& layers = canvas->Document.Layers();
        for (usize i = 0; i < layers.size(); i++) {
            auto& layer = layers[i];
            if (not layer.Visible) continue;
            const bool active = layer.Id == canvas->Document.ActiveLayer;
            Vec3 q[4]; CanvasCorners(*canvas, layer.DepthTransform.Matrix(), q);
            Quad3(dl, P, q, LayerColor(i, active ? 70 : 35), active ? Selected : LayerColor(i, 220), active ? 2.5f : 1.2f);
            LabelAt(dl, P, q[3], layer.Name.c_str(), active ? Selected : Label);
            char depth[32];
            std::snprintf(depth, sizeof(depth), "z %.2f", layer.DepthTransform.Position.z);
            ImVec2 s;
            if (P(q[1], s)) dl->AddText({ s.x - 44, s.y + 4 }, IM_COL32(200, 200, 210, 150), depth);
        }
    }

    void DrawModelOverlay(EditorContext& ctx, ImDrawList* dl, const Projector& P, ImVec2 origin, ImVec2 size)
    {
        auto* model = ctx.FocusedModel();
        if (not model) return;
        DrawGrid(dl, P, 4, 0.8f);
        const auto& bc = model->Surface.BaseColor;
        Mat4 m = Translation({ 0, 0.5f * model->LocalTransform.Scale.y, 0 }) * Scaling(model->LocalTransform.Scale);
        DrawCube(dl, P, m, IM_COL32(int(bc.x * 255), int(bc.y * 255), int(bc.z * 255), 255), 2.0f);
        if (model->MeshAsset.empty()) {
            const char* hint = "Placeholder cube - no mesh assigned (mesh import is a TODO)";
            ImVec2 ts = ImGui::CalcTextSize(hint);
            dl->AddText({ origin.x + (size.x - ts.x) * 0.5f, origin.y + size.y - 40 }, IM_COL32(220, 220, 230, 160), hint);
        }
    }

    void DrawCanvasFlatOverlay(EditorContext& ctx, ImDrawList* dl, ImVec2 origin, ImVec2 size, bool hovered)
    {
        auto* canvas = ctx.FocusedCanvas();
        if (not canvas) return;

        Vec4 r = Gfx::CanvasPageRect(ctx.View.Camera, canvas->Document.Size, { size.x, size.y });
        ImVec2 p0 = { origin.x + r.x, origin.y + r.y }, p1 = { p0.x + r.z, p0.y + r.w };
        dl->AddRect({ p0.x - 1, p0.y - 1 }, { p1.x + 1, p1.y + 1 }, IM_COL32(0, 0, 0, 120));

        char text[200];
        std::snprintf(text, sizeof(text), "%.0f x %.0f px", canvas->Document.Size.x, canvas->Document.Size.y);
        dl->AddText({ p0.x, p1.y + 6 }, IM_COL32(200, 200, 210, 170), text);

        if (hovered) {
            ImVec2 m = ImGui::GetIO().MousePos;
            const f32 scale = r.z / canvas->Document.Size.x;
            std::snprintf(text, sizeof(text), "page %.0f, %.0f", (m.x - p0.x) / scale, (m.y - p0.y) / scale);
            ImVec2 ts = ImGui::CalcTextSize(text);
            dl->AddText({ origin.x + size.x - ts.x - 10, origin.y + size.y - 22 }, IM_COL32(220, 220, 230, 200), text);
        }
    }

    // Converts viewport input into Drawing::PointerEvents for the active tool.
    void ForwardToTool(EditorContext& ctx, ImVec2 origin, ImVec2 size)
    {
        auto* canvas = ctx.FocusedCanvas();
        if (not canvas) return;
        auto* layer = canvas->Document.Active();
        auto* tool  = ctx.ToolFor(ctx.ActiveDrawingMode);
        if (not layer or not tool or layer->Locked) return;
        auto* drawing = layer->DrawingAt(ctx.Timeline.Current());
        if (not drawing) return;

        Vec4 r = Gfx::CanvasPageRect(ctx.View.Camera, canvas->Document.Size, { size.x, size.y });
        const f32 scale = r.z / canvas->Document.Size.x;
        ImVec2 m = ImGui::GetIO().MousePos;

        Draw::PointerEvent e;
        e.Position = { (m.x - origin.x - r.x) / scale, (m.y - origin.y - r.y) / scale };
        e.Time     = ImGui::GetTime();
        // TODO(pen): pressure/tilt from SDL pen events via SDL::AddEventHook.

        Draw::ToolTarget target { .Canvas = canvas->Document, .ActiveLayer = *layer, .ActiveDrawing = *drawing, .Brush = ctx.Brush };
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))       { tool->OnPointerDown(target, e); ctx.MarkDirty(); }
        else if (ImGui::IsMouseDown(ImGuiMouseButton_Left))       tool->OnPointerMove(target, e);
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))        tool->OnPointerUp(target, e);
    }
}

int ViewportPanel::WindowFlags() const
{
    return ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
}

void ViewportPanel::Draw(EditorContext& ctx)
{
    if (not ctx.Document()) return;

    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x < 8 or avail.y < 8) return;
    const f32 dpi = ImGui::GetIO().DisplayFramebufferScale.x;
    ctx.Renderer.RequestViewportSize(u32(avail.x * dpi), u32(avail.y * dpi));

    const Gfx::ViewKind kind = ViewKindFor(ctx.Mode);
    const bool flat = kind == Gfx::ViewKind::CanvasFlat;
    ViewNavigator& nav = ctx.Navigator();

    // Capture input over the whole view.
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##viewport", avail, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    const bool hovered = ImGui::IsItemHovered();
    const bool active  = ImGui::IsItemActive();
    ImGuiIO& io = ImGui::GetIO();

    // Navigation: right-drag orbit (pan in 2D), middle-drag / shift+right pan, wheel zoom, F to frame.
    if (active or hovered) {
        const ImVec2 d = io.MouseDelta;
        if (ImGui::IsMouseDown(ImGuiMouseButton_Middle) or (ImGui::IsMouseDown(ImGuiMouseButton_Right) and (flat or io.KeyShift)))
            nav.Pan(d.x, d.y, avail.y, flat);
        else if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
            nav.Orbit(d.x, d.y);
        if (hovered and io.MouseWheel != 0.0f) nav.Zoom(io.MouseWheel, flat);
        if (hovered and ImGui::IsKeyPressed(ImGuiKey_F)) {
            if (flat) { if (auto* c = ctx.FocusedCanvas()) nav.FrameCanvas(c->Document.Size); }
            else nav = ViewNavigator {};
        }
    }

    ctx.View.Kind   = kind;
    ctx.View.Focus  = ctx.Focus;
    ctx.View.Camera = nav.Camera(kind);

    // Engine output
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const auto& target = ctx.Renderer.Viewport();
    const ImVec2 end = { origin.x + avail.x, origin.y + avail.y };
    if (target.UITexture()) dl->AddImage(ImTextureRef(ImTextureID(target.UITexture())), origin, end);

    // Overlays
    dl->PushClipRect(origin, end, true);
    const f32 aspect = avail.x / avail.y;
    Projector P { ctx.View.Camera.Projection(aspect) * ctx.View.Camera.View(), origin, avail };

    switch (kind) {
        case Gfx::ViewKind::Scene:       DrawSceneOverlay(ctx, dl, P, origin, avail);     break;
        case Gfx::ViewKind::CanvasDepth: DrawCanvasDepthOverlay(ctx, dl, P);              break;
        case Gfx::ViewKind::ModelFocus:  DrawModelOverlay(ctx, dl, P, origin, avail);     break;
        case Gfx::ViewKind::CanvasFlat:  DrawCanvasFlatOverlay(ctx, dl, origin, avail, hovered); break;
    }

    // Status line
    char status[256];
    const char* tool = ctx.Mode == EditorModeKind::Canvas2D ? Draw::DrawingModeName(ctx.ActiveDrawingMode)
                     : ctx.Mode == EditorModeKind::Model    ? ModelToolName(ctx.ActiveModelTool)
                     :                                        SceneToolName(ctx.ActiveSceneTool);
    std::snprintf(status, sizeof(status), "%s  |  %ux%u  |  %s  |  frame %lld  |  %s",
        ctx.Renderer.EngineName().c_str(), target.Extent().width, target.Extent().height,
        EditorModeName(ctx.Mode), (long long)ctx.Timeline.Current(), tool);
    dl->AddText({ origin.x + 10, origin.y + 8 }, IM_COL32(230, 230, 235, 210), status);

    if (flat and not ctx.ToolFor(ctx.ActiveDrawingMode)) {
        char note[160];
        std::snprintf(note, sizeof(note), "No Drawing::Tool registered for \"%s\" yet", Draw::DrawingModeName(ctx.ActiveDrawingMode));
        dl->AddText({ origin.x + 10, origin.y + 26 }, IM_COL32(230, 200, 140, 180), note);
    }
    dl->PopClipRect();

    if (flat and hovered) ForwardToTool(ctx, origin, avail);
}
