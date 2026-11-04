module;
#include <__Vendor/ImGui/imgui.h>
#include <memory>
#include <string>
#include <vector>

module com.citronlabs.Halftone.Editor;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.App;
import com.citronlabs.Halftone.Project;
import com.citronlabs.Halftone.Scene;

using namespace Halftone::Editor;
using namespace Halftone::Core;
namespace SceneNS = Halftone::Scene;

SceneEditorScreen::SceneEditorScreen()
{
    _Modes[u32(EditorModeKind::Scene)]    = MakeUnique<SceneLayoutMode>();
    _Modes[u32(EditorModeKind::Canvas2D)] = MakeUnique<Canvas2DMode>();
    _Modes[u32(EditorModeKind::Canvas3D)] = MakeUnique<Canvas3DMode>();
    _Modes[u32(EditorModeKind::Model)]    = MakeUnique<ModelEditMode>();
}

SceneEditorScreen::~SceneEditorScreen() = default;

void SceneEditorScreen::Draw(EditorContext& ctx)
{
    if (not ctx.Open) return;
    EditorMode& mode = ModeFor(ctx.Mode);

    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_S)) ctx.RequestSave();
    if (ImGui::IsKeyPressed(ImGuiKey_Escape) and ctx.Mode != EditorModeKind::Scene and not ImGui::IsAnyItemActive())
        ctx.RequestMode(EditorModeKind::Scene);

    DrawMainMenu(ctx, mode);
    const f32 top = DrawModeBar(ctx, ImGui::GetFrameHeight());
    Workspace::Draw(ctx, mode.Panels(), top);

    if (_ShowSceneSettings) DrawSceneSettings(ctx);
    if (_ShowImGuiDemo)     ImGui::ShowDemoWindow(&_ShowImGuiDemo);
    DrawCloseConfirmation(ctx);
}

void SceneEditorScreen::DrawMainMenu(EditorContext& ctx, EditorMode& mode)
{
    if (not ImGui::BeginMainMenuBar()) return;

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Save Scene", "Ctrl+S", false, ctx.Open->Dirty)) ctx.RequestSave();
        if (ImGui::MenuItem("Scene Settings...")) _ShowSceneSettings = true;
        ImGui::Separator();
        if (ImGui::MenuItem("Close Scene")) {
            if (ctx.Open->Dirty) _ConfirmClose = true;
            else                 ctx.RequestCloseScene();
        }
        if (ImGui::MenuItem("Quit")) ctx.QuitRequested = true;
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
        // TODO: command history (Halftone.Editor:Commands) for undo/redo + macros.
        ImGui::MenuItem("Undo", "Ctrl+Z", false, false);
        ImGui::MenuItem("Redo", "Ctrl+Y", false, false);
        ImGui::EndMenu();
    }

    mode.DrawMenus(ctx);

    if (ImGui::BeginMenu("View")) {
        for (auto& panel : mode.Panels()) ImGui::MenuItem(panel->Name(), nullptr, &panel->OpenFlag());
        ImGui::Separator();
        ImGui::MenuItem("ImGui Demo", nullptr, &_ShowImGuiDemo);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Render")) {
        for (auto& name : ctx.Renderer.Registry().Names())
            if (ImGui::MenuItem(name.c_str(), nullptr, name == ctx.Renderer.EngineName()))
                ctx.Renderer.SetEngine(name);
        ImGui::EndMenu();
    }

    ImGui::TextDisabled("  %s %s", Halftone::App::Info::Name, Halftone::App::Info::Version);
    ImGui::EndMainMenuBar();
}

