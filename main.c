#include <stdio.h>
#include <stdlib.h>
#include "main.h"

long fsize(FILE *fp) {
	long pos = ftell(fp);
	long size;
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, pos);
	return size;
}

int main(int argc, char *argv[]) {
	if(argc != 2) {
		fprintf(stderr, "usage: %s [filename]\n", argv[0]);
		return EXIT_FAILURE;
	}

	FILE *fp = NULL;
	if((fp = fopen(argv[1], "rb")) == NULL) {
		fprintf(stderr, "error: no such file '%s'\n", argv[1]);
		return EXIT_FAILURE;
	}

	long size = fsize(fp);
	printf("File size = %ld\n", size);

	fclose(fp);

	return EXIT_SUCCESS;
}
