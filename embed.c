#include <stdio.h>
#include <errno.h>
#include <string.h>

extern int errno;

void usage(char **argv) {
	printf("usage: %s infile outfile\n", argv[0]);
}

int main (int argc, char **argv) {
	if (argc<3) {
		usage(argv);
		return 1;
	}

	FILE *infile = fopen(argv[1], "r");
	if (infile == NULL) {
		fprintf(stderr, "%s\n", strerror(errno));
		return 1;
	}

	FILE *outfile = fopen(argv[2], "w");
	if (outfile == NULL) {
		fclose(infile);
		fprintf(stderr, "%s\n", strerror(errno));
		return 1;
	}

	// sanitize the output filename (for use in variables)
	for(char *c = argv[1]; *c!='\0'; c++) {
		if ('a'<=*c&&*c<='z') {
			*c &= 0xDF; // upcase
		} else if ('A'<=*c&&*c<='Z') {
			// do nothing
		} else {
			*c = '_';
		}
	}

	// output bytes
	fprintf(outfile, "unsigned char* %s = {", argv[1]);
	int c;
	int datsize = 0;
	while ((c = fgetc(infile)) != EOF) {
		if(datsize!=0) fprintf(outfile, ",");
		fprintf(outfile, "%#x", c);
		datsize++;
	}
	fprintf(outfile, "};\n");
	fclose(infile); // no longer needed

	// output length
	fprintf(outfile, "int %s_LEN = %d;\n", argv[1], datsize);

	// clean up and exit
	fclose(outfile);
	return 0;
}
