#include <stdlib.h>
#include "buffer.h"
#include "slice.h"
#include "state.h"

buflist *makeBuflist() {
	buflist *ret = malloc(sizeof(buflist));
	ret->len = 0;
	ret->size = 10;
	ret->data = malloc(sizeof(buffer*)*ret->size);
	return ret;
}

int appendBuffer(buflist *list, buffer *buf) {
	list->data[list->len] = buf;
	list->len++;
	if (list->len >= list->size - 3) {
		list->size *= 2;
		list->data = realloc(list->data, list->size*sizeof(buffer*));
	}
	return list->len-1;
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
	ret->curbuf = -1;
	ret->buffers = list;
	if (list->len > 0) {
		ret->curbuf=0;
	}
	return ret;
}
