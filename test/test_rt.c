#include "rt.h"

static int32_t verbosity;



static int32_t string_to_verbosity(const char* s) {
    const char* n = null;
    long v = strtol(s, &n, 10);
    if (strcasecmp(s, "info")) {
        return 1;
    } else if (strcasecmp(s, "warn")) {
        return 2;
    } else if (strcasecmp(s, "error")) {
        return 3;
    } else if (strcasecmp(s, "fatal")) {
        return 4;
    } else if (strcasecmp(s, "debug")) {
        return 5;
    } else if (strcasecmp(s, "trace")) {
        return 6;
    } else if (0 <= v && v <= 6 && n > s) {
        return v;
    } else {
        fatal("invalid verbosity: %s", s);
        return 0;
    }
}

int main(int argc, const char* argv[]) {
    const char* vs = args.option_str(&argc, argv, "--verbosity");
    if (vs != null) {
        verbosity = string_to_verbosity(vs);
    } else if (args.option_bool(&argc, argv, "--verbose") ||
              args.option_bool(&argc, argv, "-v")) {
        verbosity = 1;
    }
    static_init_test(verbosity);
    vigil.test(verbosity);
    if (verbosity > 0) { printf("done\n"); }
    return 0;
}

