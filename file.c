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
	file->sec_name_i = 0;	// section header name table index 
	int shstrndx = 0;		// symbol name section index
	int shsymndx = 0;		// symbol table section index
	byte *p = NULL;			// pointer to file contents
	file->sec_h = NULL;
	file->rels = NULL;
	file->rel_n = 0;
	file->syms = NULL;
	file->sym_n = 0;
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

		if(file->sec_h[i].sh_type == SHT_STRTAB && i != file->header.e_shstrndx) {
			// symbol name table section
			shstrndx = i;
		} else if(file->sec_h[i].sh_type == SHT_SYMTAB) {
			shsymndx = i;
		} else if(file->sec_h[i].sh_type == SHT_RELA) {
			// relocation info section
			int rel_off = file->sec_h[i].sh_offset;
			int rel_n = file->sec_h[i].sh_size / sizeof(Elf64_Rela);
			file->rels = (Elf64_Rela *)malloc(sizeof(Elf64_Rela) * rel_n);
			file->rel_n = rel_n;
			if(file->rels == NULL) {
				fprintf(stderr, "error: out of memory\n");
				ret_code = 0;
				goto EL_ALLOC_ERROR;
			}
			int i;
			Elf64_Rela *r = (Elf64_Rela *)(p + rel_off);
			for(i = 0; i < rel_n; i++) {
				file->rels[i].r_offset	= mem_read64((byte *)(r+i), 0);
				file->rels[i].r_info	= mem_read64((byte *)(r+i), 8);
				file->rels[i].r_addend	= mem_read64((byte *)(r+i), 16);
//				printf("offset=0x%lx, info=0x%lx, addend=%lx\n", file->rels[i].r_offset, file->rels[i].r_info, file->rels[i].r_addend);
			}
		}
	}

	// analyze section names
	file->sec_name_i = file->header.e_shstrndx;
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
	byte *name_tab = p + file->sec_h[file->sec_name_i].sh_offset;
	for(i = 0; i < file->header.e_shnum; i++) {
		const char *sec_name = (char *)(name_tab + file->sec_h[i].sh_name);
		dword sh_size = file->sec_h[i].sh_size;
		dword sh_offset = file->sec_h[i].sh_offset;

//		printf("%8.*s\ttype=%d\toffset=0x%lx\tsize=0x%lx\n", 16, sec_name, file->sec_h[i].sh_type, sh_offset, file->sec_h[i].sh_size);

		if(strncmp(sec_name, ".text", 5) == 0) {
			// load text segment
			page->text_size = sh_size;
			memcpy(page->mem + TEXT_OFFSET, p + sh_offset, sh_size);
		} else if(strncmp(sec_name, ".data", 5) == 0) {
			// load data segment
			memcpy(page->mem + DATA_OFFSET, p + sh_offset, sh_size);
		}
	}

	// analyze symbols
	if(shsymndx != 0) {
		Elf64_Sym *syms = (Elf64_Sym *)(p + file->sec_h[shsymndx].sh_offset);
		int sym_n = file->sec_h[shsymndx].sh_size / sizeof(Elf64_Sym);
		int i;
		file->syms = (Elf64_Sym *)malloc(sizeof(Elf64_Sym) * sym_n);
		file->sym_n = sym_n;
		if(file->syms == NULL) {
			fprintf(stderr, "error: out of memory\n");
			ret_code = 0;
			goto EL_ALLOC_ERROR;
		}

		byte *sym_tab = p + file->sec_h[shstrndx].sh_offset;
		Elf64_Sym *s = (Elf64_Sym *)(p + file->sec_h[shsymndx].sh_offset);
		for(i = 0; i < sym_n; i++) {
			file->syms[i].st_name	= mem_read32((byte *)(s+i), 0);
			file->syms[i].st_info	= mem_read8((byte *)(s+i), 4);
			file->syms[i].st_other	= mem_read8((byte *)(s+i), 5);
			file->syms[i].st_shndx	= mem_read16((byte *)(s+i), 6);
			file->syms[i].st_value	= mem_read64((byte *)(s+i), 8);
			file->syms[i].st_size	= mem_read64((byte *)(s+i), 16);
			/*
			printf("%8s = %8s %x %x %lx %lx\n",
					(char *)(sym_tab + file->syms[i].st_name),
					name_tab + file->sec_h[file->syms[i].st_shndx].sh_name,
					file->syms[i].st_info,
					file->syms[i].st_other,
					file->syms[i].st_value,
					file->syms[i].st_size);
					*/
		}
	}

	int j;
	for(j = 0; j < file->rel_n; j++) { elf_show_rel(file, j); }

	goto EL_RETURN;

EL_ALLOC_ERROR:
	free(file->sec_h); file->sec_h = NULL;
	free(file->rels); file->rels = NULL;
	free(file->syms); file->syms = NULL;
	free(page->mem); page->mem = NULL;
	fclose(file->fp);

EL_RETURN:
	free(p); p = NULL;

	return ret_code;
}

void elf_show_rel(Exefile *file, int n) {
	byte *name_tab = (byte *)malloc(file->sec_h[file->sec_name_i].sh_size);
	if(name_tab == NULL) {
		fprintf(stderr, "error: out of memory\n");
		return;
	} else {
		fseek(file->fp, file->sec_h[file->sec_name_i].sh_offset, SEEK_SET);
		fread(name_tab, file->sec_h[file->sec_name_i].sh_size, sizeof(byte), file->fp);
	}
	Elf64_Rela *r = &file->rels[n];
	Elf64_Sym *s = &file->syms[r->r_info & 0xffffff];
	printf("rel[%d] .text + 0x%02lx -> %s + 0x%02lx\n",
			n,
			r->r_offset,
			name_tab + file->sec_h[s->st_shndx].sh_name,
			r->r_addend);

	free(name_tab); name_tab = NULL;
}

int elf_rel_index(Exefile *file, dword addr) {
	int i;
	for(i = 0; i < file->rel_n; i++) {
		if(addr <= file->rels[i].r_offset && file->rels[i].r_offset < addr + INST_SIZE) {
			return i;
		}
	}
	return -1;
}
