#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// https://pubs.opengroup.org/onlinepubs/009604599/basedefs/dirent.h.html

#define NAME_MAX 260

typedef struct DIR DIR;

struct dirent { char d_name[NAME_MAX]; };

DIR* opendir(const char *dirname);
struct dirent *readdir(DIR *dirp);
int closedir(DIR* dir);

#ifdef __cplusplus
} // extern "C"
#endif

