/* Golang style wrapper for strings */

#ifndef SLICE_H__
#define SLICE_H__

typedef struct {
	char *String;
	int len;
} StrSlice;

StrSlice *MakeSlice(char *str);

void DestroySlice(StrSlice * s);

#endif
