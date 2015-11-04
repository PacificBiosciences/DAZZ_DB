#include "open_compressed.h"
#include <assert.h>	// assert()
#include <errno.h>	// EINVAL, EISDIR, ENFILE, ENOENT, errno
#include <fcntl.h>	// O_RDONLY, open()
#include <stdio.h>	// fprintf(), stderr
#include <stdlib.h>	// free(), malloc()
#include <string.h>	// memcpy(), memmove(), strcmp(), strerror(), strlen()
#include <sys/stat.h>	// S_IFDIR, stat(), struct stat
#include <sys/types.h>	// pid_t, size_t, ssize_t
#include <sys/wait.h>	// WNOHANG, waitpid()
#include <unistd.h>	// _SC_OPEN_MAX, _exit(), close(), dup2(), execlp(), fork(), pipe(), read(), sysconf()

#define BUFSIZE 32768

typedef struct _pid_list {
	pid_t pid;
	struct _pid_list *next;
} pid_list;

// prevent reuse of closed stream
static int m_already_closed_stdin = 0;
// map of open files to gzip process id's
static pid_t *m_open_processes = 0;
// list of closed processes that need to be waited on
static pid_list m_closed_processes = { 0, 0 };

static void finish_nohang(void) {
	pid_list *a = &m_closed_processes;
	while (a->next) {
		if (waitpid(a->next->pid, 0, WNOHANG) > 0) {
			pid_list * const b = a->next;
			a->next = a->next->next;
			free(b);
		} else {
			a = a->next;
		}
	}
}

static ssize_t open_max;
// internal file buffers
char ** buffers = 0;
ssize_t *buffer_start = 0;
ssize_t *buffer_length = 0;
static int already_init = 0;

void open_compressed_init(void) {
	if (already_init) {
		return;
	}
	already_init = 1;
	open_max = sysconf(_SC_OPEN_MAX);
	assert(open_max);
	buffers = malloc(open_max * sizeof(char *));
	assert(buffers);
	buffer_start = malloc(open_max * sizeof(ssize_t));
	assert(buffer_start);
	buffer_length = malloc(open_max * sizeof(ssize_t));
	assert(buffer_length);
	m_open_processes = malloc(open_max * sizeof(pid_t));
	assert(buffer_length);
	// settings for stdin
	buffer_start[0] = buffer_length[0] = 0;
	ssize_t i = 0;
	for (i = 0; i != open_max; ++i) {
		buffers[i] = 0;
		m_open_processes[i] = 0;
	}
}

void open_compressed_finish(void) {
	if (!already_init) {
		return;
	}
	// close these files now, to give processes time to finish
	ssize_t i;
	for (i = 0; i != open_max; ++i) {
		if (m_open_processes[i]) {
			close(i);
		}
	}
	for (i = 0; i != open_max; ++i) {
		if (buffers[i]) {
			free(buffers[i]);
		}
	}
	free(buffers);
	free(buffer_start);
	free(buffer_length);
	while (m_closed_processes.next) {
		waitpid(m_closed_processes.next->pid, 0, 0);
		pid_list * const b = m_closed_processes.next;
		m_closed_processes.next = m_closed_processes.next->next;
		free(b);
	}
	for (i = 0; i != open_max; ++i) {
		if (m_open_processes[i]) {
			waitpid(m_open_processes[i], 0, 0);
		}
	}
	free(m_open_processes);
	already_init = 0;
}

// close the file and wait on the gzip process, if any

void close_compressed(const int i) {
	assert(-1 < i && i < open_max);
	close(i);
	if (i == 0) {
		m_already_closed_stdin = 1;
	}
	if (m_open_processes[i]) {
		pid_list * const a = malloc(sizeof(pid_list));
		assert(a);
		a->pid = m_open_processes[i];
		a->next = m_closed_processes.next;
		m_closed_processes.next = a;
		m_open_processes[i] = 0;
	}
	finish_nohang();
}

