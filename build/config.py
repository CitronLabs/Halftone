"""
Halftone build manifest.

This file is the single source of truth for *what* gets built. The scripts next
to it (modules.py, generate_compile_commands, do_build_modules) and the root
SConstruct only decide *how*.

Adding a module
---------------
1. Create   lib/Halftone.<Name>/module.cppm          (primary interface)
            lib/Halftone.<Name>/<Partition>.cppm     (one per partition)
            src/Lib/<Name>/<Partition>.cpp           (implementation units)
2. Append a Module(...) entry to TARGETS below.

Ordering inside TARGETS does not matter: every source file is scanned for its
`module` / `import` declarations and the build graph is ordered from that.
"""
import os
import sys

# Project root is derived from this file's location, so the tree can live
# anywhere (it used to be hard-coded to D:\Animation\Software\Halftone).
PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def _host_platform() -> str:
    if sys.platform.startswith("win"):
        return "windows"
    if sys.platform == "darwin":
        return "macos"
    return "linux"


class Module:
    """Defines a standalone compilable workspace module."""
    def __init__(self, name, interfaces=None, implementations=None, out_dir=None):
        self.name            = name
        self.interfaces      = interfaces or []        # C++20 Module Interfaces (.cppm)
        self.implementations = implementations or []   # Implementation units (.cpp)
        self.out_dir         = out_dir or f"build/modules/Halftone.module/bin/{name.split('.', 2)[-1]}/"


class Program:
    """The final executable: plain translation units + vendor sources, linked
    against every Module object."""
    def __init__(self, name, sources, vendor_sources, out_dir):
        self.name           = name
        self.sources        = sources           # may `import` modules, but declare none
        self.vendor_sources = vendor_sources    # third-party .cpp, never import modules
        self.out_dir        = out_dir


class BuildConfig:
    PROJECT_DIR = PROJECT_DIR
    PLATFORM    = os.environ.get("HALFTONE_PLATFORM", _host_platform())
    TOOLCHAIN   = os.environ.get("CXX", "clang++")

    # "debug" or "release"; override with `scons variant=release`
    # or HALFTONE_VARIANT=release for the standalone scripts.
    VARIANT     = os.environ.get("HALFTONE_VARIANT", "debug")

    INCLUDE_DIRS = [
        "lib",
        "lib/__Vendor",
        "lib/__Vendor/Vulkan",
        "lib/__Vendor/ImGui",
        "lib/__Vendor/ImGui/Backends",
    ]

    DEFINES = [
        "VULKAN_HPP_NO_EXCEPTIONS",
        "VULKAN_HPP_NO_STRUCT_CONSTRUCTORS",
        "SDL_VIDEO_VULKAN",
    ]

    # Flags shared across all C++ translation units
    GLOBAL_FLAGS = [
        "-std=c++23",
        "-Wall",
        "-Wno-reserved-module-identifier",
        "-Werror=reorder-init-list",    # vk-hpp designated initialisers must follow declaration order
    ]

    VARIANT_FLAGS = {
        "debug":   ["-g", "-O0", "-DHALFTONE_DEBUG=1"],
        "release": ["-O2", "-DNDEBUG"],
    }

    # Per-platform compile flags, link search paths, libraries and runtime
    # files to copy next to the executable.
    PLATFORM_FLAGS = {
        "windows": ["--target=x86_64-w64-mingw32", "-fexperimental-library"],
        "linux":   [],
        "macos":   [],
    }

    LINK_FLAGS = {
        # -static bakes libc++/libunwind into Halftone.exe so only SDL3.dll and
        # the system vulkan-1.dll are needed at runtime.
        "windows": ["--target=x86_64-w64-mingw32", "-fexperimental-library", "-static"],
        "linux":   ["-Wl,-rpath,$ORIGIN"],
        "macos":   ["-Wl,-rpath,@executable_path"],
    }

    LIB_DIRS = {
        "windows": ["build/modules/SDL3.module/bin", "build/modules/Vulkan.module/bin"],
        # On Linux/macOS point HALFTONE_SDL3_DIR at an SDL3 install prefix if it
        # is not on the system search path.
        "linux":   [os.path.join(os.environ["HALFTONE_SDL3_DIR"], "lib")] if "HALFTONE_SDL3_DIR" in os.environ else [],
        "macos":   [os.path.join(os.environ["HALFTONE_SDL3_DIR"], "lib")] if "HALFTONE_SDL3_DIR" in os.environ else [],
    }

    LIBS = {
        "windows": ["SDL3", "vulkan-1"],
        "linux":   ["SDL3", "vulkan"],
        "macos":   ["SDL3", "vulkan"],
    }

    RUNTIME_FILES = {
        "windows": ["build/modules/SDL3.module/bin/SDL3.dll"],
        "linux":   [os.path.join(os.environ["HALFTONE_SDL3_DIR"], "lib", "libSDL3.so.0")] if "HALFTONE_SDL3_DIR" in os.environ else [],
        "macos":   [],
    }

    # Imports that are not produced by TARGETS (e.g. `import std;`) and should
    # not be treated as missing modules by the dependency scanner.
    EXTERNAL_MODULES = {"std", "std.compat"}

    # ------------------------------------------------------------------
    @classmethod
    def compile_flags(cls):
        flags  = list(cls.GLOBAL_FLAGS)
        flags += cls.VARIANT_FLAGS.get(cls.VARIANT, [])
        flags += cls.PLATFORM_FLAGS.get(cls.PLATFORM, [])
        flags += [f"-D{d}" for d in cls.DEFINES]
        flags += [f"-I{os.path.join(cls.PROJECT_DIR, d)}" for d in cls.INCLUDE_DIRS]
        return flags

    @classmethod
    def link_flags(cls):
        flags  = list(cls.LINK_FLAGS.get(cls.PLATFORM, []))
        flags += [f"-L{d if os.path.isabs(d) else os.path.join(cls.PROJECT_DIR, d)}"
                  for d in cls.LIB_DIRS.get(cls.PLATFORM, [])]
        flags += [f"-l{l}" for l in cls.LIBS.get(cls.PLATFORM, [])]
        return flags


