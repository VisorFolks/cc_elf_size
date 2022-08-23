/*
 * CYANCORE LICENSE
 * Copyrights (C) <2022>, Cyancore Team
 *
 * File Name		: elf_size.c
 * Description		: This is a source files for generating custom utiliy
 *			  to compute elf section sizes and memory regions
 * Primary Author	: Akash Kollipara [akashkollipara@gmail.com]
 * Organisation		: VisorFolks
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <elf.h>

static char *size_help =
"Usage : ./size -f <file> -m <name start_addr size> -m <name start_addr size> ...\n\n"
"Options :\n"
"-m: Memory zones, multiple memory zones can be passed. There are 4 keywords for flash or rom,\n"
"    -m with 'Flash', 'flash', 'ROM', 'rom' will be used to compute the size of binary.\n"
"-f: File, only latest of -f files will be processed.\n"
"Example :\n"
"$ ./size -f a.out -m Flash 0x1000 0x200 -m RAM 0x80000000 0x100\n";

typedef struct section_info
{
	char *name;
	size_t size;
	uint64_t vma;
	uint32_t flags;
	uint32_t type;
	uint32_t align;
} sec_info_t;

typedef struct memzone
{
	char *name;
	uint64_t saddr;
	uint64_t eaddr;
	size_t size;
} mz_t;

static uint64_t strtonum(char *i)
{
	if(i[0] == '0' && i[1] == 'x')
		return strtol(i, NULL, 0);
	else
		return strtol(i, NULL, 10);
}

static bool isELF(FILE *fd)
{
	Elf32_Ehdr e;
	fseek(fd, 0, 0);
	fread((void *)&e, 1, EI_NIDENT, fd);
	return strncmp((void *)&e, "\177ELF",4) ? false : true;
}

static int ELFClass(FILE *fd)
{
	Elf32_Ehdr e;
	fseek(fd, 0, 0);
	fread((void *)&e, 1, EI_NIDENT, fd);
	return e.e_ident[EI_CLASS];
}

static int get_section_count(FILE *fd)
{
	int sec_cnt;
	switch(ELFClass(fd))
	{
		case ELFCLASS32:
			{
				Elf32_Ehdr e;
				fseek(fd, 0, 0);
				fread((void *)&e, 1, sizeof(Elf32_Ehdr), fd);
				sec_cnt = e.e_shnum;
			}
			break;
		case ELFCLASS64:
			{
				Elf64_Ehdr e;
				fseek(fd, 0, 0);
				fread((void *)&e, 1, sizeof(Elf64_Ehdr), fd);
				sec_cnt = e.e_shnum;
			}
			break;
		default:
			sec_cnt = -1;
	}
	return sec_cnt;
}

static void create_section_info(sec_info_t **sec, unsigned int sec_cnt, FILE *fd)
{
	unsigned int sec_offset;
	size_t name_len;
	char *buff;
	switch(ELFClass(fd))
	{
		case ELFCLASS32:
			{
				Elf32_Ehdr e;
				Elf32_Off temp_offset;
				Elf32_Word temp_size;
				fseek(fd, 0, 0);
				fread((void *)&e, 1, sizeof(Elf32_Ehdr), fd);
				sec_offset = e.e_shoff;
				fseek(fd, sec_offset, SEEK_SET);
				for(int i = 0; i < sec_cnt; i++)
				{
					Elf32_Shdr sh;
					sec[i] = (sec_info_t *)malloc(sizeof(sec_info_t));
					memset(sec[i], 0, sizeof(sec_info_t));
					fread((void *)&sh, 1, sizeof(Elf32_Shdr), fd);
					sec[i]->size = sh.sh_size;
					sec[i]->vma = sh.sh_addr;
					sec[i]->flags = sh.sh_flags;
					sec[i]->align = sh.sh_addralign;
					sec[i]->type = sh.sh_type;

					if(i == e.e_shstrndx)
					{
						temp_size = sh.sh_size;
						temp_offset = sh.sh_offset;
					}
				}
				
				buff = (char *)malloc(temp_size);
				fseek(fd, temp_offset, SEEK_SET);
				fread((void *)buff, 1, temp_size, fd);
				fseek(fd, sec_offset, SEEK_SET);
				for(int i = 0; i < sec_cnt; i++)
				{
					Elf32_Shdr sh;
					fread((void *)&sh, 1, sizeof(Elf32_Shdr), fd);
					name_len = strlen((char*)(buff + sh.sh_name));
					if(!name_len)
						continue;
					sec[i]->name = malloc(name_len);
					strcpy(sec[i]->name, (char*)(buff + sh.sh_name));
				}
			}
			break;
		case ELFCLASS64:
			{
				Elf64_Ehdr e;
				Elf64_Off temp_offset;
				Elf64_Word temp_size;
				fseek(fd, 0, 0);
				fread((void *)&e, 1, sizeof(Elf64_Ehdr), fd);
				sec_offset = e.e_shoff;
				fseek(fd, sec_offset, SEEK_SET);
				for(int i = 0; i < sec_cnt; i++)
				{
					Elf64_Shdr sh;
					sec[i] = (sec_info_t *)malloc(sizeof(sec_info_t));
					memset(sec[i], 0, sizeof(sec_info_t));
					fread((void *)&sh, 1, sizeof(Elf64_Shdr), fd);
					sec[i]->size = sh.sh_size;
					sec[i]->vma = sh.sh_addr;
					sec[i]->flags = sh.sh_flags;
					sec[i]->align = sh.sh_addralign;
					sec[i]->type = sh.sh_type;

					if(i == e.e_shstrndx)
					{
						temp_size = sh.sh_size;
						temp_offset = sh.sh_offset;
					}
				}
				
				buff = (char *)malloc(temp_size);
				fseek(fd, temp_offset, SEEK_SET);
				fread((void *)buff, 1, temp_size, fd);
				fseek(fd, sec_offset, SEEK_SET);
				for(int i = 0; i < sec_cnt; i++)
				{
					Elf32_Shdr sh;
					fread((void *)&sh, 1, sizeof(Elf64_Shdr), fd);
					name_len = strlen((char*)(buff + sh.sh_name));
					sec[i]->name = malloc(name_len);
					strcpy(sec[i]->name, (char*)(buff + sh.sh_name));
				}
			}
			break;
		default:
			sec_cnt = -1;
	}
	free(buff);
}

static void destroy_section_info(sec_info_t **sec, unsigned int sec_cnt)
{
	if(!sec)
		return;
	for(int i = 0; i < sec_cnt; i++)
	{
		if(sec[i])
		{
			if(sec[i]->name)
				free(sec[i]->name);
			free(sec[i]);
		}
	}
	free(sec);
}

static void print_sections_info(sec_info_t **sec, unsigned int n_sec, mz_t **mz, int mcnt)
{
	int cntr = 0;
	size_t bin_size = 0;
	for(int i = 0; i < 70; i++)
		printf("=");
	printf("\n %5s %13s %17s %17s %11s\n", "S.No.", "Section", "VMA", "Size", "Load");
	for(int i = 0; i < 70; i++)
		printf("=");
	printf("\n");
	for(int i = 0; i < n_sec; i++)
	{
		if(!sec[i]->flags)
			continue;
		char temp = sec[i]->type & SHT_PROGBITS ? '*' : 0;
		if(sec[i]->type & SHT_PROGBITS)
			bin_size += sec[i]->size;
		++cntr;
		printf("%3d) %20s\t0x%08lx\t%10lu B\t%3c\n", cntr, sec[i]->name, sec[i]->vma, sec[i]->size, temp);
	}
	for(int i = 0; i < 70; i++)
		printf("=");
	printf("\nTotal Size of bin file = %lu Bytes\n", bin_size);

	if(mz && mcnt)
	{
		size_t size;
		int pc;
		for(int m = 0; m < mcnt; m++)
		{
			size = 0;
			printf("%6s Usage : ", mz[m]->name);
			if(!strcmp(mz[m]->name, "ROM") || !strcmp(mz[m]->name, "Flash") ||
			   !strcmp(mz[m]->name, "rom") || !strcmp(mz[m]->name, "flash"))
			{
				size = bin_size;
			   	goto skip_compute_size;
			}
			for(int i = 0; i < n_sec; i++)
				if(sec[i]->vma >= mz[m]->saddr && sec[i]->vma < mz[m]->eaddr)
					size += sec[i]->size;
skip_compute_size:
			pc = size * 100 / mz[m]->size;
			printf("[");
			for(int i = 0; i < 40; i ++)
				printf("%c", (i < (int)((float)pc/2.5) ? '=':' '));
			printf("] %3u%%, %lu / %lu B\n", pc, size, mz[m]->size);

		}
	}
}

int main(int argc, char *argv[])
{
	int ret = 0, mem_cnt = 0, n_sec = 0;
	char *fname;
	FILE *fd = NULL;
	mz_t **mz = NULL;
	sec_info_t **sec = NULL;

	if(argc < 3)
		goto usage_exit;

	for(int i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-f"))
			fname = argv[++i];
		else if(!strcmp(argv[i], "-m"))
		{
			if(argc - i <= 3)
				goto usage_exit;
			mem_cnt ++;
			mz = (mz_t **)realloc(mz, (sizeof(mz_t *)*mem_cnt));
			mz[mem_cnt - 1] = (mz_t *)malloc(sizeof(mz_t));
			mz[mem_cnt - 1]->name = argv[++i];
			mz[mem_cnt - 1]->saddr = strtonum(argv[++i]);
			mz[mem_cnt - 1]->size = strtonum(argv[++i]);
			mz[mem_cnt - 1]->eaddr = mz[mem_cnt - 1]->saddr + mz[mem_cnt - 1]->size;
		}
	}

	fd = fopen(fname, "r");
	if(!fd)
	{
		printf("< x > Failed to open %s\n", argv[1]);
		ret = -1;	// Replace with correct errno
		goto exit;
	}

	if(!isELF(fd))
	{
		printf("< x > File is non elf spec comliant!\n");
		ret = -1;
		goto exit;	// Replace with correct errno
	}

	n_sec = get_section_count(fd);
	sec = (sec_info_t **)calloc(n_sec, sizeof(sec_info_t *));
	if(!sec)
	{
		ret = -ENOMEM;
		goto exit;
	}
	create_section_info(sec, n_sec, fd);
	printf("Size of %s\n", fname);
	print_sections_info(sec, n_sec, mz, mem_cnt);
	goto exit;
usage_exit:
	printf("%s", size_help);
exit:
	destroy_section_info(sec, n_sec);
	if(mz && mem_cnt)
	{
		for(int i = 0; i < mem_cnt; i++)
			if(mz[i])
				free(mz[i]);
		free(mz);
	}
	if(fd)
		fclose(fd);
	return ret;
}
