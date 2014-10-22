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
	int shnamendx = 0;			// section header name table index 
	int shstrndx = 0;			// symbol name section index
	byte *p = NULL;				// pointer to file contents
	file->sec_h = NULL;
	page->mem = NULL;

	if((file->fp = fopen(filename, "rb")) == NULL) {
		fprintf(stderr, "error: no such file '%s'\n", filename);
		return 0;
	}

	byte header[sizeof(Elf64_header)];
	fread(header, 1, sizeof(Elf64_header), file->fp);
	if(memcmp(header, "\x7f""ELF", 4) != 0) {
		// check ELF magic code
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

	p = (byte *)malloc(fsize(file->fp));
	file->sec_h = (Elf64_sh *)malloc(sizeof(Elf64_sh) * file->header.e_shnum);
	if(file->sec_h == NULL || p == NULL) {
		fprintf(stderr, "error: out of memory\n");
		ret_code = 0;
		goto EL_ALLOC_ERROR;
	}
	
	// read section headers
	int i;
	fseek(file->fp, 0, SEEK_SET);
	fread(p, fsize(file->fp), sizeof(byte), file->fp);
	for(i = 0; i < file->header.e_shnum; i++) {
		byte *q = p + file->header.e_shoff + sizeof(Elf64_sh) * i;
		file->sec_h[i].sh_name		= mem_read32(q, 0);
		file->sec_h[i].sh_type		= mem_read32(q, 4);
		file->sec_h[i].sh_flags		= mem_read64(q, 8);
		file->sec_h[i].sh_addr		= mem_read64(q, 16);
		file->sec_h[i].sh_offset	= mem_read64(q, 24);
		file->sec_h[i].sh_size		= mem_read64(q, 32);
		file->sec_h[i].sh_link		= mem_read32(q, 40);
		file->sec_h[i].sh_info		= mem_read32(q, 44);
		file->sec_h[i].sh_addralign	= mem_read64(q, 48);
		file->sec_h[i].sh_entsize	= mem_read64(q, 56);

		if(file->sec_h[i].sh_type == SHT_STRTAB && file->sec_h[i].sh_type != file->header.e_shstrndx) {
			// symbol name table section
			shstrndx = file->sec_h[i].sh_type;
		} else if(file->sec_h[i].sh_type == SHT_RELA) {
			// relocation info section
			int rel_off = file->sec_h[i].sh_offset;
			int rel_n = file->sec_h[i].sh_size / sizeof(Elf64_Rela);
			int i;
			Elf64_Rel *rels = (Elf64_Rel *)(p + rel_off);
			for(i = 0; i < rel_n; i++) {
				printf("offset=0x%lx, info=0x%lx\n", rels[i].r_offset, rels[i].r_info);
			}
		}
	}

	// analyze section names
	shnamendx = file->header.e_shstrndx;
	page->size = SEGMENT_SIZE;
	page->mem = (void *)malloc(SEGMENT_SIZE);
	page->offset_addr = 0;
	if(page->mem == NULL) {
		fprintf(stderr, "error: out of memory\n");
		ret_code = 0;
		goto EL_ALLOC_ERROR;
	} else {
		// stack pointer
		cpu.gpr[1] = STACK_OFFSET;
		// next instruction pointer
		cpu.nip = TEXT_OFFSET;
	}
	byte *name_tab = p + file->sec_h[shnamendx].sh_offset;
	for(i = 0; i < file->header.e_shnum; i++) {
		const char *sec_name = (char *)(name_tab + file->sec_h[i].sh_name);
		dword sh_size = file->sec_h[i].sh_size;
		dword sh_offset = file->sec_h[i].sh_offset;

		printf("%8.*s\ttype=%d\toffset=0x%lx\tsize=0x%lx\n", 16, sec_name, file->sec_h[i].sh_type, sh_offset, file->sec_h[i].sh_size);

		if(strncmp(sec_name, ".text", 5) == 0) {
			// load text segment
			page->text_size = sh_size;
			memcpy(page->mem + TEXT_OFFSET, p + sh_offset, sh_size);
		} else if(strncmp(sec_name, ".data", 5) == 0) {
			// load data segment
			memcpy(page->mem + DATA_OFFSET, p + sh_offset, sh_size);
		}
	}

	goto EL_RETURN;

EL_ALLOC_ERROR:
	free(file->sec_h); file->sec_h = NULL;
	free(page->mem); page->mem = NULL;
	fclose(file->fp);

EL_RETURN:
	free(p); p = NULL;

	return ret_code;
}

