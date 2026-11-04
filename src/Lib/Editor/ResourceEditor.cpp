module;
#include <__Vendor/ImGui/imgui.h>
#include <string>
#include <utility>
#include <vector>

module com.citronlabs.Halftone.Editor;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Project;

using namespace Halftone::Editor;
using namespace Halftone::Core;
namespace Proj = Halftone::Project;

namespace
{
    // Edits a JSON value as text; commits when the text parses.
    bool InputJson(const char* label, json& value, bool multiline)
    {
        String text = value.dump(multiline ? 2 : -1);
        if (InputString(label, text, multiline)) {
            json parsed = json::parse(text, nullptr, false, true);
            if (not parsed.is_discarded()) { value = std::move(parsed); return true; }
        }
        return false;
    }

    bool StringList(const char* label, List<String>& items, const char* addLabel)
    {
        bool changed = false;
        ImGui::TextDisabled("%s", label);
        for (usize i = 0; i < items.size(); i++) {
            ImGui::PushID(int(i));
            ImGui::SetNextItemWidth(-32.0f);
            changed |= InputString("##item", items[i]);
            ImGui::SameLine();
            if (ImGui::SmallButton("x")) { items.erase(items.begin() + i); changed = true; ImGui::PopID(); break; }
            ImGui::PopID();
        }
        if (ImGui::SmallButton(addLabel)) { items.emplace_back(); changed = true; }
        return changed;
    }

    template<typename T>
    void InheritedRows(const List<Proj::Scoped<T>>& scoped)
    {
        bool header = false;
        for (auto& s : scoped) {
            if (not s.Inherited) continue;
            if (not header) { ImGui::Spacing(); ImGui::SeparatorText("Inherited"); header = true; }
            ImGui::BulletText("%s", s.Resource->Name.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("from %s %s", Proj::ItemKindName(s.Owner->Kind()), s.Owner->Name().c_str());
        }
    }

    // Generic "own items" list: header row per item with a delete button,
    // body drawn by `edit`.
    template<typename T, typename Edit>
    bool OwnRows(List<T>& items, const char* addLabel, Edit edit)
    {
        bool changed = false;
        for (usize i = 0; i < items.size(); i++) {
            ImGui::PushID(int(i));
            bool open = ImGui::CollapsingHeader((items[i].Name + "###res").c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);
            ImGui::SameLine(ImGui::GetContentRegionMax().x - 54.0f);
            if (ImGui::SmallButton("Delete")) { items.erase(items.begin() + i); ImGui::PopID(); return true; }
            if (open) {
                ImGui::Indent();
                changed |= InputString("Name", items[i].Name);
                changed |= edit(items[i]);
                ImGui::Unindent();
            }
            ImGui::PopID();
        }
        if (ImGui::Button(addLabel)) { items.emplace_back(); changed = true; }
        return changed;
    }
}

bool Halftone::Editor::DrawResourceEditor(Proj::ProjectItem& item)
{
    bool changed = false;
    const auto resolved = item.Resolve();
    auto& res = item.Resources;

    if (not ImGui::BeginTabBar("resources")) return false;

    auto tabLabel = [&](Proj::ResourceKind kind, usize total) {
        return String(Proj::ResourceKindName(kind)) + " (" + std::to_string(total) + ")###" + Proj::ResourceKindName(kind);
    };

    if (ImGui::BeginTabItem(tabLabel(Proj::ResourceKind::AssetPack, resolved.AssetPacks.size()).c_str())) {
        ImGui::TextWrapped("Bundles of brushes, canvases, models and materials that this %s and everything inside it can use.",
                           Proj::ItemKindName(item.Kind()));
        changed |= OwnRows(res.AssetPacks, "+ New Asset Pack", [](Proj::AssetPack& p) {
            bool c = InputString("Description", p.Description);
            c |= StringList("Assets", p.Assets, "+ Add asset path");
            return c;
        });
        InheritedRows(resolved.AssetPacks);
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(tabLabel(Proj::ResourceKind::BuildSystem, resolved.BuildSystems.size()).c_str())) {
        ImGui::TextWrapped("How shots are exported. Each build system names an exporter and its settings.");
        changed |= OwnRows(res.BuildSystems, "+ New Build System", [](Proj::BuildSystem& b) {
            const char* targets[] = { "ImageSequence", "Video", "LayeredExport", "Custom" };
            int current = 3;
            for (int i = 0; i < 4; i++) if (b.Target == targets[i]) current = i;
            bool c = false;
            if (ImGui::Combo("Target", &current, targets, 4)) { b.Target = targets[current]; c = true; }
            c |= InputJson("Settings", b.Settings, true);
            return c;
        });
        InheritedRows(resolved.BuildSystems);
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(tabLabel(Proj::ResourceKind::Macro, resolved.Macros.size()).c_str())) {
        ImGui::TextWrapped("Recorded sequences of editor commands that can be replayed on any scene in scope.");
        changed |= OwnRows(res.Macros, "+ New Macro", [](Proj::Macro& m) {
            return StringList("Steps", m.Steps, "+ Add step");
        });
        InheritedRows(resolved.Macros);
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(tabLabel(Proj::ResourceKind::Config, resolved.Configs.size()).c_str())) {
        ImGui::TextWrapped("Key/value settings. Inner scopes override outer ones key by key.");
        changed |= OwnRows(res.Configs, "+ New Config", [](Proj::Config& c) {
            return InputJson("Values", c.Values, true);
        });
        InheritedRows(resolved.Configs);

        // Effective values after overrides
        json effective = json::object();
        for (auto& c : resolved.Configs)
            for (auto it = c.Resource->Values.begin(); it != c.Resource->Values.end(); ++it) effective[it.key()] = it.value();
        if (not effective.empty()) {
            ImGui::Spacing();
            ImGui::SeparatorText("Effective");
            if (ImGui::BeginTable("effective", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV)) {
                for (auto it = effective.begin(); it != effective.end(); ++it) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); ImGui::TextUnformatted(it.key().c_str());
                    ImGui::TableNextColumn(); ImGui::TextUnformatted(it.value().dump().c_str());
                }
                ImGui::EndTable();
            }
        }
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
    return changed;
}
