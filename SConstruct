import os
import sys

# 1. Initialize Environment
# Note: Since image_674d47.png shows MinGW-style library extensions (.dll.a),
# we explicitly configure the tools for MinGW if you are on Windows.

system_env = os.environ.copy()

env = Environment(
    tools=['clang', 'clang++', 'gnulink', 'ar'],  # Forces SCons to use MinGW/GCC
    ENV=system_env,
    CPPPATH=[
        '#lib',                             # For SoundScaper.pkg.h
        '#lib/__Vendor',                    # For <SDL3/SDL.h> if structured as lib/__Vendor/SDL3
        '#lib/__Vendor/SDL3',               # Alternative SDL3 include path
        '#lib/__Vendor/ImGui',              # ImGui core headers
        '#lib/__Vendor/ImGui/Backends',     # ImGui backend headers
        '#lib/__Vendor/Vulkan',             # Vulkan headers
    ],
    CCFLAGS=['-std=c++23', '-O2', '-Wall']
)

# 2. Collect Source Files (Sourced from src/ as seen in image_674d47.png)
app_sources = Glob('src/App/*.cpp')

imgui_sources = [
    'src/Lib/__Vendor/ImGui/imgui.cpp',
    'src/Lib/__Vendor/ImGui/imgui_demo.cpp',
    'src/Lib/__Vendor/ImGui/imgui_draw.cpp',
    'src/Lib/__Vendor/ImGui/imgui_tables.cpp',
    'src/Lib/__Vendor/ImGui/imgui_widgets.cpp',
    'src/Lib/__Vendor/ImGui/Backends/imgui_impl_sdl3.cpp',
    'src/Lib/__Vendor/ImGui/Backends/imgui_impl_vulkan.cpp'
]

all_sources = app_sources + imgui_sources

# 3. Configure Local Linker Paths for SDL3
env.Append(LIBPATH=[
    '#src/Lib/__Vendor/SDL3', 
    '#src/Lib/__Vendor/Vulkan'          
])
env.Append(LIBS=['SDL3', 'vulkan-1']) # Links to your vendored files

# 5. Build the Target directly into your existing bin directory
env.Program(target='build/bin/Halftone', source=all_sources)
