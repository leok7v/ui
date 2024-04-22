#pragma once
#include "ut/std.h"

begin_c

typedef struct {
    // On Unix it is responsibility of the main() to assign these values
    int32_t c;      // argc
    const char** v; // argv[argc]
    const char** env;
    void    (*main)(int32_t argc, const char* argv[], const char** env);
    void    (*WinMain)(const char* command_line); // windows specific
    int32_t (*option_index)(const char* option); // e.g. option: "--verbosity" or "-v"
    void    (*remove_at)(int32_t ix);
    /* argc=3 argv={"foo", "--verbose"} -> returns true; argc=1 argv={"foo"} */
    bool    (*option_bool)(const char* option);
    /* argc=3 argv={"foo", "--n", "153"} -> value==153, true; argc=1 argv={"foo"}
       also handles negative values (e.g. "-153") and hex (e.g. 0xBADF00D)
    */
    bool    (*option_int)(const char* option, int64_t *value);
    // for argc=3 argv={"foo", "--path", "bar"}
    //     option_str("--path", option)
    // returns option: "bar" and argc=1 argv={"foo"} */
    const char* (*option_str)(const char* option);
    // basename() for argc=3 argv={"/bin/foo.exe", ...} returns "foo":
    const char* (*basename)(void);
    // removes quotes from a head and tail of the string `s` if present
    const char* (*unquote)(char* *s); // modifies `s` in place
    void (*fini)(void);
    void (*test)(void);
} args_if;

extern args_if args;

/* Usage:

  (both main() and WinMain() could be compiled at the same time on Windows):

  static int run(void);

  int main(int argc, char* argv[], char* envp[]) { // link.exe /SUBSYSTEM:CONSOLE
      args.main(argc, argv, envp); // Initialize args with command-line parameters
      int r = run();
      args.fini(); // Clean-up
      return r;
  }

  int APIENTRY WinMain(HINSTANCE inst, HINSTANCE prev, char* cl, int show) {
      // link.exe /SUBSYSTEM:WINDOWS
      args.WinMain(cl); // Initialize args with command line string
      int r = run();
      args.fini(); // Clean-up
      return 0;
  }

  static int run(void) {
      if (args.option_bool("-v")) {
          debug.verbosity.level = debug.verbosity.verbose;
      }
      int64_t num = 0;
      if (args.option_int("--number", &num)) {
          printf("--number: %ld\n", num);
      }
      const char* path = args.option_str("--path");
      if (path != null) {
          printf("--path: %s\n", path);
      }
      return 0;
  }

*/

end_c
