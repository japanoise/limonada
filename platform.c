#include "platform.h"
#ifdef _WIN32
#include <shlwapi.h>
#include <stdio.h>

char *basename(char *path) {
	int l = strlen(path);
	char *fn = malloc(l);
	char *ext = malloc(l);
	_splitpath(path, NULL, NULL, fn, ext);
	strcat(fn, ext);
	free(ext);
	return fn;
}

int fexist(char *path) {
	FILE *f = fopen(path, "r");
	if (f) {
		fclose(f);
		return 1;
	} else {
		return errno == ENOENT;
	}
}
#else
#include <libgen.h>
#include <unistd.h>

int fexist(char *path) {
	return access( path, F_OK ) != -1;
}
#endif
