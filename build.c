#include <stdio.h>
#include <string.h>
#define CTL_IMPLEMENTATION
#include "ctl.h"

const char* program_name;

bool is_pack = false;
bool is_test_all = false;
bool is_test = false;
const char *test_script = NULL;
bool is_debug = true;
bool using_asan = false; // Will use ASan (Address Sanitizer)
bool using_ubsan = false; // Will use UBSan (Undefined Behavior Sanitizer)
bool using_tsan = false; // Will use TSan (Thread sanitizer)
// NOTE: Only ASan and TSan aren't compatible with each other
// But you can use UBSan with TSan or ASan

#ifndef POSIX_PREFIX
  // #define POSIX_PREFIX "aarch64-linux-gnu-"
  #define POSIX_PREFIX ""
#endif

#ifndef APPLE_PREFIX
  #define APPLE_PREFIX "aarch64-apple-darwin25.1-"
  // #define APPLE_PREFIX "x86_64-apple-darwin25.1-"
#endif

#ifndef MINGW_PREFIX
  // #define MINGW_PREFIX "aarch64-w64-mingw32-"
  #define MINGW_PREFIX "x86_64-w64-mingw32-"
#endif

#ifndef MSVC_PREFIX
  #define MSVC_PREFIX ""
#endif

#define TIMBER_H "timber.h"
#define TESTS_FOLDER "./tests/"
#define BUILD_FOLDER ".build/"
#define MAJOR_VER "1"
#define VERSION "1.0.0"

// POSIX
#define POSIX_CC POSIX_PREFIX "clang"
#define POSIX_AR POSIX_PREFIX "ar"
#define POSIX_STRIP POSIX_PREFIX "strip"
#define POSIX_RANLIB POSIX_PREFIX "ranlib"
#define POSIX_OBJECT BUILD_FOLDER"timber.o"
#define POSIX_STATIC_LIB BUILD_FOLDER"libtimber.a"
#define POSIX_DYNAMIC_LIB "libtimber.so"
#define POSIX_DYNAMIC_LIB_FULL POSIX_DYNAMIC_LIB"."VERSION
#define POSIX_DYNAMIC_LIB_MAJOR POSIX_DYNAMIC_LIB"."MAJOR_VER

bool build_obj_posix(CommandBuilder* cmd) {
  cmd_push(cmd, POSIX_CC);
  cmd_push(cmd, "-x", "c", "-c", "-Wall", "-Wextra", "-Wno-unused-function", "-DTIMBER_IMPLEMENTATION");
  cmd_push(cmd, "-fPIC", "-fvisibility=hidden", "-pthread", "-DTIMBER_SHARED", "-DTIMBER_BUILD");
  if (is_debug) {
    cmd_push(cmd, "-ggdb", "-O0", "-DTIMBER_DEBUG", "-fno-omit-frame-pointer");
    if (using_ubsan) cmd_push(cmd, "-fsanitize=undefined");
    if (using_asan) cmd_push(cmd, "-fsanitize=address");
    if (using_tsan) cmd_push(cmd, "-fsanitize=thread");
  } else cmd_push(cmd, "-O2");
  cmd_push(cmd, TIMBER_H);
  cmd_push(cmd, "-o", POSIX_OBJECT);
  return cmd_run(cmd);
}

bool build_dynlib_posix(CommandBuilder* cmd) {
  cmd_push(cmd, POSIX_CC);
  cmd_push(cmd, "-shared", "-pthread");
  cmd_push(cmd, "-Wl,-soname,"POSIX_DYNAMIC_LIB_FULL);
  cmd_push(cmd, POSIX_OBJECT);
  cmd_push(cmd, "-o", BUILD_FOLDER POSIX_DYNAMIC_LIB_FULL);
  if (!cmd_run(cmd)) return false;
  if (!cmd_insta_run(cmd, "ln", "-sf", POSIX_DYNAMIC_LIB_FULL,
                          BUILD_FOLDER POSIX_DYNAMIC_LIB_MAJOR)) return false;
  return cmd_insta_run(cmd, "ln", "-sf", POSIX_DYNAMIC_LIB_MAJOR,
                            BUILD_FOLDER POSIX_DYNAMIC_LIB);
}