// Breadcrumb + mode switcher under the menu bar:
//   Project / Sub / Scene  >  Canvas "Hero"      [Scene] [Canvas 2D] [Canvas 3D] [Model]
f32 SceneEditorScreen::DrawModeBar(EditorContext& ctx, f32 top)
{
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const f32 height = ImGui::GetFrameHeight() + 12.0f;
    ImGui::SetNextWindowPos({ vp->Pos.x, vp->Pos.y + top });
    ImGui::SetNextWindowSize({ vp->Size.x, height });
    ImGui::Begin("##modebar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings
                                      | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar);

    ImGui::AlignTextToFramePadding();
    const String path = ctx.Open->Item ? ctx.Open->Item->DisplayPath() : ctx.Open->Document.Name();
    ImGui::TextDisabled("%s", path.c_str());
    if (ctx.Open->Dirty) { ImGui::SameLine(0, 4); ImGui::TextDisabled("*"); }

    auto* doc   = ctx.Document();
    auto* focus = doc->Find(ctx.Focus);
    if (focus and ctx.Mode != EditorModeKind::Scene) {
        ImGui::SameLine();
        ImGui::TextUnformatted(">");
        ImGui::SameLine();
        ImGui::Text("%s \"%s\"", SceneNS::NodeKindName(focus->Kind()), focus->Name().c_str());
    }

    // Mode buttons on the right. Canvas modes need a canvas, Model needs a model.
    const ID     sel       = ctx.Selection;
    const bool   canCanvas = (focus and focus->Kind() == SceneNS::NodeKind::Canvas) or doc->FindAs<SceneNS::Canvas>(sel);
    const bool   canModel  = (focus and focus->Kind() == SceneNS::NodeKind::Model)  or doc->FindAs<SceneNS::Model>(sel);
    auto target = [&](SceneNS::NodeKind kind) -> ID {
        if (focus and focus->Kind() == kind) return focus->Id();
        auto* s = doc->Find(sel);
        return s and s->Kind() == kind ? sel : NullID;
    };

    const f32 buttonsW = 4 * 96.0f + 3 * ImGui::GetStyle().ItemSpacing.x;
    ImGui::SameLine(ImGui::GetContentRegionMax().x - buttonsW);
    auto modeButton = [&](EditorModeKind m, bool enabled, ID f) {
        const bool current = ctx.Mode == m;
        ImGui::BeginDisabled(not enabled and not current);
        if (current) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::Button(EditorModeName(m), { 96, 0 }) and not current) ctx.RequestMode(m, f);
        if (current) ImGui::PopStyleColor();
        ImGui::EndDisabled();
        ImGui::SameLine();
    };
    modeButton(EditorModeKind::Scene,    true,      NullID);
    modeButton(EditorModeKind::Canvas2D, canCanvas, target(SceneNS::NodeKind::Canvas));
    modeButton(EditorModeKind::Canvas3D, canCanvas, target(SceneNS::NodeKind::Canvas));
    modeButton(EditorModeKind::Model,    canModel,  target(SceneNS::NodeKind::Model));

    ImGui::End();
    return top + height;
}

void SceneEditorScreen::DrawSceneSettings(EditorContext& ctx)
{
    if (not ctx.Open->Item) { _ShowSceneSettings = false; return; }
    ImGui::SetNextWindowSize({ 560, 520 }, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene Settings", &_ShowSceneSettings)) {
        auto& item = *ctx.Open->Item;
        ImGui::TextDisabled("%s", item.DisplayPath().c_str());
        bool changed = InputString("Description", item.Description, true);
        changed |= DrawResourceEditor(item);
        if (changed) ctx.MarkDirty();   // saved together with the scene
    }
    ImGui::End();
}

void SceneEditorScreen::DrawCloseConfirmation(EditorContext& ctx)
{
    if (_ConfirmClose) { ImGui::OpenPopup("Close Scene?"); _ConfirmClose = false; }
    if (ImGui::BeginPopupModal("Close Scene?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Save changes to \"%s\" before closing?", ctx.Open->Document.Name().c_str());
        if (ImGui::Button("Save", { 100, 0 }))    { ctx.RequestSave(); ctx.RequestCloseScene(); ImGui::CloseCurrentPopup(); }
        ImGui::SameLine();
        if (ImGui::Button("Discard", { 100, 0 })) { ctx.RequestCloseScene(); ImGui::CloseCurrentPopup(); }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", { 100, 0 }))  ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}