# ------------------------------------------
# DECLARE YOUR PROJECT TARGETS HERE
# ------------------------------------------
# Dependency layering (lower rows never import higher rows):
#
#   App shell     src/App/Main.cpp  (composition root, not a module)
#   Editor        Halftone.Editor      (screens, modes, panels)
#   Engines       Halftone.Chroma
#   Graphics      Halftone.Graphics
#   Projects      Halftone.Project     (library tree, scoped resources, scene files)
#   Document      Halftone.Scene  ->  Halftone.Drawing  ->  Halftone.Animation
#   Platform      Halftone.Bindings
#   Metadata      Halftone.App
#   Foundation    Halftone.Core  (+ Halftone.Core.JSON)
#
TARGETS = [
    Module(
        name="com.citronlabs.Halftone.Core.JSON",
        interfaces=[
            "lib/Halftone.Core/JSON/module.cppm",
        ],
        out_dir="build/modules/Halftone.module/bin/Halftone.Core/",
    ),

    Module(
        name="com.citronlabs.Halftone.Core",
        interfaces=[
            "lib/Halftone.Core/Types.cppm",
            "lib/Halftone.Core/Math.cppm",
            "lib/Halftone.Core/Time.cppm",
            "lib/Halftone.Core/Log.cppm",
            "lib/Halftone.Core/module.cppm",
        ],
        implementations=[
            "src/Lib/Core/Log.cpp",
        ],
        out_dir="build/modules/Halftone.module/bin/Halftone.Core/",
    ),

    Module(
        name="com.citronlabs.Halftone.App",
        interfaces=[
            "lib/Halftone.App/Info.cppm",
            "lib/Halftone.App/Config.cppm",
            "lib/Halftone.App/module.cppm",
        ],
        implementations=[
            "src/App/Config/Preferences.cpp",
        ],
        out_dir="build/modules/Halftone.module/bin/Halftone.App/",
    ),

    Module(
        name="com.citronlabs.Halftone.Bindings",
        interfaces=[
            "lib/Halftone.Bindings/SDL.cppm",
            "lib/Halftone.Bindings/Vulkan.cppm",
            "lib/Halftone.Bindings/ImGUI.cppm",
            "lib/Halftone.Bindings/module.cppm",
        ],
        implementations=[
            "src/Lib/Bindings/SDL.cpp",
            "src/Lib/Bindings/Vulkan.cpp",
            "src/Lib/Bindings/ImGUI.cpp",
        ],
        out_dir="build/modules/Halftone.module/bin/Halftone.Bindings/",
    ),

    Module(
        name="com.citronlabs.Halftone.Scene",
        interfaces=[
            "lib/Halftone.Scene/Node.cppm",
            "lib/Halftone.Scene/Camera.cppm",
            "lib/Halftone.Scene/Light.cppm",
            "lib/Halftone.Scene/Canvas.cppm",
            "lib/Halftone.Scene/Model.cppm",
            "lib/Halftone.Scene/Scene.cppm",
            "lib/Halftone.Scene/Serialization.cppm",
            "lib/Halftone.Scene/module.cppm",
        ],
        implementations=[
            "src/Lib/Scene/Scene.cpp",
            "src/Lib/Scene/Serialization.cpp",
        ],
        out_dir="build/modules/Halftone.module/bin/Halftone.Scene/",
    ),

    Module(
        name="com.citronlabs.Halftone.Animation",
        interfaces=[
            "lib/Halftone.Animation/Timeline.cppm",
            "lib/Halftone.Animation/Cadence.cppm",
            "lib/Halftone.Animation/ExposureSheet.cppm",
            "lib/Halftone.Animation/Rig.cppm",
            "lib/Halftone.Animation/module.cppm",
        ],
        implementations=[
            "src/Lib/Animation/Timeline.cpp",
            "src/Lib/Animation/ExposureSheet.cpp",
        ],
        out_dir="build/modules/Halftone.module/bin/Halftone.Animation/",
    ),

    Module(
        name="com.citronlabs.Halftone.Project",
        interfaces=[
            "lib/Halftone.Project/Resources.cppm",
            "lib/Halftone.Project/ProjectItem.cppm",
            "lib/Halftone.Project/Library.cppm",
            "lib/Halftone.Project/module.cppm",
        ],
        implementations=[
            "src/Lib/Project/Resources.cpp",
            "src/Lib/Project/ProjectItem.cpp",
            "src/Lib/Project/Library.cpp",
        ],
        out_dir="build/modules/Halftone.module/bin/Halftone.Project/",
    ),

    Module(
        name="com.citronlabs.Halftone.Drawing",
        interfaces=[
            "lib/Halftone.Drawing/Stroke.cppm",
            "lib/Halftone.Drawing/Brush.cppm",
            "lib/Halftone.Drawing/Drawing.cppm",
            "lib/Halftone.Drawing/Layer.cppm",
            "lib/Halftone.Drawing/Canvas.cppm",
            "lib/Halftone.Drawing/Tool.cppm",
            "lib/Halftone.Drawing/module.cppm",
        ],
        implementations=[
            "src/Lib/Drawing/Canvas.cpp",
        ],
        out_dir="build/modules/Halftone.module/bin/Halftone.Drawing/",
    ),

    Module(
        name="com.citronlabs.Halftone.Graphics",
        interfaces=[
            "lib/Halftone.Graphics/RenderTarget.cppm",
            "lib/Halftone.Graphics/RenderEngine.cppm",
            "lib/Halftone.Graphics/EngineRegistry.cppm",
            "lib/Halftone.Graphics/Renderer.cppm",
            "lib/Halftone.Graphics/module.cppm",
        ],
        implementations=[
            "src/Lib/Graphics/RenderTarget.cpp",
            "src/Lib/Graphics/EngineRegistry.cpp",
            "src/Lib/Graphics/Renderer.cpp",
        ],
        out_dir="build/modules/Halftone.module/bin/Halftone.Graphics/",
    ),

    Module(
        name="com.citronlabs.Halftone.Chroma",
        interfaces=[
            "lib/Halftone.Chroma/module.cppm",
        ],
        implementations=[
            "src/Lib/Chroma/Engine.cpp",
        ],
        out_dir="build/modules/Halftone.module/bin/Halftone.Chroma/",
    ),

    Module(
        name="com.citronlabs.Halftone.Editor",
        interfaces=[
            "lib/Halftone.Editor/Context.cppm",
            "lib/Halftone.Editor/Panel.cppm",
            "lib/Halftone.Editor/Workspace.cppm",
            "lib/Halftone.Editor/ResourceEditor.cppm",
            "lib/Halftone.Editor/Mode.cppm",
            "lib/Halftone.Editor/Screen.cppm",
            "lib/Halftone.Editor/Panels/Viewport.cppm",
            "lib/Halftone.Editor/Panels/Timeline.cppm",
            "lib/Halftone.Editor/Panels/Outliner.cppm",
            "lib/Halftone.Editor/Panels/Properties.cppm",
            "lib/Halftone.Editor/Panels/Tools.cppm",
            "lib/Halftone.Editor/Panels/Layers.cppm",
            "lib/Halftone.Editor/Panels/Brush.cppm",
            "lib/Halftone.Editor/Panels/Material.cppm",
            "lib/Halftone.Editor/Modes/SceneLayout.cppm",
            "lib/Halftone.Editor/Modes/Canvas2D.cppm",
            "lib/Halftone.Editor/Modes/Canvas3D.cppm",
            "lib/Halftone.Editor/Modes/ModelEdit.cppm",
            "lib/Halftone.Editor/Screens/Splash.cppm",
            "lib/Halftone.Editor/Screens/ProjectBrowser.cppm",
            "lib/Halftone.Editor/Screens/SceneEditor.cppm",
            "lib/Halftone.Editor/Shell.cppm",
            "lib/Halftone.Editor/module.cppm",
        ],
        implementations=[
            "src/Lib/Editor/Context.cpp",
            "src/Lib/Editor/Workspace.cpp",
            "src/Lib/Editor/ResourceEditor.cpp",
            "src/Lib/Editor/Shell.cpp",
            "src/Lib/Editor/Panels/Viewport.cpp",
            "src/Lib/Editor/Panels/Timeline.cpp",
            "src/Lib/Editor/Panels/Outliner.cpp",
            "src/Lib/Editor/Panels/Properties.cpp",
            "src/Lib/Editor/Panels/Tools.cpp",
            "src/Lib/Editor/Panels/Layers.cpp",
            "src/Lib/Editor/Panels/Brush.cpp",
            "src/Lib/Editor/Panels/Material.cpp",
            "src/Lib/Editor/Modes/SceneLayout.cpp",
            "src/Lib/Editor/Modes/Canvas2D.cpp",
            "src/Lib/Editor/Modes/Canvas3D.cpp",
            "src/Lib/Editor/Modes/ModelEdit.cpp",
            "src/Lib/Editor/Screens/Splash.cpp",
            "src/Lib/Editor/Screens/ProjectBrowser.cpp",
            "src/Lib/Editor/Screens/SceneEditor.cpp",
        ],
        out_dir="build/modules/Halftone.module/bin/Halftone.Editor/",
    ),
]

PROGRAM = Program(
    name="Halftone",
    sources=[
        "src/App/Main.cpp",
    ],
    vendor_sources=[
        "src/Lib/__Vendor/ImGui/imgui.cpp",
        "src/Lib/__Vendor/ImGui/imgui_demo.cpp",
        "src/Lib/__Vendor/ImGui/imgui_draw.cpp",
        "src/Lib/__Vendor/ImGui/imgui_tables.cpp",
        "src/Lib/__Vendor/ImGui/imgui_widgets.cpp",
        "src/Lib/__Vendor/ImGui/Backends/imgui_impl_sdl3.cpp",
        "src/Lib/__Vendor/ImGui/Backends/imgui_impl_vulkan.cpp",
    ],
    out_dir="build/bin/",
)
