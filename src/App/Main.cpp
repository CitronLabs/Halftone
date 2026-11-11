// Halftone - application entry point / composition root.
//
// This file only wires modules together; it should stay small. Anything with
// behaviour belongs in a module under lib/ + src/Lib/.
//
//   Bindings  (SDL window, Vulkan device, ImGui)      platform
//   Project   (library of projects, scenes, resources) what's on disk
//   Graphics  (EngineRegistry, Renderer)               engine-agnostic rendering
//   Chroma    (+ future engine modules)                pluggable NPR renderers
//   Scene / Drawing / Animation                        the open document
//   Editor    (Shell: Splash -> Browser -> Scene Editor)
#include <chrono>
#include <cstdlib>
#include <string>
#include <vector>
#include <memory>
#include <functional>

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.App;
import com.citronlabs.Halftone.Bindings;
import com.citronlabs.Halftone.Project;
import com.citronlabs.Halftone.Scene;
import com.citronlabs.Halftone.Animation;
import com.citronlabs.Halftone.Graphics;
import com.citronlabs.Halftone.Chroma;
import com.citronlabs.Halftone.Editor;

using namespace Halftone;
using namespace Halftone::Core;

namespace
{
    // Register every render engine compiled into this build. New engines:
    // add a Module to build/config.py, import it above and register it here.
    void RegisterEngines(Graphics::EngineRegistry& registry)
    {
        registry.Register<Chroma::Engine>(Chroma::Engine::EngineName);
    }

    // Register interactive drawing tools (one per Drawing::DrawingMode).
    // None exist yet - this is where the brush engine plugs in, e.g.
    //   ctx.DrawingTools.push_back(MakeUnique<Brushes::RasterBrushTool>());
    void RegisterDrawingTools(Editor::Context& /*ctx*/) {}

    // Optional: HALFTONE_MAX_FRAMES=<n> quits after n frames (smoke tests / CI).
    u64 MaxFramesFromEnv()
    {
        const char* value = std::getenv("HALFTONE_MAX_FRAMES");
        return value ? std::stoull(value) : 0;
    }
}

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;

    Log::Info("App", "{} {} starting", App::Info::Name, App::Info::Version);

    auto& sdl    = Bindings::SDL::GetBinding();
    auto& vulkan = Bindings::Vulkan::GetBinding();
    auto& imgui  = Bindings::ImGUI::GetBinding();
    auto& prefs  = App::Preferences::Get();

    // --- Platform (needed before the splash can draw) --------------------------
    if (not sdl.Initialize({ .Title = App::Info::Name, .Width = 1600, .Height = 900 })) return 1;

    const String prefDir = sdl.PreferencesDirectory();
    prefs.Load(prefDir + "Preferences.json");

    if (not vulkan.Initialize({ .VSync = prefs.Value("/Window/VSync", true) })) return 1;
    if (not imgui.Initialize(prefs.Value("/Interface/Scale", 1.0f)))            return 1;

    // --- Long-lived services ----------------------------------------------------
    Graphics::EngineRegistry engines;
    Graphics::Renderer       renderer(engines);
    Project::ProjectLibrary  library;
    Animation::Timeline      timeline;

    Editor::EditorContext ctx {
        .Library  = library,
        .Renderer = renderer,
        .Timeline = timeline,
    };
    Editor::Shell shell(ctx);

    // --- Startup work, shown on the splash screen --------------------------------
    auto& splash = shell.Splash();
    splash.AddStep("Registering render engines", [&] { RegisterEngines(engines); return true; });
    splash.AddStep("Starting render engine", [&] {
        return renderer.Initialize(prefs.Value<String>("/Rendering/Engine", App::Info::Engine));
    });
    splash.AddStep("Loading drawing tools", [&] { RegisterDrawingTools(ctx); return true; });
    splash.AddStep("Scanning project library", [&] {
        return library.Open(prefs.Value<String>("/Projects/Library", prefDir + "Projects"));
    });

    // --- Main loop ------------------------------------------------------------------
    using Clock = std::chrono::steady_clock;
    auto       last      = Clock::now();
    const u64  maxFrames = MaxFramesFromEnv();
    u64        frames    = 0;

    while (sdl.PumpEvents() and not shell.QuitRequested())
    {
        auto now = Clock::now();
        f64  dt  = std::chrono::duration<f64>(now - last).count();
        last     = now;

        if (sdl.ConsumeResized()) vulkan.RequestSwapchainRebuild();

        // 1. Build UI (screens draw, panels edit the document, navigation applies)
        renderer.BeginFrame();
        imgui.NewFrame();
        shell.Frame(dt);
        imgui.EndFrame();

        // 2. Record GPU work
        Bindings::FrameContext frame;
        if (vulkan.BeginFrame(frame))
        {
            if (shell.ViewportActive() and renderer.IsInitialized())
                renderer.RenderViewport(frame, shell.View(), *shell.Document(), timeline.Current(), timeline.Seconds());

            vulkan.BeginSwapchainPass(frame, { 0.08f, 0.08f, 0.10f, 1.0f });
            imgui.Draw(frame.Cmd);
            vulkan.EndSwapchainPass(frame);

            vulkan.EndFrame(frame);
        }

        if (maxFrames and ++frames >= maxFrames) break;
    }

    // --- Shutdown (reverse order) -------------------------------------------------
    vulkan.WaitIdle();
    prefs.Save();
    renderer.Shutdown();
    imgui.Shutdown();
    vulkan.Shutdown();
    sdl.Shutdown();

    Log::Info("App", "Goodbye");
    return 0;
}
