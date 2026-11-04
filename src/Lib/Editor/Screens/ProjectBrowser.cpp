module;
#include <__Vendor/ImGui/imgui.h>
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

module com.citronlabs.Halftone.Editor;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.App;
import com.citronlabs.Halftone.Project;

using namespace Halftone::Editor;
using namespace Halftone::Core;
namespace Proj = Halftone::Project;

namespace
{
    const char* KindTag(Proj::ItemKind kind)
    {
        switch (kind) {
            case Proj::ItemKind::Project:    return "[P]";
            case Proj::ItemKind::Subproject: return "[S]";
            case Proj::ItemKind::Scene:      return "[#]";
        }
        return "";
    }
}

void ProjectBrowserScreen::OnEnter(EditorContext& ctx)
{
    ctx.Library.Rescan();
    _Selected = _SelectPath.empty() ? nullptr : ctx.Library.FindByPath(_SelectPath);
    _Modified = false;
}

void ProjectBrowserScreen::Select(Proj::ProjectItem* item)
{
    _Selected   = item;
    _SelectPath = item ? item->Path() : String();
    _Modified   = false;
}

void ProjectBrowserScreen::BeginCreate(NewKind kind, Proj::ProjectItem* parent)
{
    _NewKind   = kind;
    _NewParent = parent;
    const char* defaults[] = { "", "New Project", "New Subproject", "New Scene" };
    std::snprintf(_NewName, sizeof(_NewName), "%s", defaults[u32(kind)]);
    _OpenNewPopup = true;
}

void ProjectBrowserScreen::Draw(EditorContext& ctx)
{
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImGuiWindowFlags fixed = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse
                                 | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
    const f32 toolbarH = 46.0f;
    const f32 treeW    = std::max(320.0f, vp->Size.x * 0.28f);

    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize({ vp->Size.x, toolbarH });
    ImGui::Begin("##browser-toolbar", nullptr, fixed | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
    DrawToolbar(ctx);
    ImGui::End();

    ImGui::SetNextWindowPos({ vp->Pos.x, vp->Pos.y + toolbarH });
    ImGui::SetNextWindowSize({ treeW, vp->Size.y - toolbarH });
    ImGui::Begin("Projects", nullptr, fixed);
    DrawTree(ctx);
    ImGui::End();

    ImGui::SetNextWindowPos({ vp->Pos.x + treeW, vp->Pos.y + toolbarH });
    ImGui::SetNextWindowSize({ vp->Size.x - treeW, vp->Size.y - toolbarH });
    ImGui::Begin("Inspector", nullptr, fixed);
    if (_Selected)                         DrawInspector(ctx);
    else if (ctx.Library.Projects().empty()) DrawEmptyState(ctx);
    else                                   ImGui::TextDisabled("Select a project, subproject or scene.");
    ImGui::End();

    DrawNewItemPopup(ctx);
}

void ProjectBrowserScreen::DrawToolbar(EditorContext& ctx)
{
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", Halftone::App::Info::Name);
    ImGui::SameLine();
    ImGui::TextDisabled("Project Library  -  %s", ctx.Library.Root().c_str());

    const f32 right = ImGui::GetContentRegionMax().x;
    ImGui::SameLine(right - 260);
    if (ImGui::Button("New Project")) BeginCreate(NewKind::Project, nullptr);
    ImGui::SameLine();
    if (ImGui::Button("Rescan")) { ctx.Library.Rescan(); _Selected = ctx.Library.FindByPath(_SelectPath); }
    ImGui::SameLine();
    if (ImGui::Button("Quit")) ctx.QuitRequested = true;
}

void ProjectBrowserScreen::DrawTree(EditorContext& ctx)
{
    for (auto& project : ctx.Library.Projects()) DrawTreeItem(ctx, *project);
    if (ctx.Library.Projects().empty()) ImGui::TextDisabled("No projects yet.");
}

void ProjectBrowserScreen::DrawTreeItem(EditorContext& ctx, Proj::ProjectItem& item)
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick
                             | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
    if (not item.IsContainer()) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    if (&item == _Selected)     flags |= ImGuiTreeNodeFlags_Selected;

    char label[512];
    std::snprintf(label, sizeof(label), "%s %s###%s", KindTag(item.Kind()), item.Name().c_str(), item.Path().c_str());
    const bool open = ImGui::TreeNodeEx(label, flags);

    if (ImGui::IsItemClicked() and not ImGui::IsItemToggledOpen()) Select(&item);
    if (not item.IsContainer() and ImGui::IsItemHovered() and ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        ctx.RequestOpenScene(item);

    if (ImGui::BeginPopupContextItem()) {
        Select(&item);
        if (item.IsContainer()) {
            if (ImGui::MenuItem("New Scene..."))      BeginCreate(NewKind::Scene, &item);
            if (ImGui::MenuItem("New Subproject...")) BeginCreate(NewKind::Subproject, &item);
        } else if (ImGui::MenuItem("Open Scene")) {
            ctx.RequestOpenScene(item);
        }
        ImGui::EndPopup();
    }

    if (open and item.IsContainer()) {
        for (auto& child : item.Children()) DrawTreeItem(ctx, *child);
        ImGui::TreePop();
    }
}

void ProjectBrowserScreen::DrawInspector(EditorContext& ctx)
{
    auto& item = *_Selected;

    ImGui::TextDisabled("%s", Proj::ItemKindName(item.Kind()));
    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.6f);
    ImGui::TextUnformatted(item.Name().c_str());
    ImGui::PopFont();
    ImGui::TextDisabled("%s", item.DisplayPath().c_str());
    ImGui::TextDisabled("%s", item.Path().c_str());
    ImGui::Spacing();

    if (item.Kind() == Proj::ItemKind::Scene) {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(40, 120, 190, 255));
        if (ImGui::Button("Open Scene", { 160, 34 })) ctx.RequestOpenScene(item);
        ImGui::PopStyleColor();
        ImGui::SameLine();
    } else {
        if (ImGui::Button("New Scene"))      BeginCreate(NewKind::Scene, &item);
        ImGui::SameLine();
        if (ImGui::Button("New Subproject")) BeginCreate(NewKind::Subproject, &item);
        ImGui::SameLine();
    }

    ImGui::BeginDisabled(not _Modified);
    if (ImGui::Button(_Modified ? "Save Changes" : "Saved")) { if (ctx.Library.Save(item)) _Modified = false; }
    ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::SeparatorText("Settings");
    _Modified |= InputString("Description", item.Description, true);

    if (item.Kind() != Proj::ItemKind::Scene) {
        usize scenes = 0, subs = 0;
        for (auto& c : item.Children()) (c->IsContainer() ? subs : scenes)++;
        ImGui::TextDisabled("%zu scene(s), %zu subproject(s)", scenes, subs);
    }

    ImGui::Spacing();
    ImGui::SeparatorText(item.Kind() == Proj::ItemKind::Scene ? "Exposed to this scene" : "Exposed to everything in this folder");
    _Modified |= DrawResourceEditor(item);
}

