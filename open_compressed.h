#ifndef _OPEN_COMPRESSED_H
#define _OPEN_COMPRESSED_H

#include <sys/types.h>	// size_t, ssize_t

extern void open_compressed_init(void);
extern void open_compressed_finish(void);
extern const char *get_suffix(const char *filename);
// if filename is changed to have a suffix added, the resulting pointer
// will need to be properly freed
extern int find_suffix(const char **filename, const char **suffix);
extern int open_compressed(const char *filename);
extern void close_compressed(int fd);
extern ssize_t pfgets(int fd, void *buf, size_t buf_size);
extern ssize_t pfread(int fd, void *buf, size_t buf_size);
extern ssize_t pfread2(int fd, void *buf, size_t buf_size);
extern ssize_t pfpeek(int fd, void *buf, size_t buf_size);

#endif // !_OPEN_COMPRESSED_H
