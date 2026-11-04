"""
Shared C++20-module build graph for Halftone.

Reads the manifest in config.py, scans every source for its `module` and
`import` declarations, and produces an ordered list of compile steps. Used by:

  * SConstruct                  (incremental builds, the normal path)
  * build/do_build_modules      (standalone full build without SCons)
  * build/generate_compile_commands  (clangd / LSP database)

Clang module flow per module:
    Part.cppm   --precompile-->  <module>-<Part>.pcm  --codegen-->  <module>-<Part>.o
    module.cppm --precompile-->  <module>.pcm         --codegen-->  <module>.o
    Impl.cpp    ------------------- compile ------------------->   <Impl>.o
"""
import os
import re

from config import BuildConfig, TARGETS, PROGRAM

_MODULE_DECL = re.compile(r"^\s*(export\s+)?module\s+([A-Za-z_][\w.]*)?\s*(?::\s*([A-Za-z_]\w*))?\s*;", re.M)
_IMPORT_DECL = re.compile(r"^\s*(?:export\s+)?import\s+([A-Za-z_][\w.]*)?\s*(?::\s*([A-Za-z_]\w*))?\s*;", re.M)
_COMMENTS    = re.compile(r"//[^\n]*|/\*.*?\*/", re.S)


class BuildError(Exception):
    pass


class Unit:
    """One translation unit and the artifacts it produces."""
    INTERFACE      = "interface"       # export module X;  / export module X:P;
    IMPLEMENTATION = "implementation"  # module X;
    PLAIN          = "plain"           # no module declaration (Main.cpp)
    VENDOR         = "vendor"          # third-party, never scanned

    def __init__(self, src, kind, module=None, partition=None, imports=None, out_dir=None):
        self.src       = src
        self.kind      = kind
        self.module    = module
        self.partition = partition
        self.imports   = imports or []   # fully-qualified names, "Mod" or "Mod:Part"
        self.out_dir   = out_dir
        self.pcm       = None
        self.obj       = None

    @property
    def unit_name(self):
        return f"{self.module}:{self.partition}" if self.partition else self.module


def _rel(path):
    return os.path.normpath(path)


def _abs(path):
    return os.path.join(BuildConfig.PROJECT_DIR, path)


def scan(src):
    """Returns (kind, module, partition, imports) for a source file."""
    with open(_abs(src), encoding="utf-8", errors="replace") as f:
        text = _COMMENTS.sub("", f.read())

    kind, module, partition = Unit.PLAIN, None, None
    for m in _MODULE_DECL.finditer(text):
        if m.group(2) is None:        # `module;` (global module fragment) / `module :private;`
            continue
        kind      = Unit.INTERFACE if m.group(1) else Unit.IMPLEMENTATION
        module    = m.group(2)
        partition = m.group(3)
        break

    imports = []
    for m in _IMPORT_DECL.finditer(text):
        name, part = m.group(1), m.group(2)
        if name is None and part is None:
            continue
        if name is None:              # `import :Part;`
            if module is None:
                raise BuildError(f"{src}: partition import ':{part}' outside a module")
            imports.append(f"{module}:{part}")
        else:
            imports.append(f"{name}:{part}" if part else name)

    # A module implementation unit implicitly imports its primary interface.
    if kind == Unit.IMPLEMENTATION and partition is None:
        imports.append(module)
    return kind, module, partition, imports


def pcm_filename(module, partition):
    # Matches clang's -fprebuilt-module-path lookup convention.
    return f"{module}-{partition}.pcm" if partition else f"{module}.pcm"