void ProjectBrowserScreen::DrawEmptyState(EditorContext& /*ctx*/)
{
    ImGui::Dummy({ 0, 40 });
    ImGui::TextUnformatted("Your project library is empty.");
    ImGui::TextDisabled("A project is a folder that holds scenes and subprojects, plus the asset packs,");
    ImGui::TextDisabled("build systems, macros and configs they share.");
    ImGui::Spacing();
    if (ImGui::Button("Create your first project", { 240, 34 })) BeginCreate(NewKind::Project, nullptr);
}

void ProjectBrowserScreen::DrawNewItemPopup(EditorContext& ctx)
{
    const char* titles[] = { "", "New Project", "New Subproject", "New Scene" };
    if (_OpenNewPopup) { ImGui::OpenPopup("Create###create"); _OpenNewPopup = false; }

    ImGui::SetNextWindowSize({ 380, 0 });
    if (not ImGui::BeginPopupModal("Create###create", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
        return;

    ImGui::TextUnformatted(titles[u32(_NewKind)]);
    if (_NewParent) ImGui::TextDisabled("in %s", _NewParent->DisplayPath().c_str());
    if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
    const bool enter = ImGui::InputText("Name", _NewName, sizeof(_NewName), ImGuiInputTextFlags_EnterReturnsTrue);

    if ((ImGui::Button("Create", { 120, 0 }) or enter) and _NewName[0])
    {
        Proj::ProjectItem* created = nullptr;
        switch (_NewKind) {
            case NewKind::Project:    created = ctx.Library.CreateProject(_NewName); break;
            case NewKind::Subproject: created = _NewParent ? ctx.Library.CreateSubproject(*_NewParent, _NewName) : nullptr; break;
            case NewKind::Scene:      created = _NewParent ? ctx.Library.CreateScene(*_NewParent, _NewName) : nullptr; break;
            case NewKind::None: break;
        }
        if (created) Select(created);
        _NewKind = NewKind::None;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", { 120, 0 })) { _NewKind = NewKind::None; ImGui::CloseCurrentPopup(); }
    ImGui::EndPopup();
}
