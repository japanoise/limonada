#include <stdlib.h>
#ifdef _WIN32
#include <string.h>
// Untested
char *basename(char *path) {
	int l = strlen(path);
	char *fn = malloc(l);
	char *ext = malloc(l);
	_splitpath(path, NULL, NULL, fn, ext);
	strcat(fn, ext);
	free(ext);
	return fn;
}
#else
#include <libgen.h>
#endif
#include "slice.h"
#include "state.h"

buffer *makeBuffer(char *name) {
	buffer *ret = malloc(sizeof(buffer));
	ret->name = MakeSlice(name);
	return ret;
}

buffer *makeBufferFromFile(char *filename) {
	char* bn = basename(filename);
	buffer *ret = makeBuffer(bn);
#ifdef _WIN32
	free(bn);
#endif
	return ret;
}

buflist *makeBuflist() {
	buflist *ret = malloc(sizeof(buflist));
	ret->len = 0;
	ret->size = 10;
	ret->data = malloc(sizeof(buffer*)*ret->size);
	return ret;
}

void appendBuffer(buflist *list, buffer *buf) {
	list->data[list->len] = buf;
	list->len++;
	if (list->len >= list->size - 3) {
		list->size *= 2;
		list->data = realloc(list->data, list->size*sizeof(buffer*));
	}
}

void killBuffer(buffer *buf) {
	DestroySlice(buf->name);
	free(buf);
}

void killBufferInList(buflist *list, int which){
	killBuffer(list->data[which]);
	if (which != list->len-1) {
		for (int i = which; i<list->len; i++) {
			list->data[i] = list->data[i+1];
		}
	}
	list->len--;
}

buflist *makeBuflistFromArgs(int argc, char *argv[]) {
	buflist *ret = makeBuflist();
	for (int i = 0; i < argc; i++) {
		appendBuffer(ret, makeBufferFromFile(argv[i]));
	}
	return ret;
}

limonada *makeState(buflist *list) {
	limonada *ret = malloc(sizeof(limonada));
	ret->buffers = list;
	if (list->len > 0) {
		ret->curbuf=0;
	}
	return ret;
}
