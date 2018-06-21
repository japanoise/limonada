#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
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
	ret->filename = NULL;
	ret->panx = 0;
	ret->pany = 0;
	ret->zoom = 0;
	return ret;
}

buffer *makeBufferFromFile(char *filename) {
	char* bn = basename(filename);
	buffer *ret = makeBuffer(bn);
	setBufferFileName(filename, ret);
#ifdef _WIN32
	free(bn);
#endif
	return ret;
}

void setBufferFileName(char *filename, buffer* buf) {
	int l = strlen(filename);
	buf->filename = malloc(l);
	strcpy(buf->filename, filename);
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
	if (buf->filename != NULL) {
		free(buf->filename);
	}
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
