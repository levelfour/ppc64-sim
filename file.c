#include "file.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

int fexists(const char *filename) {
	struct stat st;
	return stat(filename, &st);
}

long fsize(FILE *fp) {
	long size;
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	return size;
}

int elf_loadfile(Exefile *file, const char *filename, struct Storage *page) {
	int ret_code = 1;
	byte *sh_p = NULL;		// section header pointer
	byte *name_tab = NULL;	// section name table
	file->sec_h = NULL;
	page->mem = NULL;

	if((file->fp = fopen(filename, "rb")) == NULL) {
		fprintf(stderr, "error: no such file '%s'\n", filename);
		return 0;
	}

	byte header[sizeof(struct Elf64_header)];
	fread(header, 1, sizeof(struct Elf64_header), file->fp);
	if(memcmp(header, "\x7f""ELF", 4) != 0) {
		fprintf(stderr, "error: invalid file type\n");
		ret_code = 0;
		goto EL_ALLOC_ERROR;
	}

	memcpy(file->header.e_idnet, header, 16);
	file->header.e_type		= mem_read16(header, 16);
	file->header.e_machine	= mem_read16(header, 18);
	file->header.e_version	= mem_read32(header, 20);
	file->header.e_entry	= mem_read64(header, 24);
	file->header.e_phoff	= mem_read64(header, 32);
	file->header.e_shoff	= mem_read64(header, 40);
	file->header.e_flags	= mem_read32(header, 48);
	file->header.e_ehsize	= mem_read16(header, 52);
	file->header.e_phentsize = mem_read16(header, 54);
	file->header.e_phnum	= mem_read16(header, 56);
	file->header.e_shentsize = mem_read16(header, 58);
	file->header.e_shnum	= mem_read16(header, 60);
	file->header.e_shstrndx	= mem_read16(header, 62);

	sh_p = (byte *)malloc(sizeof(struct Elf64_sh) * file->header.e_shnum);
	file->sec_h = (struct Elf64_sh*)malloc(sizeof(struct Elf64_sh) * file->header.e_shnum);
	if(file->sec_h == NULL || sh_p == NULL) {
		fprintf(stderr, "error: out of memory\n");
		ret_code = 0;
		goto EL_ALLOC_ERROR;
	}
	
	// read section headers
	int i;
	fseek(file->fp, file->header.e_shoff, SEEK_SET);
	fread(sh_p, sizeof(struct Elf64_sh) * file->header.e_shnum, sizeof(byte), file->fp);
	file->sec_name_tab_off = 0;
	for(i = 0; i < file->header.e_shnum; i++) {
		byte *p = sh_p + sizeof(struct Elf64_sh) * i;
		file->sec_h[i].sh_name		= mem_read32(p, 0);
		file->sec_h[i].sh_type		= mem_read32(p, 4);
		file->sec_h[i].sh_flags		= mem_read64(p, 8);
		file->sec_h[i].sh_addr		= mem_read64(p, 16);
		file->sec_h[i].sh_offset	= mem_read64(p, 24);
		file->sec_h[i].sh_size		= mem_read64(p, 32);
		file->sec_h[i].sh_link		= mem_read32(p, 40);
		file->sec_h[i].sh_info		= mem_read32(p, 44);
		file->sec_h[i].sh_addralign	= mem_read64(p, 48);
		file->sec_h[i].sh_entsize	= mem_read64(p, 56);
		if(file->sec_name_tab_off == 0 && file->sec_h[i].sh_type == SHT_STRTAB) {
			// section name table
			file->sec_name_tab_off = file->sec_h[i].sh_offset;
		}
	}

	// analyze section names
	name_tab = (byte *)malloc(file->header.e_shoff - file->sec_name_tab_off);
	page->size = SEGMENT_SIZE;
	page->mem = (void *)malloc(SEGMENT_SIZE);
	page->offset_addr = 0;
	if(name_tab == NULL || page->mem == NULL) {
		fprintf(stderr, "error: out of memory\n");
		ret_code = 0;
		goto EL_ALLOC_ERROR;
	} else {
		// stack pointer
		cpu.gpr[1] = STACK_OFFSET;
		// next instruction pointer
		cpu.nip = TEXT_OFFSET;
	}
	fseek(file->fp, file->sec_name_tab_off, SEEK_SET);
	fread(name_tab, file->header.e_shoff - file->sec_name_tab_off, 1, file->fp);
	for(i = 0; i < file->header.e_shnum; i++) {
		const char *sec_name = (char *)(name_tab + file->sec_h[i].sh_name);
		dword sh_size = file->sec_h[i].sh_size;
		dword sh_offset = file->sec_h[i].sh_offset;

		printf("%.*s %lx\n", 16, sec_name, sh_offset);

		if(strncmp(sec_name, ".text", 5) == 0) {
			// load text segment
			page->text_size = sh_size;
			fseek(file->fp, sh_offset, SEEK_SET);
			fread(page->mem + TEXT_OFFSET, sizeof(byte), sh_size, file->fp);
		} else if(strncmp(sec_name, ".data", 5) == 0) {
			// load data segment
			fseek(file->fp, sh_offset, SEEK_SET);
			fread(page->mem + DATA_OFFSET, sizeof(byte), sh_size, file->fp);
		}
	}

	goto EL_RETURN;

EL_ALLOC_ERROR:
	free(file->sec_h); file->sec_h = NULL;
	free(page->mem); page->mem = NULL;
	fclose(file->fp);

EL_RETURN:
	free(sh_p); sh_p = NULL;
	free(name_tab); name_tab = NULL;

	return ret_code;
}

