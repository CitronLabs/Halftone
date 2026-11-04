# Halftone architecture

Halftone is built from C++23 named modules. Each area of the program is one
module with one partition per concept, and modules only depend on the modules
below them in the layer list.

## What the artist sees

```
Splash ──▶ Project Browser ──(Open Scene / double-click)──▶ Scene Editor
            tree of projects                                 ├─ Scene mode       empty 3D view: add canvases, models, lights, cameras
            ├─ Project      (folder)                         ├─ Canvas 2D mode   layers, blend modes, brushes, drawing modes, X-sheet
            │  ├─ Subproject (folder)                        ├─ Canvas 3D mode   the same layers placed in 3D (multiplane depth)
            │  │  └─ Scene   (.htscene file)                 └─ Model mode       stylised material + texture-paint layers
            │  └─ Scene
            └─ inspector: settings, Asset Packs, Build Systems, Macros, Configs
               (scoped: a scene sees its own + every parent's, inner configs override outer)
```

* Click a project, subproject or scene once to inspect it. Scenes also get an
  **Open Scene** button, and double-clicking a scene opens it.
* In the editor, double-click a canvas or model in the Outliner (or use the
  mode buttons at the top right) to work on it. **Esc** or **Scene** goes
  back to the whole scene.
* Viewport navigation:
  * Right-drag orbits (in Canvas 2D it pans).
  * Middle-drag or Shift+right-drag pans.
  * The mouse wheel zooms.
  * **F** frames the view.
  * **Ctrl+S** saves.

## Layers

```
 src/App/Main.cpp                       composition root (not a module)
 ─────────────────────────────────────────────────────────────────────────
 Halftone.Editor          Shell, Screens (Splash, ProjectBrowser, SceneEditor),
                          Modes (SceneLayout, Canvas2D, Canvas3D, ModelEdit), Panels
 Halftone.Chroma          default NPR render engine (implements Graphics::RenderEngine)
 Halftone.Graphics        RenderEngine + RenderView/ViewCamera, EngineRegistry, Renderer, RenderTarget
 Halftone.Project         ProjectLibrary (disk tree), ProjectItem, scoped Resources
 Halftone.Scene           Node, Camera, Light, Canvas, Model, Scene, Serialization
 Halftone.Drawing         Stroke, BrushPreset, Drawing, Layer, CanvasDocument, Tool
 Halftone.Animation       Timeline, Cadence, ExposureSheet, Rig
 Halftone.Bindings        SDL (window/events), Vulkan (device/swapchain/frames), ImGUI
 Halftone.App             Info, Preferences
 Halftone.Core            Types, Math, Time, Log  (+ Halftone.Core.JSON)
```

The document modules (Scene, Drawing, Animation) and Project contain no GPU
or UI code, so you can serialise, test or batch-process them without a
window.

## Where things go

| You are adding...                          | Interface                                     | Implementation                      |
|--------------------------------------------|-----------------------------------------------|-------------------------------------|
| a concept in an existing module            | `lib/Halftone.<Mod>/<Concept>.cppm`           | `src/Lib/<Mod>/<Concept>.cpp`       |
| a new module                               | `lib/Halftone.<Mod>/module.cppm` + partitions | `src/Lib/<Mod>/`                    |
| a render engine                            | `lib/Halftone.<EngineName>/module.cppm`       | `src/Lib/<EngineName>/`             |
| an editor panel                            | `lib/Halftone.Editor/Panels/<Name>.cppm`      | `src/Lib/Editor/Panels/<Name>.cpp`  |
| an editor mode (a new "hone into" view)    | `lib/Halftone.Editor/Modes/<Name>.cppm`       | `src/Lib/Editor/Modes/<Name>.cpp`   |
| a full-window screen                       | `lib/Halftone.Editor/Screens/<Name>.cppm`     | `src/Lib/Editor/Screens/<Name>.cpp` |
| a drawing tool (brush, vector pen, fill)   | implement `Drawing::Tool`                     | register in `RegisterDrawingTools()` (Main.cpp) |
| a startup task                             | `splash.AddStep(label, fn)` in Main.cpp       |                                     |
| a third-party library                      | headers in `lib/__Vendor/<Lib>/`, sources in `src/Lib/__Vendor/<Lib>/`, binaries in `build/modules/<Lib>.module/bin/` | |