def build_graph():
    """Scans all sources and returns units in a valid compile order."""
    units, by_name = [], {}

    for mod in TARGETS:
        for src in mod.interfaces:
            kind, name, part, imports = scan(src)
            if kind != Unit.INTERFACE:
                raise BuildError(f"{src}: listed as an interface but declares no `export module`")
            if name != mod.name:
                raise BuildError(f"{src}: declares module '{name}' but is listed under '{mod.name}'")
            u = Unit(_rel(src), kind, name, part, imports, _rel(mod.out_dir))
            u.pcm = os.path.join(u.out_dir, pcm_filename(name, part))
            u.obj = os.path.splitext(u.pcm)[0] + ".o"
            if u.unit_name in by_name:
                raise BuildError(f"{src}: '{u.unit_name}' is already declared by {by_name[u.unit_name].src}")
            by_name[u.unit_name] = u
            units.append(u)

        for src in mod.implementations:
            kind, name, part, imports = scan(src)
            if kind != Unit.IMPLEMENTATION:
                raise BuildError(f"{src}: listed as an implementation but declares no `module {mod.name};`")
            if name != mod.name:
                raise BuildError(f"{src}: implements '{name}' but is listed under '{mod.name}'")
            if part is not None:
                raise BuildError(
                    f"{src}: `module {name}:{part};` declares a *new* internal partition, it does not "
                    f"implement the interface partition :{part}. Use `module {name};` instead "
                    f"(it can see everything the module's partitions export).")
            u = Unit(_rel(src), kind, name, part, imports, _rel(mod.out_dir))
            stem = os.path.splitext(os.path.basename(src))[0]
            u.obj = os.path.join(u.out_dir, f"impl.{stem}.o")
            units.append(u)

    prog_dir = _rel(os.path.join("build", "modules", f"{PROGRAM.name}.module", "bin"))
    for src in PROGRAM.sources:
        kind, name, _, imports = scan(src)
        if kind != Unit.PLAIN:
            raise BuildError(f"{src}: program sources must not declare a module (found '{name}')")
        u = Unit(_rel(src), kind, imports=imports, out_dir=prog_dir)
        u.obj = os.path.join(prog_dir, os.path.splitext(os.path.basename(src))[0] + ".o")
        units.append(u)

    for src in PROGRAM.vendor_sources:
        # src/Lib/__Vendor/<Lib>/... -> build/modules/<Lib>.module/bin
        parts      = _rel(src).split(os.sep)
        lib        = parts[parts.index("__Vendor") + 1] if "__Vendor" in parts else "Vendor"
        vendor_dir = _rel(os.path.join("build", "modules", f"{lib}.module", "bin"))
        u = Unit(_rel(src), Unit.VENDOR, out_dir=vendor_dir)
        u.obj = os.path.join(vendor_dir, os.path.splitext(os.path.basename(src))[0] + ".o")
        units.append(u)

    # Resolve imports to the interface units that provide them.
    for u in units:
        u.deps = []
        for imp in u.imports:
            if imp.split(":")[0] in BuildConfig.EXTERNAL_MODULES:
                continue
            if imp not in by_name:
                raise BuildError(f"{u.src}: imports unknown module '{imp}' (add it to TARGETS in build/config.py)")
            u.deps.append(by_name[imp])

    return _toposort(units)


def _toposort(units):
    ordered, state = [], {}

    def visit(u, chain):
        s = state.get(id(u))
        if s == "done":
            return
        if s == "active":
            cycle = " -> ".join(x.src for x in chain + [u])
            raise BuildError(f"module import cycle: {cycle}")
        state[id(u)] = "active"
        for d in u.deps:
            visit(d, chain + [u])
        state[id(u)] = "done"
        ordered.append(u)

    for u in units:
        visit(u, [])
    return ordered


def module_path_flags(units):
    dirs = sorted({u.out_dir for u in units if u.kind == Unit.INTERFACE})
    return [f"-fprebuilt-module-path={d}" for d in dirs]


def commands(unit, units):
    """Returns a list of (target, [argv...]) compile steps for a unit."""
    cxx   = BuildConfig.TOOLCHAIN
    flags = BuildConfig.compile_flags()
    mpath = module_path_flags(units)

    if unit.kind == Unit.INTERFACE:
        return [
            (unit.pcm, [cxx] + flags + mpath + ["-x", "c++-module", "--precompile", unit.src, "-o", unit.pcm]),
            (unit.obj, [cxx] + flags + mpath + ["-Wno-unused-command-line-argument", "-c", unit.pcm, "-o", unit.obj]),
        ]
    if unit.kind in (Unit.IMPLEMENTATION, Unit.PLAIN):
        return [(unit.obj, [cxx] + flags + mpath + ["-c", unit.src, "-o", unit.obj])]
    # Vendor code: same flags minus project warnings-as-errors.
    vflags = [f for f in flags if not f.startswith("-Werror") and f != "-Wall"]
    return [(unit.obj, [cxx] + vflags + ["-c", unit.src, "-o", unit.obj])]


def executable_path():
    ext = ".exe" if BuildConfig.PLATFORM == "windows" else ""
    return os.path.join(_rel(PROGRAM.out_dir), PROGRAM.name + ext)


def link_response_file():
    """Path of the response file the link step reads its object list from."""
    return os.path.join(_rel(PROGRAM.out_dir), PROGRAM.name + ".link.rsp")


def write_link_response_file(units):
    """Writes every object path into the response file (one per line, quoted,
    forward slashes). Linking ~100 objects on one command line overflows the
    8191-character limit of cmd.exe on Windows; `clang++ @file` avoids it."""
    path = link_response_file()
    os.makedirs(os.path.join(BuildConfig.PROJECT_DIR, os.path.dirname(path)), exist_ok=True)
    lines = ['"' + u.obj.replace(os.sep, "/").replace('"', '\\"') + '"' for u in units]
    with open(os.path.join(BuildConfig.PROJECT_DIR, path), "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    return path


def link_command(units):
    """Link argv. Call write_link_response_file(units) before running it."""
    exe = executable_path()
    return exe, [BuildConfig.TOOLCHAIN, "@" + link_response_file(), "-o", exe] + BuildConfig.link_flags()