bool build_stclib_posix(CommandBuilder* cmd) {
  cmd_push(cmd, POSIX_AR, "rcs");
  cmd_push(cmd, POSIX_STATIC_LIB);
  cmd_push(cmd, POSIX_OBJECT);
  if (!cmd_run(cmd)) return false;
  if (is_debug) return true;
  if (!cmd_insta_run(cmd, POSIX_STRIP, "-x", POSIX_STATIC_LIB)) return false;
  return cmd_insta_run(cmd, POSIX_RANLIB, POSIX_STATIC_LIB);
}

// Apple
#define APPLE_CC APPLE_PREFIX "clang"
#define APPLE_AR APPLE_PREFIX "ar"
#define APPLE_STRIP APPLE_PREFIX "strip"
#define APPLE_RANLIB APPLE_PREFIX "ranlib"
#define APPLE_OBJECT BUILD_FOLDER"timber.o"
#define APPLE_STATIC_LIB BUILD_FOLDER "libtimber.a"
#define APPLE_DYNAMIC_LIB "libtimber"
#define APPLE_DYNAMIC_LIB_FULL  APPLE_DYNAMIC_LIB"."VERSION".dylib"
#define APPLE_DYNAMIC_LIB_MAJOR APPLE_DYNAMIC_LIB"."MAJOR_VER".dylib"
#define APPLE_DYNAMIC_LIB_NOVER APPLE_DYNAMIC_LIB".dylib"

bool build_obj_apple(CommandBuilder* cmd) {
  cmd_push(cmd, APPLE_CC);
  cmd_push(cmd, "-x", "c", "-c", "-Wall", "-Wextra", "-Wno-unused-function", "-DTIMBER_IMPLEMENTATION");
  cmd_push(cmd, "-fvisibility=hidden", "-DTIMBER_SHARED", "-DTIMBER_BUILD", "-fno-omit-frame-pointer");
  if (is_debug) {
    cmd_push(cmd, "-g", "-O0", "-DTIMBER_DEBUG");
    if (using_ubsan) cmd_push(cmd, "-fsanitize=undefined");
    if (using_asan) cmd_push(cmd, "-fsanitize=address");
    if (using_tsan) cmd_push(cmd, "-fsanitize=thread");
  } else cmd_push(cmd, "-O2");
  cmd_push(cmd, TIMBER_H);
  cmd_push(cmd, "-o", APPLE_OBJECT);
  return cmd_run(cmd);
}

bool build_dynlib_apple(CommandBuilder* cmd) {
  cmd_push(cmd, APPLE_CC);
  cmd_push(cmd, "-dynamiclib");
  cmd_push(cmd, "-install_name", "@rpath/"APPLE_DYNAMIC_LIB_FULL);
  cmd_push(cmd, "-Wl,-compatibility_version,"MAJOR_VER".0.0");
  cmd_push(cmd, "-Wl,-current_version,"VERSION);
  cmd_push(cmd, APPLE_OBJECT);
  cmd_push(cmd, "-o", BUILD_FOLDER APPLE_DYNAMIC_LIB_FULL);
  if (!cmd_run(cmd)) return false;
  if (!cmd_insta_run(cmd, "ln", "-sf", APPLE_DYNAMIC_LIB_FULL, BUILD_FOLDER APPLE_DYNAMIC_LIB_MAJOR)) return false;
  return cmd_insta_run(cmd, "ln", "-sf", APPLE_DYNAMIC_LIB_MAJOR, BUILD_FOLDER APPLE_DYNAMIC_LIB".dylib");
}

bool build_stclib_apple(CommandBuilder* cmd) {
  cmd_push(cmd, APPLE_AR, "rcs");
  cmd_push(cmd, APPLE_STATIC_LIB);
  cmd_push(cmd, APPLE_OBJECT);
  if (!cmd_run(cmd)) return false;
  if (is_debug) return true;
  if (!cmd_insta_run(cmd, APPLE_STRIP, "-x", APPLE_STATIC_LIB)) return false;
  return cmd_insta_run(cmd, APPLE_RANLIB, APPLE_STATIC_LIB);
}

