#ifndef PLATFORM_H
#define PLATFORM_H

#ifndef _WIN32
#define PATHSEP "/"
#else
#define PATHSEP "\\"
#endif

char *basename(char *path);

int fexist(char *path);
#endif
