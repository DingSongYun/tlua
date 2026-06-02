---
name: run-tlua
description: Build, run, test, and develop the tlua transpiler & interpreter (TypingLua). Use when asked to build tlua, run .tlua files, test tlua, or develop/debug the tlua C source.
---

# run-tlua

TypingLua (`tlua`) is a Lua 5.4 language extension that adds inline type annotations to `.tlua` files. Types are erased at transpile time — the output is standard Lua. The project is a C codebase (lexer + parser/transpiler + interpreter) built with CMake + MSVC + Ninja on Windows.

The driver script is `.claude/skills/run-tlua/smoke.sh`.

## Prerequisites

- **Visual Studio 2022/2026** with C/C++ workload (provides `cl.exe` via `vcvars64.bat`)
- **CMake** 3.15+ (in PATH)
- **Ninja** build system (in PATH, available via chocolatey)

## Build

```bash
cd tlua
.claude/skills/run-tlua/smoke.sh build
```

For debug builds with PDB symbols:

```bash
.claude/skills/run-tlua/smoke.sh build-debug
```

Build output lands in `build/`:
- `build/tlua.exe` — interpreter (transpile + execute in one step)
- `build/tluac.exe` — transpiler CLI (`.tlua` → `.lua`)
- `build/lua.exe` — standard Lua 5.4 interpreter
- `build/lua54.dll` — Lua shared library (for debugger hook injection)
- `build/Release/test_*.exe` — test executables

## Run (agent path)

All commands below assume `cd tlua` first.

### Run a .tlua file

```bash
.claude/skills/run-tlua/smoke.sh run examples/demo.tlua
```

### Evaluate inline tlua code

```bash
.claude/skills/run-tlua/smoke.sh eval "local x: number = 1 + 2; print(x)"
```

### Transpile .tlua to .lua (inspect output)

```bash
.claude/skills/run-tlua/smoke.sh transpile examples/demo.tlua -o /dev/null
# Or print to stdout:
build/tluac.exe -p examples/demo.tlua
```

### Run all tests

```bash
.claude/skills/run-tlua/smoke.sh test
```

### Run individual test suites

```bash
.claude/skills/run-tlua/smoke.sh test-lexer
.claude/skills/run-tlua/smoke.sh test-transpiler
.claude/skills/run-tlua/smoke.sh test-e2e
.claude/skills/run-tlua/smoke.sh test-runtime
```

### Full cycle (build + test + demo)

```bash
.claude/skills/run-tlua/smoke.sh all
```

## Direct invocation (for internal development)

When modifying the lexer/parser C code, you can call the executables directly after building:

```bash
# Interpreter
build/tlua.exe <file.tlua> [args...]
build/tlua.exe -e "<inline tlua code>"

# Transpiler
build/tluac.exe <file.tlua>          # output to <file>.lua
build/tluac.exe <file.tlua> -o out.lua
build/tluac.exe -p <file.tlua>       # print to stdout

# Tests (after build)
build/Release/test_lexer.exe
build/Release/test_transpiler.exe
build/Release/test_e2e.exe
build/Release/test_runtime.exe
```

## Project structure

```
tlua/
├── tlua/                       # Core C source
│   ├── tlua_lexer.c/.h        #   Lexer (tokenizer)
│   ├── tlua_parser.c/.h       #   Parser + transpiler (type erasure)
│   ├── tlua_main.c            #   tluac CLI entry
│   └── tlua_run.c             #   tlua interpreter entry (embeds Lua VM)
├── lua/                        # Lua 5.4.7 source (upstream, don't modify)
├── tests/
│   ├── tlua_test.h            #   Test framework macros
│   ├── test_lexer.c           #   Lexer unit tests (28 tests)
│   ├── test_transpiler.c      #   Transpiler unit tests (42 tests)
│   ├── test_e2e.c             #   E2E: tluac → lua pipeline (7 tests)
│   ├── test_runtime.c         #   Runtime: tlua.exe on fixtures (6 tests)
│   └── fixtures/              #   .tlua + .expected.lua + .expected.out
├── examples/                   # Demo .tlua files
├── docs/
│   ├── tlua-reference.md      #   Full type syntax reference
│   └── tlua-syntax.ebnf       #   Formal grammar
├── CMakeLists.txt              #   Build config (all targets)
└── .claude/skills/run-tlua/    #   This skill + driver
```

## Test fixtures workflow

When adding new syntax/features:

1. Create `tests/fixtures/<name>.tlua` with typed source
2. Create `tests/fixtures/<name>.expected.lua` with expected transpiled output
3. Create `tests/fixtures/<name>.expected.out` with expected runtime output
4. The e2e tests verify tluac output matches `.expected.lua`
5. The runtime tests verify `tlua.exe` output matches `.expected.out`

## Gotchas

- **MSVC C4819 warnings** are harmless — they occur because source files contain Chinese comments and the system codepage is GBK. They don't affect compilation.
- **Test executables are in `build/Release/`** (not `build/`), while main executables (`tlua.exe`, `tluac.exe`, `lua.exe`) are directly in `build/`. This is due to CMake's `CMAKE_RUNTIME_OUTPUT_DIRECTORY` setting.
- **vcvars64.bat must be called before cmake** — running cmake from a plain shell fails with "No CMAKE_C_COMPILER." The smoke.sh script handles this automatically by writing a temp .bat file.
- **Stale cmake cache**: if you switch generators (e.g. from VS to Ninja), delete `build/` first. The smoke.sh `clean` command does this.
- **Line count preservation**: the transpiler is designed to keep line numbers identical between `.tlua` and `.lua` output. This is critical for debugger mappings. Tests verify this.
- **`build/lua54.dll`** must be next to `tlua.exe` — the interpreter links dynamically for lua-debug compatibility.

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| `No CMAKE_C_COMPILER could be found` | Use `smoke.sh build` (auto-wraps with vcvars64) or call vcvars64.bat manually before cmake |
| `Does not match the generator used previously` | Run `smoke.sh clean` then `smoke.sh build` |
| `cmake: generator Visual Studio 17 2022 could not find any instance` | VS installation is at version 18 (VS 2026). Use `-G Ninja` (handled by smoke.sh) |
| Test fails with "cannot find tluac.exe" | Build first: `smoke.sh build` |
| `lua54.dll not found` when running tlua.exe | Run from the `tlua/` project root, or copy lua54.dll next to tlua.exe |
