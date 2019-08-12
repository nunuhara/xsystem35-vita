#ifndef __VITA_SYS_MMAN__
#define __VITA_SYS_MMAN__

#include <sys/types.h>

#define PROT_READ 1
#define MAP_SHARED 1
#define MAP_FAILED NULL
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t off);
int munmap(void *addr, size_t length);

#endif /* __VITA_SYS_MMAN__ */
