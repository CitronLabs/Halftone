module;
#include <__Vendor/ImGui/imgui.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

module com.citronlabs.Halftone.Editor;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.App;

using namespace Halftone::Editor;
using namespace Halftone::Core;

void SplashScreen::Update(EditorContext& /*ctx*/, f64 dt)
{
    _Elapsed += dt;
    if (_Failed or _Next >= _Steps.size()) return;

    // One step per frame so the progress bar redraws between steps.
    auto& step = _Steps[_Next];
    Log::Info("Startup", "{}", step.Label);
    if (step.Run and not step.Run()) {
        _Failed     = true;
        _FailedStep = step.Label;
        Log::Error("Startup", "Failed: {}", step.Label);
        return;
    }
    _Next++;
}

void SplashScreen::Draw(EditorContext& ctx)
{
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(16, 18, 22, 255));
    ImGui::Begin("##splash", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 c = { vp->Pos.x + vp->Size.x * 0.5f, vp->Pos.y + vp->Size.y * 0.5f };

    // Halftone dot field behind the title.
    for (int y = -6; y <= 6; y++)
        for (int x = -18; x <= 18; x++) {
            const f32 fade = 1.0f - (std::abs(x) / 18.0f);
            const f32 r    = 1.0f + 3.2f * fade * (1.0f - std::abs(y) / 7.0f);
            dl->AddCircleFilled({ c.x + x * 22.0f, c.y - 40 + y * 22.0f }, r, IM_COL32(60, 170, 230, int(40 + 60 * fade)));
        }

    ImGui::PushFont(nullptr, 64.0f);
    const char* title = "HALFTONE";
    ImVec2 ts = ImGui::CalcTextSize(title);
    dl->AddText(ImGui::GetFont(), 64.0f, { c.x - ts.x * 0.5f, c.y - 80 }, IM_COL32(240, 240, 245, 255), title);
    ImGui::PopFont();

    char sub[96];
    std::snprintf(sub, sizeof(sub), "Version %s  -  %s engine", Halftone::App::Info::Version, Halftone::App::Info::Engine);
    ImVec2 ss = ImGui::CalcTextSize(sub);
    dl->AddText({ c.x - ss.x * 0.5f, c.y + 2 }, IM_COL32(170, 175, 185, 255), sub);

    // Progress
    const f32 barW = 360.0f;
    const f32 progress = _Steps.empty() ? 1.0f : f32(_Next) / f32(_Steps.size());
    ImGui::SetCursorScreenPos({ c.x - barW * 0.5f, c.y + 60 });
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, IM_COL32(60, 170, 230, 255));
    ImGui::ProgressBar(std::min(progress, f32(_Elapsed / MinimumSeconds)), { barW, 6 }, "");
    ImGui::PopStyleColor();

    const char* label = _Failed ? _FailedStep.c_str()
                      : _Next < _Steps.size() ? _Steps[_Next].Label.c_str() : "Ready";
    ImVec2 ls = ImGui::CalcTextSize(label);
    dl->AddText({ c.x - ls.x * 0.5f, c.y + 76 }, _Failed ? IM_COL32(240, 110, 110, 255) : IM_COL32(150, 155, 165, 255), label);

    if (_Failed) {
        ImGui::SetCursorScreenPos({ c.x - 40, c.y + 110 });
        if (ImGui::Button("Quit", { 80, 0 })) ctx.QuitRequested = true;
    }

    ImGui::End();
    ImGui::PopStyleColor();
}