After adding files, list them in `build/config.py` (`TARGETS`). Their order in
the list doesn't matter, because the build scans `import` declarations to
work out the dependency order.

## Projects on disk

```
<Library>/                        default: <user prefs dir>/Projects, override with /Projects/Library in Preferences.json
  Spring Short/
    project.htproj                { Kind: "Project", Name, Description, Resources: {...} }
    Lookdev.htscene               { Kind: "Scene", Name, Description, Resources: {...}, Scene: {...} }
    Sequence 01/
      project.htproj              { Kind: "Subproject", ... }
      Shot 010.htscene
```

* **Resources** have four kinds, each with an open JSON `Settings`/`Values`
  blob so features can add fields:
  * `AssetPacks`: named lists of asset paths.
  * `BuildSystems`: an exporter target plus its settings.
  * `Macros`: ordered command steps.
  * `Configs`: key/value pairs.
* `ProjectItem::Resolve()` collects an item's own resources plus everything
  its parents expose, outermost first. `ResolvedResources::ConfigValue(key)`
  applies the overrides, so the innermost scope wins.
* Two configs are already used when a scene opens:
  * `FrameRate` sets the timeline's frame rate.
  * `DefaultCadence` sets the cadence for new canvas layers. (The demo
    subproject sets threes, so every new layer in its scenes starts on
    threes.)
* `Scene::SceneToJson` and `SceneFromJson` store every node. Canvases are
  stored with their full layer stack (blend, opacity, depth transform,
  exposures, drawings and strokes), and models with their material and paint
  layers. Unknown keys are ignored and missing keys keep their defaults.

## Canvases and models

* **`Scene::Canvas`** is a 2D surface in the 3D scene.
  * **Role** (Character, Environment, Prop or Free) presets its size,
    facing, starting layers and lighting flags.
  * **Facing** handles billboarding, and **Anchor** gives it a pinned ground
    contact point, which guards against foreshortening and sliding.
  * **Mapping** chooses between a flat card and camera projection, together
    with `ProjectionCamera`.
  * Its contents are a **`Drawing::CanvasDocument`**: page size plus an
    ordered `Layer` stack.
* **`Drawing::Layer`** holds its kind (raster or vector), blend mode,
  opacity, visible and locked flags, and a **DepthTransform**, which is its
  placement in the canvas's own 3D space and what the Canvas 3D view edits.
  It also has an **ExposureSheet** (frame → drawing, with its own cadence)
  and the list of `Drawing`s.
* **`Scene::Model`** has a mesh asset path and a **Material** (base colour or
  texture, shadow tint, toon bands, outline) with `TexturePaintLayer`s.

## Frame flow

```
SDL::PumpEvents ─▶ Renderer::BeginFrame (apply viewport resize)
   ─▶ ImGUI::NewFrame ─▶ Shell::Frame
          ├─ Timeline::Update (scene editor only)
          ├─ current Screen Update + Draw  (panels edit EditorContext, request navigation)
          └─ apply Navigation  (open/close/save scene, enter mode)   ← never mid-draw
   ─▶ ImGUI::EndFrame
   ─▶ Vulkan::BeginFrame
        ─▶ Renderer::RenderViewport(RenderView) ─▶ RenderEngine::Render   (only when a scene is open)
        ─▶ Vulkan::BeginSwapchainPass ─▶ ImGUI::Draw ─▶ EndSwapchainPass
   ─▶ Vulkan::EndFrame (submit + present)
```

* The Viewport panel fills in `EditorContext::View`, a `Graphics::RenderView`
  made of a `ViewKind` (Scene, CanvasFlat, CanvasDepth or ModelFocus), the
  focus node, and the editor's `ViewCamera`. Each mode keeps its own
  `ViewNavigator`, so returning to Scene mode restores the view you had.
