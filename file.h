#ifndef __FILE_H__
#define __FILE_H__

#include <stdio.h>
#include "main.h"

int fexists(const char *filename);
long fsize(FILE *fp);
int elf_loadfile(Exefile *file, const char *filename, struct Storage *page);
void elf_show_rel(Exefile *file, int n);
int elf_rel_index(Exefile *file, dword addr);
dword elf_relocate(Exefile *file, int n);

#endif // __FILE_H__
