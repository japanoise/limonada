#include <stdlib.h>
#include <string.h>
#include "slice.h"

StrSlice *MakeSlice(char *str)
{
	StrSlice *ret = malloc(sizeof(StrSlice));
	ret->len = strlen(str);
	ret->String = malloc(ret->len+1);
	strcpy(ret->String, str);
	return ret;
}

void DestroySlice(StrSlice * s)
{
	free(s->String);
	free(s);
}