// MSVC
#define MSVC_CC MSVC_PREFIX "cl"
#define MSVC_AR MSVC_PREFIX "lib"
#define MSVC_LINKER MSVC_PREFIX "link"
#define MSVC_DYNAMIC_OBJ BUILD_FOLDER"timber_dyn.obj"
#define MSVC_STATIC_OBJ BUILD_FOLDER"timber_stc.obj"
#define MSVC_STATIC_LIB BUILD_FOLDER "libtimber_static.lib"
#define MSVC_IMPORT_LIB BUILD_FOLDER "libtimber.lib"
#define MSVC_DYNAMIC_LIB "libtimber"
#define MSVC_DYNAMIC_LIB_FULL  MSVC_DYNAMIC_LIB"-"VERSION".dll"
#define MSVC_DYNAMIC_LIB_MAJOR MSVC_DYNAMIC_LIB"-"MAJOR_VER".dll"
#define MSVC_DYNAMIC_LIB_NOVER MSVC_DYNAMIC_LIB".dll"

bool build_obj_msvc(CommandBuilder* cmd) {
  for (int i = 0; i < 2; i++) {
    bool is_dyn_obj = i == 0;
    cmd_push(cmd, MSVC_CC, "/nologo");
    cmd_push(cmd, "/TC", "/c", "/W4", "/D_CRT_SECURE_NO_WARNINGS", "/DTIMBER_IMPLEMENTATION");
    cmd_push(cmd, "/std:c11", "/experimental:c11atomics");
    if (is_debug) {
      cmd_push(cmd, "/Zi", "/DTIMBER_DEBUG", "/Od");
      if (is_dyn_obj) cmd_push(cmd, "/Fd"MSVC_DYNAMIC_OBJ".pdb");
      else cmd_push(cmd, "/Fd"MSVC_STATIC_OBJ".pdb");
    } else cmd_push(cmd, "/O2");
    if (is_dyn_obj) cmd_push(cmd, "/DTIMBER_BUILD", "/DTIMBER_SHARED");
    cmd_push(cmd, TIMBER_H);
    if (is_dyn_obj) cmd_push(cmd, "/Fo"MSVC_DYNAMIC_OBJ);
    else cmd_push(cmd, "/Fo"MSVC_STATIC_OBJ);
    if (!cmd_run(cmd)) return false;
  }
  return true;
}

bool build_dynlib_msvc(CommandBuilder* cmd) {
  cmd_push(cmd, MSVC_LINKER, "/nologo", "/DLL");
  if (is_debug) cmd_push(cmd, "/DEBUG");
  cmd_push(cmd, MSVC_DYNAMIC_OBJ);
  cmd_push(cmd, "/OUT:"BUILD_FOLDER MSVC_DYNAMIC_LIB_FULL, "/IMPLIB:"MSVC_IMPORT_LIB);
  return cmd_run(cmd);
}

bool build_stclib_msvc(CommandBuilder* cmd) {
  cmd_push(cmd, MSVC_AR, "/nologo");
  cmd_push(cmd, "/OUT:"MSVC_STATIC_LIB, MSVC_STATIC_OBJ);
  return cmd_run(cmd);
}

// MinGW
#define MINGW_CC MINGW_PREFIX "gcc"
#define MINGW_AR MINGW_PREFIX "ar"
#define MINGW_STRIP MINGW_PREFIX "strip"
#define MINGW_RANLIB MINGW_PREFIX "ranlib"

#define MINGW_STATIC_OBJ BUILD_FOLDER"timber_stc.o"
#define MINGW_STATIC_LIB BUILD_FOLDER "libtimber.a"
#define MINGW_DYNAMIC_OBJ BUILD_FOLDER"timber_dyn.o"
#define MINGW_IMPORT_LIB BUILD_FOLDER "libtimber.dll.a"
#define MINGW_DYNAMIC_LIB "libtimber"
#define MINGW_DYNAMIC_LIB_FULL  MINGW_DYNAMIC_LIB"-"VERSION".dll"
#define MINGW_DYNAMIC_LIB_MAJOR MINGW_DYNAMIC_LIB"-"MAJOR_VER".dll"
#define MINGW_DYNAMIC_LIB_NOVER MINGW_DYNAMIC_LIB".dll"

