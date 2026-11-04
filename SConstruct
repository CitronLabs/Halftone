"""
Halftone - SCons entry point.

    scons                    build build/bin/Halftone[.exe] (debug)
    scons variant=release    optimised build
    scons run                build, then launch
    scons gen_lspinfo        regenerate compile_commands.json for clangd
    scons -c                 clean

What to build is declared in build/config.py; the module dependency graph is
derived automatically from each file's `module` / `import` declarations
(build/modules.py), so SCons rebuilds exactly the units affected by a change.
"""
import os
import sys

sys.path.insert(0, os.path.join(Dir('#').abspath, 'build'))

AddOption('--cxx', dest='cxx', type='string', action='store', help='C++ compiler (default: clang++)')

variant = ARGUMENTS.get('variant', os.environ.get('HALFTONE_VARIANT', 'debug'))
os.environ['HALFTONE_VARIANT'] = variant
if GetOption('cxx'):
    os.environ['CXX'] = GetOption('cxx')

import config
import modules
from SCons.Scanner.C import CScanner

cfg = config.BuildConfig

env = Environment(
    ENV     = os.environ.copy(),
    CPPPATH = [os.path.join(cfg.PROJECT_DIR, d) for d in cfg.INCLUDE_DIRS],  # used by the header scanner only
)

try:
    units = modules.build_graph()
except modules.BuildError as e:
    print(f"[Halftone] build graph error: {e}")
    Exit(1)

header_scanner = CScanner()

def esc(argv):
    # Actions go through SCons variable expansion and then the shell; flags
    # like -Wl,-rpath,$ORIGIN need a literal '$' at the end of both.
    shell_dollar = '$$' if sys.platform.startswith('win') else '\\$$'
    return [a.replace('$', shell_dollar) for a in argv]

# ---------------------------------------------------------------------------
# 1. One SCons Command per compile step, wired to the .pcm files it imports.
# ---------------------------------------------------------------------------
pcm_node = {}
objects  = []

for unit in units:
    steps = [(t, esc(a)) for t, a in modules.commands(unit, units)]
    for target, argv in steps:
        if target.endswith('.pcm'):
            node = env.Command(target, unit.src, [argv], source_scanner=header_scanner)
            pcm_node[unit.unit_name] = node
        elif target.endswith('.o') and unit.kind == modules.Unit.INTERFACE:
            node = env.Command(target, pcm_node[unit.unit_name], [argv])
            objects.append(node)
        else:
            node = env.Command(target, unit.src, [argv], source_scanner=header_scanner)
            objects.append(node)

        for dep in getattr(unit, 'deps', []):
            env.Depends(node, pcm_node[dep.unit_name])

# ---------------------------------------------------------------------------
# 2. Link + runtime files.
# ---------------------------------------------------------------------------
exe_path, link_argv = modules.link_command(units)

def write_link_rsp(target, source, env):
    modules.write_link_response_file(units)
    return 0

program = env.Command(exe_path, objects, [
    Action(write_link_rsp, 'Writing ' + modules.link_response_file()),
    esc(link_argv),
])

runtime = []
for f in cfg.RUNTIME_FILES.get(cfg.PLATFORM, []):
    src = f if os.path.isabs(f) else os.path.join(cfg.PROJECT_DIR, f)
    if os.path.exists(src):
        runtime += env.InstallAs(os.path.join(config.PROGRAM.out_dir, os.path.basename(f)), os.path.realpath(src))

Default(program, runtime)

# ---------------------------------------------------------------------------
# 3. Utility aliases (names kept from the original SConstruct).
# ---------------------------------------------------------------------------
gen_lsp_info_cmd = env.Command(
    target = 'compile_commands.json',
    source = ['build/config.py', 'build/modules.py'],
    action = [[sys.executable, os.path.join('build', 'generate_compile_commands')]],
)
env.AlwaysBuild(gen_lsp_info_cmd)
env.Alias('gen_lspinfo', gen_lsp_info_cmd)

env.Alias('gen_modules', [pcm_node[k] for k in pcm_node])

run_cmd = env.Command('run.phony', [program] + runtime, [[os.path.abspath(exe_path)]])
env.AlwaysBuild(run_cmd)
env.Alias('run', run_cmd)
