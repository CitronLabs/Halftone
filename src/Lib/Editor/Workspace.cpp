module;
#include <__Vendor/ImGui/imgui.h>
#include <memory>
#include <vector>

module com.citronlabs.Halftone.Editor;

import com.citronlabs.Halftone.Core;

using namespace Halftone::Editor;
using namespace Halftone::Core;

void Workspace::Draw(EditorContext& ctx, List<Unique<Panel>>& panels, f32 top)
{
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 origin = { vp->Pos.x, vp->Pos.y + top };
    const ImVec2 size   = { vp->Size.x, vp->Size.y - top };

    bool used[5] = {};
    for (auto& p : panels) if (p->IsOpen()) used[u32(p->Slot())] = true;
    const bool rightCol = used[u32(PanelSlot::Right)] or used[u32(PanelSlot::RightBottom)];

    const f32 scale   = ImGui::GetStyle().FontScaleMain;
    const f32 left    = used[u32(PanelSlot::Left)]   ? 64.0f  * scale : 0.0f;
    const f32 right   = rightCol                     ? 330.0f * scale : 0.0f;
    const f32 bottom  = used[u32(PanelSlot::Bottom)] ? 180.0f * scale : 0.0f;
    const f32 middleH = size.y - bottom;
    const f32 centerW = size.x - left - right;

    // If only one of the right-hand slots is used it takes the full height.
    const f32 split = (used[u32(PanelSlot::Right)] and used[u32(PanelSlot::RightBottom)]) ? 0.48f
                    : (used[u32(PanelSlot::Right)] ? 1.0f : 0.0f);

    auto rectFor = [&](PanelSlot slot, ImVec2& pos, ImVec2& sz) {
        switch (slot) {
            case PanelSlot::Left:        pos = { origin.x, origin.y };                                  sz = { left, middleH };                 break;
            case PanelSlot::Center:      pos = { origin.x + left, origin.y };                           sz = { centerW, middleH };              break;
            case PanelSlot::Right:       pos = { origin.x + left + centerW, origin.y };                 sz = { right, middleH * split };        break;
            case PanelSlot::RightBottom: pos = { origin.x + left + centerW, origin.y + middleH * split }; sz = { right, middleH * (1 - split) }; break;
            case PanelSlot::Bottom:      pos = { origin.x, origin.y + middleH };                        sz = { size.x, bottom };                break;
        }
    };

    const ImGuiWindowFlags baseFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse
                                     | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings;

    for (auto& panel : panels)
    {
        if (not panel->IsOpen()) continue;

        ImVec2 pos, sz;
        rectFor(panel->Slot(), pos, sz);
        ImGui::SetNextWindowPos(pos);
        ImGui::SetNextWindowSize(sz);

        if (ImGui::Begin(panel->Name(), &panel->OpenFlag(), baseFlags | panel->WindowFlags()))
            panel->Draw(ctx);
        ImGui::End();
    }
}