bool build_obj_mingw(CommandBuilder* cmd) {
  for (int i = 0; i < 2; i++) {
    bool is_dyn_obj = i == 0;
    cmd_push(cmd, MINGW_CC);
    cmd_push(cmd, "-x", "c", "-c", "-Wall", "-Wextra", "-Wno-unused-function", "-DTIMBER_IMPLEMENTATION");
    if (is_dyn_obj) cmd_push(cmd, "-DTIMBER_BUILD", "-DTIMBER_SHARED");
    if (is_debug) cmd_push(cmd, "-g", "-O0", "-DTIMBER_DEBUG", "-fno-omit-frame-pointer");
    else cmd_push(cmd, "-O2");
    cmd_push(cmd, TIMBER_H);
    if (is_dyn_obj) cmd_push(cmd, "-o", MINGW_DYNAMIC_OBJ);
    else cmd_push(cmd, "-o", MINGW_STATIC_OBJ);
    if (!cmd_run(cmd)) return false;
  }
  return true;
}

bool build_dynlib_mingw(CommandBuilder* cmd) {
  cmd_push(cmd, MINGW_CC);
  cmd_push(cmd, "-shared");
  cmd_push(cmd, "-Wl,--out-implib,"MINGW_IMPORT_LIB);
  cmd_push(cmd, MINGW_DYNAMIC_OBJ);
  cmd_push(cmd, "-o", BUILD_FOLDER MINGW_DYNAMIC_LIB_FULL);
  if (!cmd_run(cmd)) return false;
  if (!cmd_insta_run(cmd, "ln", "-sf", MINGW_DYNAMIC_LIB_FULL, BUILD_FOLDER MINGW_DYNAMIC_LIB_MAJOR)) return false;
  if (!cmd_insta_run(cmd, "ln", "-sf", MINGW_DYNAMIC_LIB_MAJOR, BUILD_FOLDER MINGW_DYNAMIC_LIB_NOVER)) return false;
  return true;
}

bool build_stclib_mingw(CommandBuilder* cmd) {
  cmd_push(cmd, MINGW_AR, "rcs");
  cmd_push(cmd, MINGW_STATIC_LIB);
  cmd_push(cmd, MINGW_STATIC_OBJ);
  if (!cmd_run(cmd)) return false;
  if (is_debug) return true;
  if (!cmd_insta_run(cmd, MINGW_STRIP, "-x", MINGW_STATIC_LIB)) return false;
  return cmd_insta_run(cmd, MINGW_RANLIB, MINGW_STATIC_LIB);
}

typedef enum {
  TP_NONE = 0,
  TP_POSIX = 1 << 0,
  TP_APPLE = 1 << 1,
  TP_MSVC = 1 << 2,
  TP_MINGW = 1 << 3,
  TP_ALL = TP_POSIX | TP_APPLE | TP_MSVC | TP_MINGW,
  #define _TargetPlatform_count 5
} TargetPlatform;

bool compile_test(CommandBuilder *cmd, const char *name) {
  char input_file[1024];
  snprintf(input_file, sizeof(input_file), TESTS_FOLDER"%s.c", name);

  char output_file[1024];
  snprintf(output_file, sizeof(output_file), TESTS_FOLDER BUILD_FOLDER"%s", name);

  // O0 (No sanitizers)
  if (!cmd_insta_run(cmd, "clang", "-ggdb", "-O0", input_file, "-o",
    temp_sprintf("%s_o0", output_file))) return false;

  // TSan + UBSan
  if (!cmd_insta_run(cmd, "clang", "-ggdb", "-O0", "-fsanitize=thread,undefined",
    input_file, "-o", temp_sprintf("%s_tsan", output_file))) return false;

  // ASan + UBSan
  if (!cmd_insta_run(cmd, "clang", "-ggdb", "-O0", "-fsanitize=address,undefined",
    input_file, "-o", temp_sprintf("%s_asan", output_file))) return false;

  // O3
  if (!cmd_insta_run(cmd, "clang", "-O3", input_file, "-DNDEBUG", "-DTIMBER_RELEASE",
    "-o", temp_sprintf("%s_o3", output_file))) return false;
  return true;
}

bool run_test(CommandBuilder *cmd, const char *name) {
  char binary[1024];
  snprintf(binary, sizeof(binary), TESTS_FOLDER BUILD_FOLDER"%s", name);
  if (!cmd_insta_run(cmd, temp_sprintf("%s_o3", binary))) return false;
  if (!cmd_insta_run(cmd, temp_sprintf("%s_o0", binary))) return false;
  if (!cmd_insta_run(cmd, temp_sprintf("%s_asan", binary))) return false;
  if (!cmd_insta_run(cmd, temp_sprintf("%s_tsan", binary))) return false;
  return true;
}

