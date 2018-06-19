#include <stdlib.h>
#include <string.h>
#include "slice.h"

StrSlice *MakeSlice(char *str) {
	StrSlice *ret = malloc(sizeof(StrSlice));
	ret->String = str;
	ret->len = strlen(str);
	return ret;
}
