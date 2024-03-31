#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _MSC_FULL_VER
#define countof(a) _countof(a)
#define popen(c, m) _popen(c, m)
#define pclose(f) _pclose(f)
#endif

enum { max_command_output = 16 * 1024 };

int run_command(const char* command, char* output, size_t max_output) {
    FILE* f = popen(command, "r");
    int r = f != NULL ? 1 : -1;
    size_t total = 0;
    while (r > 0) {
        size_t seen = fread(output + total, 1, max_output - total, f);
        if (seen <= 0) {
            r = 0;
        } else if (total + seen + 1 >= max_output) {
            r = -1;
        } else {
            total += seen;
        }
    }
    if (f != NULL) { pclose(f); }
    if (total < max_output) {
        output[total] = 0;
    }
    return r;
}

int main(void) {
    printf("// Automatically generated by version.c (project prebuild).\n");
    printf("// DO NOT EDIT.\n");
    printf("\n");
    static char hash[max_command_output];
    strcpy(hash, "BADF00D");
    if (run_command("git rev-parse --short HEAD", hash, countof(hash)) != 0) {
        fprintf(stderr, "Failed to get git hash.\n");
        return 1;
    }
    if (hash[strlen(hash) - 1] == '\n') { hash[strlen(hash) - 1] = 0; }
    time_t t = time(NULL);
    struct tm* utc = gmtime(&t);
    static char tag[max_command_output];
    strcpy(tag, "C0DEFEED");
    if (run_command("git describe --tags HEAD 2>nul", tag, countof(tag)) != 0) {
        fprintf(stderr, "Failed to get git tag.\n");
        return 1;
    }
    if (tag[strlen(tag) - 1] == '\n') { tag[strlen(tag) - 1] = 0; }
    printf("#pragma once\n");
    printf("#define version_hash \"%s\"\n", hash);
    printf("#define version_tag \"%s\"\n", tag);
    printf("#define version_yy (%02d)\n", utc->tm_year % 100);
    printf("#define version_mm (%02d)\n", utc->tm_mon + 1);
    printf("#define version_dd (%02d)\n", utc->tm_mday);
    printf("#define version_hh (%02d)\n", utc->tm_hour);
    printf("#define version_str \"%02d.%02d.%02d.%02dUTC %s\"\n",
        utc->tm_year % 100, utc->tm_mon + 1, utc->tm_mday, utc->tm_hour, hash);
    printf("#define version_int32 (0x%02d%02d%02d%02d)\n",
        utc->tm_year % 100, utc->tm_mon + 1, utc->tm_mday, utc->tm_hour);
    printf("#define version_hash_int64 (0x%sLL)\n", hash);
    return 0;
}