void print_usage(FILE* f) {
  if (!f) f = stderr;
  fprintf(f, "Usage:\n");
  fprintf(f, "  %s <...targets|help|test|pack> [...options]\n", program_name);
  fprintf(f, "  Available targets:\n");
  fprintf(f, "    posix: build using POSIX toolchain (Linux/BSD) (Compiler: "POSIX_CC", Archiver: "POSIX_AR")\n");
  fprintf(f, "    apple: build using Apple toolchain (MacOS) (Compiler: "APPLE_CC", Archiver: "APPLE_AR")\n");
  fprintf(f, "    msvc: build using MSVC toolchain (Windows) (Compiler: "MSVC_CC", Linker: "MSVC_LINKER", Archiver: "MSVC_AR")\n");
  fprintf(f, "    mingw: build using MinGW toolchain (Windows) (Compiler: "MINGW_CC", Archiver: "MINGW_AR")\n");
  fprintf(f, "    all: Build for all targets\n");
  fprintf(f, "  test <test_script>: Run a specific test\n");
  fprintf(f, "  pack: Run all build tools and pack them into zip/tarballs\n");
  fprintf(f, "  Available options:\n");
  fprintf(f, "    --release|-r|-R: optimize the code and don't add debug info to binaries\n");
  fprintf(f, "    --tsan: add TSan (thread sanitizer) to binaries (only for POSIX and APPLE)\n");
  fprintf(f, "    --asan: add ASan (address sanitizer) to binaries (only for POSIX and APPLE)\n");
  fprintf(f, "    --ubsan: add UBSan (undefined behavior sanitizer) to binaries (only for POSIX and APPLE)\n");
  fprintf(f, "    <test_script>: When providing test flag, name doesn't include .c like 10_basic\n");
}

bool run_tests(CommandBuilder *cmd) {
  if (!mkdir_if_not_exists(TESTS_FOLDER BUILD_FOLDER)) {
    fprintf(stderr, "ERROR: Couldn't create folder: "TESTS_FOLDER BUILD_FOLDER": %s\n", strerror(errno));
    return false;
  }

  if (is_test_all) {
    TODO("run_tests for all tests");
    return true;
  }

  if (!compile_test(cmd, test_script)) return false;
  if (!run_test(cmd, test_script)) return false;
  return true;
}

bool run_build(CommandBuilder *cmd, TargetPlatform targets) {
  if (!mkdir_if_not_exists(BUILD_FOLDER)) {
    fprintf(stderr, "ERROR: Couldn't create folder: "BUILD_FOLDER": %s\n", strerror(errno));
    return false;
  }

  if (targets & TP_POSIX) {
    printf("==> Building POSIX (Linux/BSD)\n");
    printf("    Compiler: %s\n", POSIX_CC);
    printf("    Archiver: %s\n", POSIX_AR);
    if (!build_obj_posix(cmd)) return false;
    if (!build_dynlib_posix(cmd)) return false;
    if (!build_stclib_posix(cmd)) return false;
    printf("==> Compilation finished for POSIX (Linux/BSD)\n");
    printf("    Dynamic library: %s\n", BUILD_FOLDER POSIX_DYNAMIC_LIB);
    printf("    Static library: %s\n", POSIX_STATIC_LIB);
  }

  if (targets & TP_APPLE) {
    printf("==> Building for Apple (MacOS)\n");
    printf("    Compiler: %s\n", APPLE_CC);
    printf("    Archiver: %s\n", APPLE_AR);
    if (!build_obj_apple(cmd)) return false;
    if (!build_dynlib_apple(cmd)) return false;
    if (!build_stclib_apple(cmd)) return false;
    printf("==> Compilation finished for Apple (MacOS)\n");
    printf("    Dynamic library: %s\n", BUILD_FOLDER APPLE_DYNAMIC_LIB);
    printf("    Static library: %s\n", APPLE_STATIC_LIB);
  }

  if (targets & TP_MINGW) {
    printf("==> Building for MinGW (Windows)\n");
    printf("    Compiler: %s\n", MINGW_CC);
    printf("    Archiver: %s\n", MINGW_AR);
    if (!build_obj_mingw(cmd)) return false;
    if (!build_dynlib_mingw(cmd)) return false;
    if (!build_stclib_mingw(cmd)) return false;
    printf("==> Compilation finished for MinGW (Windows)\n");
    printf("    Dynamic library: %s\n", BUILD_FOLDER MINGW_DYNAMIC_LIB);
    printf("    Import library: %s\n", MINGW_IMPORT_LIB);
    printf("    Static library: %s\n", MINGW_STATIC_LIB);
  }

  if (targets & TP_MSVC) {
    printf("==> Building for MSVC (Windows)\n");
    printf("    Compiler: %s\n", MSVC_CC);
    printf("    Linker: %s\n", MSVC_LINKER);
    printf("    Archiver: %s\n", MSVC_AR);
    if (!build_obj_msvc(cmd)) return false;
    if (!build_dynlib_msvc(cmd)) return false;
    if (!build_stclib_msvc(cmd)) return false;
    printf("==> Compilation finished for MSVC (Windows)\n");
    printf("    Dynamic library: %s\n", BUILD_FOLDER MSVC_DYNAMIC_LIB);
    printf("    Import library: %s\n", MSVC_IMPORT_LIB);
    printf("    Static library: %s\n", MSVC_STATIC_LIB);
  }
  return true;
}

