#ifndef __FILE_H__
#define __FILE_H__

#include <stdio.h>
#include "main.h"

int fexists(const char *filename);
long fsize(FILE *fp);
int elf_loadfile(Exefile *file, const char *filename, struct Storage *page);

#endif // __FILE_H__
