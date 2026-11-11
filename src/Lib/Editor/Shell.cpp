module;
#include <__Vendor/ImGui/imgui.h>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

module com.citronlabs.Halftone.Editor;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Project;
import com.citronlabs.Halftone.Scene;
import com.citronlabs.Halftone.Animation;

using namespace Halftone::Editor;
using namespace Halftone::Core;
namespace Proj = Halftone::Project;

Shell::Shell(Context& ctx)
    : _Ctx(ctx),
      _Splash(MakeUnique<SplashScreen>()),
      _Browser(MakeUnique<ProjectBrowserScreen>()),
      _Editor(MakeUnique<SceneEditorScreen>())
{
    _Current = _Splash.get();
}

Shell::~Shell() = default;

bool Shell::ViewportActive() const
{
    return _Current and _Current->UsesViewport() and _Ctx.Open;
}

void Shell::SwitchTo(Screen& screen)
{
    if (_Current == &screen) return;
    if (_Current) _Current->OnExit(_Ctx);
    _Current = &screen;
    _Current->OnEnter(_Ctx);
}

void Shell::Frame(f64 dt)
{
    if (_Current == _Splash.get() and _Splash->Finished()) SwitchTo(*_Browser);

    if (_Current == _Editor.get()) _Ctx.Timeline.Update(dt);

    _Current->Update(_Ctx, dt);
    _Current->Draw(_Ctx);

    ApplyNavigation();
}

void Shell::ApplyNavigation()
{
    Navigation nav = _Ctx.Nav;
    _Ctx.Nav = {};

    if (nav.SaveScene and _Ctx.Open and _Ctx.Open->Item) {
        if (_Ctx.Library.SaveScene(*_Ctx.Open->Item, _Ctx.Open->Document)) {
            _Ctx.Open->Dirty = false;
            Log::Info("Editor", "Saved '{}'", _Ctx.Open->Item->Path());
        }
    }

    if (nav.CloseScene and _Ctx.Open) {
        if (_Ctx.Open->Item) _Browser->SelectPath(_Ctx.Open->Item->Path());
        _Ctx.Open.reset();
        _Ctx.Mode = EditorModeKind::Scene;
        _Ctx.Focus = _Ctx.Selection = NullID;
        SwitchTo(*_Browser);
    }

    if (nav.OpenScene) OpenScene(*nav.OpenScene);

    if (nav.ChangeMode and _Ctx.Open) EnterMode(nav.Mode, nav.Focus);
}

void Shell::OpenScene(Proj::ProjectItem& item)
{
    auto open = MakeUnique<Halftone::Editor::OpenScene>();
    open->Item = &item;
    if (not _Ctx.Library.LoadScene(item, open->Document)) return;   // logged by the library

    // Frame rate comes from the scene's effective config (project defaults
    // overridden by subprojects / the scene itself).
    auto resolved = item.Resolve();
    json rate = resolved.ConfigValue("FrameRate", json::array({ 24, 1 }));
    FrameRate fr = { 24, 1 };
    if (rate.is_array() and rate.size() == 2) fr = { rate[0].get<u32>(), rate[1].get<u32>() };

    open->DefaultCadence = u32(std::max(1, resolved.ConfigValue("DefaultCadence", 2).get<int>()));

    _Ctx.Timeline = Halftone::Animation::Timeline(fr, FrameRange { .Start = 1, .End = 120 });
    _Ctx.Open      = std::move(open);
    _Ctx.Mode      = EditorModeKind::Scene;
    _Ctx.Focus     = NullID;
    _Ctx.Selection = NullID;
    for (auto& nav : _Ctx.Navigators) nav = ViewNavigator {};

    Log::Info("Editor", "Opened scene '{}' ({} nodes, {:.3f} fps, cadence {})", item.DisplayPath(),
              _Ctx.Open->Document.Nodes().size(), fr.FramesPerSecond(), _Ctx.Open->DefaultCadence);
    SwitchTo(*_Editor);
    _Editor->ModeFor(EditorModeKind::Scene).OnEnter(_Ctx);
}

void Shell::EnterMode(EditorModeKind mode, ID focus)
{
    EditorMode& next = _Editor->ModeFor(mode);
    if (mode == EditorModeKind::Scene) focus = NullID;
    if (not next.Accepts(_Ctx, focus)) {
        Log::Warn("Editor", "{} mode needs a suitable element selected", EditorModeName(mode));
        return;
    }

    _Editor->ModeFor(_Ctx.Mode).OnExit(_Ctx);

    const bool sameFocus = focus == _Ctx.Focus;
    _Ctx.Mode  = mode;
    _Ctx.Focus = focus;
    if (focus != NullID) _Ctx.Selection = focus;

    // Re-frame the view when honing into a different element.
    if (not sameFocus) _Ctx.Navigator() = ViewNavigator {};
    next.OnEnter(_Ctx);
}