int main(int argc, char** argv) {
  BUIC_REBUILD_URSELF(argc, argv);
  program_name = argv_shift(&argc, &argv);
  TargetPlatform targets = TP_NONE;

  for (const char* arg = argv_shift(&argc, &argv); arg; arg = argv_shift(&argc, &argv)) {
    if (strcmp(arg, "-r") == 0 ||
        strcmp(arg, "--release") == 0 ||
        strcmp(arg, "-R") == 0) is_debug = false;
    else if (strcmp(arg, "--asan") == 0) {
      if (using_tsan) {
        fprintf(stderr, "ERROR: You can't use ASan while using TSan\n");
        return 1;
      }
      using_asan = true;
    }
    else if (strcmp(arg, "--ubsan") == 0) using_ubsan = true;
    else if (strcmp(arg, "--tsan") == 0) {
      if (using_asan) {
        fprintf(stderr, "ERROR: You can't use TSan while using ASan\n");
        return 1;
      }
      using_tsan = true;
    }
    else if (strcmp(arg, "pack") == 0) {
      if (is_test) {
        fprintf(stderr, "ERROR: Incompatible flags: 'pack' and 'test'\n");
        return 1;
      }
      is_pack = true;
    }
    else if (strcmp(arg, "test") == 0) {
      if (is_pack) {
        fprintf(stderr, "ERROR: Incompatible flags: 'pack' and 'test'\n");
        return 1;
      }
      const char *test_name = argv_shift(&argc, &argv);
      if (!test_name) {
        fprintf(stderr, "ERROR: Provide test name like this: 10_basic\n");
        return 1;
      }
      if (strcmp(test_name, "all") == 0) is_test_all = true;
      test_script = test_name;
      is_test = true;
    }
    else if (strcmp(arg, "all") == 0) targets = TP_ALL;
    else if (strcmp(arg, "posix") == 0) targets |= TP_POSIX;
    else if (strcmp(arg, "apple") == 0) targets |= TP_APPLE;
    else if (strcmp(arg, "msvc") == 0) targets |= TP_MSVC;
    else if (strcmp(arg, "mingw") == 0) targets |= TP_MINGW;
    else if (strcmp(arg, "help") == 0) {
      print_usage(stdout);
      return 0;
    }
    else {
      fprintf(stderr, "ERROR: Unknown arg: '%s'\n", arg);
      print_usage(NULL);
      return 1;
    }
  }

  CommandBuilder cmd = {0};
  if (is_test) {
    if (!run_tests(&cmd)) return 1;
  } else {
    if (targets == TP_NONE) {
      fprintf(stderr, "ERROR: No target was specified\n");
      print_usage(NULL);
      return 1;
    }
    if (!run_build(&cmd, targets)) return 1;
  }

  cmd_free(&cmd);
  return 0;
}
