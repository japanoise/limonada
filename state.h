#include "buffer.h"
#include "slice.h"

/* Application state */

typedef struct {
	buffer **data;
	int len;
	int size;
} buflist;

typedef struct {
	buflist *buffers;
	int curbuf;
} limonada;

buflist *makeBuflist();

buflist *makeBuflistFromArgs(int argc, char *argv[]);

int appendBuffer(buflist *list, buffer *buf);

void killBufferInList(buflist *list, int which);

limonada *makeState(buflist *list);
