#!/usr/bin/env bash
# smoke.sh — Build, run, and test the tlua project (CLI-driven)
#
# Usage:
#   .claude/skills/run-tlua/smoke.sh [command]
#
# Commands:
#   build       Configure & build (Release) via MSVC + Ninja
#   build-debug Configure & build (Debug) with symbols/PDB
#   run <file>  Run a .tlua file with the interpreter
#   transpile <file> [-o out]  Transpile .tlua to .lua
#   eval <code> Execute inline tlua code
#   test        Run all test suites (lexer, transpiler, e2e, runtime)
#   test-lexer  Run lexer unit tests only
#   test-transpiler  Run transpiler unit tests only
#   test-e2e    Run end-to-end tests only
#   test-runtime Run runtime execution tests only
#   clean       Remove build directory
#   all         Build + test + run demo (default)
#
# Prerequisites:
#   - Visual Studio 2026 (cl.exe via vcvars64)
#   - CMake 3.15+
#   - Ninja build system
#
# All paths are relative to the tlua/ project root.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
cd "$PROJECT_ROOT"

# VS Developer environment — find vcvars64.bat
VCVARS=""
for dir in \
  "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Auxiliary/Build" \
  "C:/Program Files/Microsoft Visual Studio/18/Enterprise/VC/Auxiliary/Build" \
  "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build" \
  "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Auxiliary/Build" \
  "C:/Program Files (x86)/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build" \
  "C:/Program Files (x86)/Microsoft Visual Studio/2022/Enterprise/VC/Auxiliary/Build"; do
  if [[ -f "$dir/vcvars64.bat" ]]; then
    VCVARS="$dir/vcvars64.bat"
    break
  fi
done

if [[ -z "$VCVARS" ]]; then
  echo "ERROR: Cannot find vcvars64.bat. Install Visual Studio with C/C++ workload." >&2
  exit 1
fi

# Helper: run command inside MSVC developer environment
run_in_msvc() {
  local bat_content
  bat_content=$(cat <<BATEOF
@echo off
call "$VCVARS" >/dev/null 2>&1
cd /d $(cygpath -w "$PROJECT_ROOT")
$@
BATEOF
  )
  local tmpbat
  tmpbat="$PROJECT_ROOT/build/_run_cmd.bat"
  mkdir -p "$PROJECT_ROOT/build"
  echo "$bat_content" > "$tmpbat"
  cmd.exe //c "$(cygpath -w "$tmpbat")" 2>&1
  local rc=$?
  rm -f "$tmpbat"
  return $rc
}

do_build() {
  local build_type="${1:-Release}"
  echo "=== Building tlua ($build_type) ==="
  run_in_msvc "cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=$build_type && cmake --build build"
  echo ""
  echo "Executables:"
  ls -1 build/tlua.exe build/tluac.exe build/lua.exe build/luac.exe build/lua54.dll 2>/dev/null
  ls -1 build/Release/test_*.exe 2>/dev/null
  echo ""
  echo "=== Build complete ==="
}

do_run() {
  if [[ $# -lt 1 ]]; then
    echo "Usage: smoke.sh run <file.tlua> [args...]" >&2
    exit 1
  fi
  build/tlua.exe "$@"
}

do_transpile() {
  if [[ $# -lt 1 ]]; then
    echo "Usage: smoke.sh transpile <file.tlua> [-o output.lua]" >&2
    exit 1
  fi
  build/tluac.exe "$@"
}

do_eval() {
  if [[ $# -lt 1 ]]; then
    echo "Usage: smoke.sh eval '<tlua code>'" >&2
    exit 1
  fi
  build/tlua.exe -e "$1"
}

do_test() {
  local failed=0
  echo "=== Running all tests ==="
  echo ""

  echo "--- Lexer tests ---"
  build/Release/test_lexer.exe || failed=1
  echo ""

  echo "--- Transpiler tests ---"
  build/Release/test_transpiler.exe || failed=1
  echo ""

  echo "--- E2E tests ---"
  build/Release/test_e2e.exe || failed=1
  echo ""

  echo "--- Runtime tests ---"
  build/Release/test_runtime.exe || failed=1
  echo ""

  if [[ $failed -ne 0 ]]; then
    echo "=== SOME TESTS FAILED ==="
    exit 1
  fi
  echo "=== ALL TESTS PASSED ==="
}

do_clean() {
  echo "Removing build directory..."
  rm -rf build
  echo "Done."
}

# --- Main dispatch ---
cmd="${1:-all}"
shift || true

case "$cmd" in
  build)
    do_build Release
    ;;
  build-debug)
    do_build Debug
    ;;
  run)
    do_run "$@"
    ;;
  transpile)
    do_transpile "$@"
    ;;
  eval)
    do_eval "$@"
    ;;
  test)
    do_test
    ;;
  test-lexer)
    build/Release/test_lexer.exe
    ;;
  test-transpiler)
    build/Release/test_transpiler.exe
    ;;
  test-e2e)
    build/Release/test_e2e.exe
    ;;
  test-runtime)
    build/Release/test_runtime.exe
    ;;
  clean)
    do_clean
    ;;
  all)
    do_build Release
    echo ""
    do_test
    echo ""
    echo "--- Running demo ---"
    do_run examples/demo.tlua
    ;;
  *)
    echo "Unknown command: $cmd" >&2
    echo "Run with no args or 'all' for full build+test+demo." >&2
    exit 1
    ;;
esac
