#!/usr/bin/env python3
# WINEPATH="/usr/x86_64-w64-mingw32/bin"
import os, sys, subprocess

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
TEST = "all"

program_name = sys.argv[0]

def usage():
  print(f"Usage: {program_name} [OPTIONS]")
  print("Options:")
  print("  --platform | -p <platform = linux>: Set compile target platform")
  print(f"    platform can be {PLATFORMS}")
  print("  --variant | -v <variant = O0>: Set target variant")
  print(f"    variant can be {VARIANTS}")
  print("  --test | -t <test = all>: Set a specific test or all of them to run")
  print(f"    test can be {list(TESTS.keys())}")
  print("  help: Print this help message")

def run_cmd(cmd, log=True, stdout=True, stderr=True):
  if log:
    print(f"Command: { ' '.join(cmd) }")
  try:
    stdout_param = None if stdout else subprocess.DEVNULL
    stderr_param = None if stderr else subprocess.DEVNULL
    subprocess.run(cmd, check=True, stdout=stdout_param, stderr=stderr_param)
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
    case "--test" | "-t":
      test = shift(argv)
      if test is not None:
        if (not test in TESTS.keys()) and (not test == "all"):
          print(f"ERROR: Invalid test: {test}")
          usage()
          exit(1)
        TEST = test
    case "--variant" | "-v":
      variant = shift(argv)
      if variant is not None:
        if not variant in VARIANTS:
          print(f"ERROR: Invalid variant: {variant}")
          exit(1)
          usage()
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
run_cmd(mkdir_cmd, log=False, stdout=False)
run_cmd(['bash', '../aux/generate.sh'], log=False, stdout=False, stderr=False)

def compile_test(test, variant, platform):
  opts = TESTS[test]
  std = opts.get("std")
  is_cxx = std.startswith("c++")
  ext = ".cpp" if is_cxx else ".c"

  compile_cmd = [
    f"{get_compiler(platform, is_cxx)}",
    "-o", f"build/{platform}/{VARIANT}/{test}",
    f"{test}{ext}",
    "-I../build/", "-g", "-DTIMBER_IMPLEMENTATION",
    f"-std={std}"
  ]
  compile_cmd.extend(get_flags_from_variant(VARIANT))
  if platform == "linux":
    compile_cmd.append("-D_POSIX_C_SOURCE=200809L")

  if is_cxx:
    compile_cmd.append("-I../bindings/c++")
  run_cmd(compile_cmd, log=False)

def run_test(test, emulator, variant, platform):
  cmd = []
  if emulator is not None:
    cmd.append(emulator)
  cmd.append(f"./build/{platform}/{variant}/{test}{'.exe' if emulator == 'wine' else ''}")
  run_cmd(cmd)
  print("")

if sys.stdout.isatty() and "NO_COLOR" not in os.environ:
  bash_rst = "\x1b[0m"
  bash_grn = "\x1b[0;32m"
  bash_ylw = "\x1b[0;33m"
else:
  bash_rst = ""
  bash_grn = ""
  bash_ylw = ""

if TEST == "all":
  print(f"{bash_ylw}Compiling...{bash_rst}", end='', flush=True)
  for test in TESTS.keys():
    compile_test(test, VARIANT, PLATFORM)
  print(f" [{bash_grn}OK{bash_rst}]")
  print(f"{bash_grn}Running all tests...{bash_rst}")
  for test in TESTS.keys():
    run_test(test, None if PLATFORM == "linux" else "wine", VARIANT, PLATFORM)
else:
  compile_test(TEST, VARIANT, PLATFORM)
  run_test(TEST, None if PLATFORM == "linux" else "wine", VARIANT, PLATFORM)
