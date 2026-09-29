#!/usr/bin/env python3
# WINEPATH="/usr/x86_64-w64-mingw32/bin"
import subprocess
import sys

TESTS = {
  "10_basic": {
    "std": "c11",
  },
  "15_basic_mt": {
    "std": "c11",
  },
  "20_stress_test_st": {
    "std": "c11",
  },
  "25_stress_test_mt": {
    "std": "c11",
  },
  "30_cxx11": {
    "std": "c++11",
  },
  "35_cxx17": {
    "std": "c++17",
  },
  "37_cxx20": {
    "std": "c++20",
  },
  "40_overflow": {
    "std": "c11",
  },
  "50_empty": {
    "std": "c11",
  },
}

VARIANTS = ["O0", "O3", "ASAN", "TSAN"]
PLATFORMS = ["linux", "mingw"]
ARCHS = ["x86_64", "aarch64"]

SINGLE_HEADER = "../build/timber.h"

VARIANT = "O0"
PLATFORM = "linux"

program_name = sys.argv[0]

def usage():
  print(f"Usage: {program_name} [OPTIONS]")
  print("Options:")
  print("  --platform | -p <platform = linux>: Set compile target platform")
  print(f"    platform can be {PLATFORMS}")
  print("  --variant | -v <variant = O0>: Set target variant")
  print(f"    variant can be {VARIANTS}")
  print("  help: Print this help message")

def run_cmd(cmd):
  print(f"Command: { ' '.join(cmd) }")
  try:
    subprocess.run(cmd, check=True)
  except subprocess.CalledProcessError as e:
    print(f"ERROR: Command failed with {e.returncode}")

def shift(argv):
  if not argv:
    return None
  return argv.pop(0)

argv = sys.argv[1:]
arg = shift(argv)
while arg is not None:
  match arg:
    case "--variant" | "-v":
      variant = shift(argv)
      if variant is not None:
        if not variant in VARIANTS:
          print(f"ERROR: Invalid variant: {variant}")
          usage()
          exit(1)
        VARIANT = variant
    case "--platform" | "-p":
      platform = shift(argv)
      if platform is not None:
        if not platform in PLATFORMS:
          print(f"ERROR: Invalid platform: {platform}")
          usage()
          exit(1)
        PLATFORM = platform
    case "help":
      usage()
      exit(0)
  arg = shift(argv)

def get_compiler(platform, is_cxx):
  match platform:
    case "linux": return "clang++" if is_cxx else "clang"
    case "mingw": return "x86_64-w64-mingw32-g++" if is_cxx else "x86_64-w64-mingw32-gcc"
    case _: return "<unknown>"

def get_flags_from_variant(variant):
  match variant:
    case "O0": return ["-O0"]
    case "O3": return ["-O3"]
    case "ASAN":
      return ["-fno-omit-frame-pointer",
              "-fno-sanitize-recover=undefined",
              "-fsanitize=undefined,address"]
    case "TSAN": return ["-fsanitize=undefined,thread"]
    case _: return []

# Setup environment
mkdir_cmd = ["mkdir", "-p", f"build/{PLATFORM}/{VARIANT}"]
run_cmd(mkdir_cmd)
run_cmd(['bash', '../aux/generate.sh'])

# Compile all test files with specified variant and platform
def compile_test_all(variant, platform):
  for file_name, opts in TESTS.items():
    std = opts.get("std")
    is_cxx = std.startswith("c++")
    ext = ".cpp" if is_cxx else ".c"

    compile_cmd = [
      f"{get_compiler(platform, is_cxx)}",
      "-o", f"build/{platform}/{VARIANT}/{file_name}",
      f"{file_name}{ext}",
      "-I../build/", "-g", "-DTIMBER_IMPLEMENTATION",
      f"-std={std}"
    ]
    compile_cmd.extend(get_flags_from_variant(VARIANT))
    if platform == "linux":
      compile_cmd.append("-D_POSIX_C_SOURCE=200809L")

    if is_cxx:
      compile_cmd.append("-I../bindings/c++")
    run_cmd(compile_cmd)

def run_test_all(emulator, variant, platform):
  for file_name, opts in TESTS.items():
    cmd = []
    if emulator is not None:
      cmd.append(emulator)
    cmd.append(f"./build/{platform}/{variant}/{file_name}{'.exe' if emulator == 'wine' else ''}")
    run_cmd(cmd)
    print("")

compile_test_all(VARIANT, PLATFORM)
print("")
print("===== RUNNING =====")
print("")
run_test_all(None if PLATFORM == "linux" else "wine", VARIANT, PLATFORM)
