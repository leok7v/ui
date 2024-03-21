#pragma once

// va_first() va_rest() va_count() magic:
// https://stackoverflow.com/questions/5588855/standard-alternative-to-gccs-va-args-trick
// because
//     #define println(fmt, ...) printf(fmt "\n", ## __VA_ARGS__)
// 1. Is not a C99/C11/C17 standard (though supported by cl.exe)
// 2. Microsoft has it's own syntax
// 3. Intelisense chokes on both
// see: https://learn.microsoft.com/en-us/cpp/preprocessor/variadic-macros?view=msvc-170

#define va_first(...) __va_first_helper(__VA_ARGS__, throwaway)
#define __va_first_helper(first, ...) first
#define va_rest(...) __va_rest_helper(va_count(__VA_ARGS__), __VA_ARGS__)
#define __va_rest_helper(qty, ...) __va_rest_helper2(qty, __VA_ARGS__)
#define __va_rest_helper2(qty, ...) __va_rest_helper_##qty(__VA_ARGS__)
#define __va_rest_helper_one(first)
#define __va_rest_helper_twoormore(first, ...) , __VA_ARGS__
#define va_count(...) \
    __va_select_21th(__VA_ARGS__, \
                twoormore, twoormore, twoormore, twoormore, twoormore, \
                twoormore, twoormore, twoormore, twoormore, twoormore, \
                twoormore, twoormore, twoormore, twoormore, twoormore, \
                twoormore, twoormore, twoormore, twoormore, one, throwaway)
#define __va_select_21th(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, \
    a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, \
    a21, ...) a21

/*
va_first() va_rest() usage:

    #define println(...) printf(va_first(__VA_ARGS__) "\n----\n" va_rest(__VA_ARGS__))

    int main(int argc, const char* argv[]) {
        println("Hello");
        println("Hello %s", "World");
        println("2-args %d %d", 1, 2);
        println("3-args %d %d %d", 1, 2, 3);
        println("19-args %d %d %d %d %d %d %d %d %d %d  %d %d %d %d %d %d %d %d %d",
            1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19);
    }

produces:

    Hello
    ----
    Hello World
    ----
    2-args 1 2
    ----
    3-args 1 2 3
    ----
    19-args 1 2 3 4 5 6 7 8 9 10  11 12 13 14 15 16 17 18 19
    ----
*/