// the last suffix_size entry is a marker for the end of the list
static const char * const suffix_list[4] = { ".gz", ".bz2", ".xz", ".Z" };
static const size_t const suffix_size[5] = { 3, 4, 3, 2, 0 };
#define LONGEST_SUFFIX_SIZE 4

// simply return the suffix of the file name, if it matches one of the list

const char *get_suffix(const char * const filename) {
	const size_t filename_size = strlen(filename);
	size_t i;
	for (i = 0; suffix_size[i]; ++i) {
		const size_t j = suffix_size[i];
		if (filename_size > j && strcmp(filename + filename_size - j, suffix_list[i]) == 0) {
			return suffix_list[i];
			break;
		}
	}
	return 0;
}

// returns null, .Z, .gz, .xz, or .bz2; checks to see if the filename
// ends in any given suffix, if not checks to see if a file with the given
// suffix exists; filename may be modified to add a suffix

int find_suffix(const char ** const filename, const char ** const suffix) {
	*suffix = 0;
	const size_t filename_size = strlen(*filename);
	size_t i;
	for (i = 0; suffix_size[i]; ++i) {
		const size_t j = suffix_size[i];
		if (filename_size > j && strcmp(*filename + filename_size - j, suffix_list[i]) == 0) {
			*suffix = suffix_list[i];
			break;
		}
	}
	struct stat buf;
	if (stat(*filename, &buf) == 0) {
		// only open regular files
		if ((buf.st_mode & S_IFDIR) != 0) {
			errno = EISDIR;
			return -1;
		} else {
			return 0;
		}
	} else if (errno != ENOENT) {
		fprintf(stderr, "Error: stat: %s: %s\n", *filename, strerror(errno));
		return -1;
	} else if (!*suffix) {	// suffix was given in filename
		return -1;
	}
	// try to guess suffix
	char *s = malloc(filename_size + LONGEST_SUFFIX_SIZE + 1);
	assert(s);
	for (i = 0; suffix_size[i]; ++i) {
		memcpy(s, *filename, filename_size);
		memcpy(s + filename_size, suffix_list[i], suffix_size[i] + 1);
		if (stat(s, &buf) == -1) {
			if (errno != ENOENT) {
				fprintf(stderr, "Error: stat: %s: %s\n", s, strerror(errno));
				free(s);
				return -1;
			}
		} else if ((buf.st_mode & S_IFDIR) == 0) {
			*suffix = suffix_list[i];
			*filename = s;
			return 0;
		}
	}
	free(s);
	errno = ENOENT;
	return -1;
}

// open a file, with gzip/bzip/xz if the filename ends in .gz, .bz2, .xz, or .Z;
// if file is not found, .gz, .bz2, .xz, and .Z are added to end, to see
// if file is compressed

