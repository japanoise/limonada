/* Golang style wrapper for strings */

typedef struct {
	char *String;
	int len;
} StrSlice;

StrSlice *MakeSlice(char *str);