* **Engine output and editor overlays are separate.**
  * The engine draws the picture.
  * The Viewport panel draws editor gizmos on top with ImGui: ground grid,
    camera, light and model markers, canvas quads, layer planes, and the page
    frame. Engines never see these overlays.
  * In CanvasFlat both sides use `Graphics::CanvasPageRect()`, so the page
    the engine draws and the frame the editor draws always line up.
* Two frames are in flight. Swapchains rebuild on resize, and
  `Vulkan::DeferDestroy()` frees GPU objects only after the frame using them
  has retired.

## Render engines

To add a new engine, implement `Graphics::RenderEngine` in its own
`Halftone.<EngineName>` module, then register it in `RegisterEngines()` in
`src/App/Main.cpp`. It should handle every `ViewKind` (Chroma currently
draws the page in CanvasFlat and leaves the 3D passes as TODOs). It then
appears in the **Render** menu, and `/Rendering/Engine` in Preferences.json
can select it. `RenderEngine::DrawSettings()` puts the engine's controls in
the Properties panel.

## Module file conventions

```cpp
// lib/Halftone.Foo/Bar.cppm   (interface partition)
module;
#include <vector>                 // std / vendor headers go in the global module fragment
export module com.citronlabs.Halftone.Foo:Bar;

import com.citronlabs.Halftone.Core;
import :OtherPartition;

export namespace Halftone::Foo {
    using namespace Halftone::Core;
    class Bar { ... };
}
```

```cpp
// src/Lib/Foo/Bar.cpp   (implementation unit)
module;
#include <vector>
module com.citronlabs.Halftone.Foo;     // NOT `module ...Foo:Bar;` (the build rejects that)

import com.citronlabs.Halftone.Core;
using namespace Halftone::Foo;

void Bar::DoThing() { ... }
```

* Include the std headers you use in each file's global module fragment.
  Clang with libstdc++ won't find things like `std::move` or vector iterator
  operators through another module's includes.
* Methods and types are `PascalCase`, private members are `_PascalCase`, and
  getters take the member name without the underscore. vk-hpp designated
  initialisers must follow the order fields are declared in
  (`-Werror=reorder-init-list` enforces this).
* Watch for members whose names match types inside class scope. For
  example, `Layer& Layer;` doesn't compile, and inside `Shell` the name
  `OpenScene` means the member function, so the struct needs
  `Halftone::Editor::OpenScene`.
* Bindings are singletons reached through `GetBinding()`. They have explicit
  `Initialize()`/`Shutdown()` methods.

## Where features plug in (left open on purpose)

| Feature                         | Hook                                                                                  |
|---------------------------------|---------------------------------------------------------------------------------------|
| Raster brush engine / vector pen | implement `Drawing::Tool` for a `DrawingMode`, push into `ctx.DrawingTools`. The Viewport already forwards page-space pointer events to it |
| Pen pressure / tilt             | `SDL::AddEventHook` → fill `PointerEvent.Pressure/Tilt` (TODO in Viewport.cpp)          |
| Layer compositing in 2D         | Chroma `CanvasFlat` branch: draw `Layer::DrawingAt(frame)` with blend and opacity       |
| 3D passes (environment, canvases, models, lighting, line art, halftone) | Chroma `Render()` TODO list |
| Mesh import / texture painting  | `Model::MeshAsset`, `Material::PaintLayers[].Texture`; Model mode's **Import Mesh** menu item |
| Undo / redo and macros          | **Edit** menu placeholders; a command history in Halftone.Editor would also make `Macro::Steps` replayable |
| Build systems (export)          | `BuildSystem::Target` + `Settings`; add exporters and a **Build** action in the browser |
| Gizmo transforms                | `SceneTool::Move/Rotate/Scale` are selectable; drag handling goes in the Viewport overlay |
| Picking in the viewport         | the Viewport overlay already projects every node; add hit-testing there               |
| Docking UI                      | switch the vendored ImGui to the docking branch; panels stay unchanged                |
| GPU memory                      | `RenderTarget` uses plain `vkAllocateMemory`; move to VMA or a pool before adding raster tiles |
