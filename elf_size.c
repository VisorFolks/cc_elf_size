#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <elf.h>

static bool details;
typedef struct section_info
{
	char *name;
	size_t size;
	uint64_t vma;
	uint32_t flags;
	uint32_t type;
	uint32_t align;
} sec_info_t;

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
				free(buff);
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
					Elf32_Shdr sh;
					sec[i] = (sec_info_t *)malloc(sizeof(sec_info_t));
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
				free(buff);
			}
			break;
		default:
			sec_cnt = -1;
	}
}

static void destroy_section_info(sec_info_t **sec, unsigned int sec_cnt)
{
	if(!sec)
		return;
	for(int i = 0; i < sec_cnt; i++)
	{
		if(sec[i]->name)
			free(sec[i]->name);
		if(sec[i])
			free(sec[i]);
	}
	free(sec);
}

static void print_sections_info(sec_info_t **sec, unsigned int n_sec)
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
}

int main(int argc, char *argv[])
{
	int ret = 0;
	FILE *fd;

	if(argc < 2)
	{
		printf("< ! > Usage: size <elf file> [-d]\n");
		ret = 0;
		goto exit;
	}
	else if(argc > 2)
		if(!strcmp(argv[2], "-d"))
		{
			details = true;
			printf("Details\n");
		}

	fd = fopen(argv[1], "r");
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

	int n_sec = get_section_count(fd);
	sec_info_t **sec = (sec_info_t **)calloc(n_sec, sizeof(sec_info_t *));
	if(!sec)
	{
		ret = -ENOMEM;
		goto exit;
	}
	create_section_info(sec, n_sec, fd);
	printf("Size of %s\n", argv[1]);
	print_sections_info(sec, n_sec);
	destroy_section_info(sec, n_sec);
exit:
	if(fd)
		fclose(fd);
	return ret;
}
