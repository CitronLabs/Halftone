module;
#include <__Vendor/ImGui/imgui.h>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

module com.citronlabs.Halftone.Editor;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Animation;
import com.citronlabs.Halftone.Drawing;
import com.citronlabs.Halftone.Scene;

using namespace Halftone::Editor;
using namespace Halftone::Core;
namespace Draw = Halftone::Drawing;

int TimelinePanel::WindowFlags() const { return 0; }

namespace
{
    // Exposure strip for one layer: one cell per frame, solid where a new
    // drawing starts, pale where the previous drawing is held.
    void DrawExposureStrip(Context& ctx, Draw::Layer& layer)
    {
        auto& timeline = ctx.Timeline;
        const auto range = timeline.Range();

        ImGui::Text("Layer \"%s\"", layer.Name.c_str());
        ImGui::SameLine();

        auto& cadence = layer.Exposures.GetCadence();
        int stepIndex = std::clamp(int(cadence.Step) - 1, 0, 3);
        const char* steps[] = { "Ones", "Twos", "Threes", "Fours" };
        ImGui::SetNextItemWidth(90);
        if (ImGui::Combo("Cadence", &stepIndex, steps, 4)) { cadence.Step = u32(stepIndex + 1); ctx.MarkDirty(); }
        ImGui::SameLine();
        if (ImGui::Button("New Drawing Here")) {
            Draw::Drawing d;
            d.Id   = ctx.FocusedCanvas()->Document.NewId();
            d.Name = std::to_string(timeline.Current());
            layer.Exposures.Expose(timeline.Current(), d.Id);
            layer.Drawings.push_back(std::move(d));
            ctx.MarkDirty();
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Exposure")) { layer.Exposures.Clear(timeline.Current()); ctx.MarkDirty(); }

        // Strip
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const f32 cell = 14.0f;
        const i64 count = std::min<i64>(range.Length(), i64((ImGui::GetContentRegionAvail().x) / cell));
        ImVec2 p = ImGui::GetCursorScreenPos();
        ID previous = NullID;
        for (i64 i = 0; i < count; i++) {
            const Frame f = range.Start + i;
            const ID shown = layer.Exposures.DrawingAt(f);
            ImVec2 a = { p.x + i * cell, p.y }, b = { a.x + cell - 2, a.y + cell - 2 };
            ImU32 col = shown == NullID ? IM_COL32(50, 52, 58, 255)
                      : shown != previous ? IM_COL32(90, 190, 250, 255) : IM_COL32(40, 70, 95, 255);
            dl->AddRectFilled(a, b, col, 2.0f);
            if (f == timeline.Current()) dl->AddRect({ a.x - 1, a.y - 1 }, { b.x + 1, b.y + 1 }, IM_COL32(255, 170, 40, 255), 2.0f, 0, 2.0f);
            previous = shown;
        }
        ImGui::Dummy({ count * cell, cell });
    }
}

void TimelinePanel::Draw(Context& ctx)
{
    auto& timeline = ctx.Timeline;

    if (ImGui::Button("|<")) timeline.SetFrame(timeline.Range().Start);
    ImGui::SameLine();
    if (ImGui::Button("<"))  timeline.Step(-1);
    ImGui::SameLine();
    if (ImGui::Button(timeline.Playing() ? "Pause" : "Play")) timeline.TogglePlay();
    ImGui::SameLine();
    if (ImGui::Button(">"))  timeline.Step(1);
    ImGui::SameLine();
    if (ImGui::Button(">|")) timeline.SetFrame(timeline.Range().End);
    ImGui::SameLine();

    bool loop = timeline.Looping();
    if (ImGui::Checkbox("Loop", &loop)) timeline.SetLoop(loop);
    ImGui::SameLine();
    ImGui::TextDisabled("%.3f fps", timeline.Rate().FramesPerSecond());

    int frame = int(timeline.Current());
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderInt("##frame", &frame, int(timeline.Range().Start), int(timeline.Range().End)))
        timeline.SetFrame(Frame(frame));

    const bool canvasMode = ctx.Mode == EditorModeKind::Canvas2D or ctx.Mode == EditorModeKind::Canvas3D;
    auto* canvas = canvasMode ? ctx.FocusedCanvas() : nullptr;
    auto* layer  = canvas ? canvas->Document.Active() : nullptr;

    if (layer) {
        DrawExposureStrip(ctx, *layer);
    } else {
        const auto twos   = Halftone::Animation::Cadence::Twos();
        const auto threes = Halftone::Animation::Cadence::Threes();
        ImGui::TextDisabled("Held drawing  -  ones: %lld   twos: %lld   threes: %lld",
            (long long)frame, (long long)twos.HeldFrame(frame), (long long)threes.HeldFrame(frame));
        // TODO: camera keyframe lane and per-canvas exposure overview.
    }
}