int open_compressed(const char * const filename) {
	int fd = -1;
	const char *s = filename;
	const char *suffix = 0;
	// see if file exists
	if (strlen(s) != 0 && strcmp(s, "-") != 0 && find_suffix(&s, &suffix) == -1) {
		return -1;
	}
	if (suffix) {
		pid_t pid;
		int pipefd[2];
		if (pipe(pipefd) == -1) {
			fprintf(stderr, "Error: pipe: %s\n", strerror(errno));
			return -1;
		} else if (pipefd[0] >= open_max) {	// too many open files
			close(pipefd[0]);
			close(pipefd[1]);
			errno = ENFILE;
			fprintf(stderr, "Error: open: %s\n", strerror(errno));
			return -1;
		} else if ((pid = fork()) == -1) {
			fprintf(stderr, "Error: fork: %s\n", strerror(errno));
			close(pipefd[0]);
			close(pipefd[1]);
			return -1;
		} else if (pid == 0) {	// child
			close(0);
			close(pipefd[0]);
			if (dup2(pipefd[1], 1) == -1) {
				fprintf(stderr, "Error: dup2: %s\n", strerror(errno));
				_exit(1);
			}
			close(pipefd[1]);
			// close everything except stdin, stdout, and stderr
			int i;
			for (i = 3; i < open_max; ++i) {
				close(i);
			}
			if (strcmp(suffix, ".bz2") == 0) {
				if (execlp("bzip2", "bzip2", "-d", "-c", s, (char *)0) == -1) {
					fprintf(stderr, "Error: execlp bzip2 -d -c: %s\n", strerror(errno));
				}
			} else if (strcmp(suffix, ".xz") == 0) {
				if (execlp("xz", "xz", "-d", "-c", s, (char *)0) == -1) {
					fprintf(stderr, "Error: execlp xz -d -c: %s\n", strerror(errno));
				}
			} else {
				if (execlp("gzip", "gzip", "-d", "-c", s, (char *)0) == -1) {
					fprintf(stderr, "Error: execlp gzip -d -c: %s\n", strerror(errno));
				}
			}
			_exit(1);
		} else {		// parent
			close(pipefd[1]);
			fd = pipefd[0];
			m_open_processes[fd] = pid;
		}
	} else if (strlen(s) == 0 || strcmp(s, "-") == 0) {
		if (m_already_closed_stdin) {
			return -1;
		}
		fd = 0;
		if (!buffers[fd]) {
			buffers[fd] = malloc(BUFSIZE);
			assert(buffers[fd]);
		}
		// don't reset start/length, to allow rereading of
		// buffered parts of the stream; resize won't change
		// buffer if old size == new size
		return fd;
	} else if ((fd = open(s, O_RDONLY)) == -1) {
		fprintf(stderr, "Error: open: %s\n", strerror(errno));
		if (s != filename) {
			free((char *)s);
		}
		return -1;
	} else if (fd >= open_max) {		// too many open files
		close(fd);
		errno = ENFILE;
		fprintf(stderr, "Error: open: %s\n", strerror(errno));
		if (s != filename) {
			free((char *)s);
		}
		return -1;
	}
	if (!buffers[fd]) {
		buffers[fd] = malloc(BUFSIZE);
		assert(buffers[fd]);
	}
	buffer_start[fd] = buffer_length[fd] = 0;
	if (s != filename) {
		free((char *)s);
	}
	return fd;
}

// read input from a file descriptor - returns data up to end of line or
// end of file; returns -1 on error, otherwise number of characters read

ssize_t pfgets(const int fd, void * const ptr, size_t line_size) {
	if (fd < 0 || open_max <= fd) {
		fprintf(stderr, "Error: pfgets: fd out of range: %d\n", fd);
		return -1;
	}
	if (!buffers[fd]) {
		fprintf(stderr, "Error: pfgets: buffer unallocated\n");
		return -1;
	}
	--line_size;	// leave space for trailing '\0'
	char *line = (char *)ptr;
	const char * const line_start = line;
	char * const buf = buffers[fd];
	size_t i = buffer_start[fd];
	ssize_t j = buffer_length[fd];
	for (;;) {
		size_t k;
		const size_t k_end = (size_t)j > i + line_size ? i + line_size : (size_t)j;
		for (k = i; k != k_end && buf[k] != '\n'; ++k) { }
		size_t copy_size = k - i;
		// if we haven't run out of buffer (or space), include delimeter
		if (k != k_end && buf[k] == '\n') {
			// modify copy_size instead of k to avoid confusing
			// k != j comparison below
			++copy_size;
		}
		memcpy(line, buf + i, copy_size);
		line += copy_size;
		if (k != (size_t)j) {
			buffer_start[fd] = i + copy_size;
			buffer_length[fd] = j;
			line[0] = 0;	// add trailing '\0'
			return (ssize_t)(line - line_start);
		}
		line_size -= copy_size;
		i = 0;
		j = read(fd, buf, BUFSIZE);
		if (j <= 0) {
			if (j == -1) {
				fprintf(stderr, "Error: read(%d): %s\n", fd, strerror(errno));
			}
			buffer_start[fd] = buffer_length[fd] = 0;
			line[0] = 0;	// add trailing '\0'
			// only return -1 if we don't return anything else
			return line == line_start ? -1 : (ssize_t)(line - line_start);
		}
	}
}

