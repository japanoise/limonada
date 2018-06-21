#include "slice.h"

/* Application state */

typedef struct {
	StrSlice *name;
	char *filename;
	int zoom;
	int panx;
	int pany;
} buffer;

typedef struct {
	buffer **data;
	int len;
	int size;
} buflist;

typedef struct {
	buflist *buffers;
	int curbuf;
} limonada;

buffer *makeBuffer(char *name);

buffer *makeBufferFromFile(char *filename);

void setBufferFileName(char *filename, buffer *buf);

buflist *makeBuflist();

buflist *makeBuflistFromArgs(int argc, char *argv[]);

void appendBuffer(buflist *list, buffer *buf);

void killBuffer(buffer *buf);

void killBufferInList(buflist *list, int which);

limonada *makeState(buflist *list);