// read up to size bytes from fd and put them into ptr

ssize_t pfread(const int fd, void * const ptr, const size_t size) {
	if (fd < 0 || open_max <= fd) {
		fprintf(stderr, "Error: pfread: fd out of range: %d\n", fd);
		return -1;
	}
	if (!buffers[fd]) {
		fprintf(stderr, "Error: pfread: buffer unallocated\n");
		return -1;
	}
	char *s = (char *)ptr;
	size_t k = size;
	char * const buf = buffers[fd];
	size_t i = buffer_start[fd];
	ssize_t j = buffer_length[fd];
	for (;;) {
		if ((size_t)j - i >= k) {
			memcpy(s, buf + i, k);
			buffer_start[fd] = i + k;
			buffer_length[fd] = j;
			return size;
		}
		const size_t n = (size_t)j - i;
		memcpy(s, buf + i, n);
		s += n;
		k -= n;
		i = 0;
		j = read(fd, buf, BUFSIZE);
		if (j <= 0) {
			if (j == -1) {
				fprintf(stderr, "Error: read(%d): %s\n", fd, strerror(errno));
			}
			buffer_start[fd] = buffer_length[fd] = 0;
			// only return -1 if we don't return anything else
			return k == size ? -1 : (ssize_t)(size - k);
		}
	}
}

// like pfread(), but doesn't use buffer; obviously, don't use along with
// pfread(); currently intended for testing purposes only

ssize_t pfread2(const int fd, void * const ptr, size_t size) {
	assert(-1 < fd && fd < open_max);
	char *buf = (char *)ptr;
	while (size != 0) {
		const ssize_t j = read(fd, buf, size);
		if (j <= 0) {
			if (j == -1) {
				fprintf(stderr, "Error: read(%d): %s\n", fd, strerror(errno));
				// only return -1 if we haven't read anything
				if (buf == ptr) {
					return -1;
				}
			}
			break;
		}
		size -= j;
		buf += j;
	}
	return buf - (char *)ptr;
}

// like pfread, but don't remove anything unread from the buffer
// (however, you can add to it, and move stuff around a bit)

ssize_t pfpeek(const int fd, void * const ptr, size_t size) {
	if (fd < 0 || open_max <= fd) {
		fprintf(stderr, "Error: pfpeek: fd out of range: %d\n", fd);
		return -1;
	}
	if (!buffers[fd]) {
		fprintf(stderr, "Error: pfpeek: buffer unallocated\n");
		return -1;
	}
	if (size > BUFSIZE) {
		size = BUFSIZE;
	}
	char * const s = (char *)ptr;
	char * const buf = buffers[fd];
	size_t i = buffer_start[fd];
	size_t j = buffer_length[fd];
	if (size > BUFSIZE - (size_t)i) {	// need to make space
		// move unread section to the front
		j -= i;
		memmove(buf, buf + i, j);
		i = 0;
	}
	for (;;) {
		const size_t n = j - i;
		if (n >= size) {
			memcpy(s, buf + i, size);
			return size;
		}
		// we only need (size - n), but fill the buffer anyway
		const ssize_t k = read(fd, buf + j, BUFSIZE - j);
		if (k <= 0) {
			if (k == -1) {
				fprintf(stderr, "Error: read(%d): %s\n", fd, strerror(errno));
			}
			// only return -1 if we don't return anything else
			if (n == 0) {
				return -1;
			} else {
				memcpy(s, buf + i, n);
				return n;
			}
		}
		j += k;
	}
}
