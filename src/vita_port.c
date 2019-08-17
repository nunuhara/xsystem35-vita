#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <SDL.h>
#include <psp2/kernel/sysmem.h>
#include "system.h"

// XXX: normal malloc implementation has too small heap size for mmap
void *mmap_malloc(size_t size)
{
	// XXX: undocumented alignment requirement!
	size += sizeof(SceUID);
	size += ((~size) & 0xFFF) + 1;
	SceUID id = sceKernelAllocMemBlock(
		"mmap_alloc",
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW,
		size,
		NULL);
	if (id < 0)
		NOMEMERR();
	SceUID *ptr;
	int rc = sceKernelGetMemBlockBase(id, (void**)&ptr);
	if (rc < 0)
		SYSERROR("sceKernelGetMemBlockBase failed");
	*ptr = id;
	return ptr + 1;
}

void mmap_free(void *ptr)
{
	SceUID id = *((SceUID*)ptr - 1);
	int rc = sceKernelFreeMemBlock(id);
	if (rc < 0)
		SYSERROR("Error freeing memory");
}

// Vita doesn't have mmap, but we only need PROT_READ.
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t off)
{
	if (prot != PROT_READ || addr) {
		errno = ENOTSUP;
		return NULL;
	}

	if (off && lseek(fd, off, SEEK_SET) != off)
		return NULL;

	struct stat sbuf;
	if (fstat(fd, &sbuf) < 0) {
		WARNING("Failed to stat in mmap");
		return NULL;
	}

	ssize_t ret;
	size_t bytes;
	off_t size = sbuf.st_size - off;
	char *buf = mmap_malloc(sbuf.st_size);
	if (!buf) {
		NOMEMERR();
	}

	while (bytes < sbuf.st_size) {
		do {
			ret = read(fd, buf+bytes, sbuf.st_size - bytes);
		} while ((ret < 0) && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK));
		if (ret < 0) {
			free(buf);
			return NULL;
		}
		bytes += ret;
	}

	return buf;
}

int munmap(void *addr, size_t length)
{
	mmap_free(addr);
}

static char vita_cwd[PATH_MAX] = VITA_HOME;
static char path_buf[PATH_MAX] = {0};

char *vita_path(const char *path)
{
	// substitute ux0: for '/'
	if (path[0] == '/')
		snprintf(path_buf, PATH_MAX-1, "ux0:%s", path);
	// substitute vita_cwd for '.'
	else if (path[0] == '.' && path[1] == '/')
		snprintf(path_buf, PATH_MAX-1, "%s%s", vita_cwd, path+1);
	// absolute paths are OK as is
	else if (path[0] && path[1] && path[2] && path[3] == ':')
		strncpy(path_buf, path, PATH_MAX-1);
	// prefix relative paths with vita_cwd/
	else
		snprintf(path_buf, PATH_MAX-1, "%s/%s", vita_cwd, path);
	return path_buf;
}

int chdir(const char *path)
{
	strncpy(vita_cwd, path, PATH_MAX-1);
}

char *getcwd(char *buf, size_t size)
{
	strncpy(buf, vita_cwd, size - 1);
	buf[size-1] = '\0';
	return buf;
}

uid_t getuid(void)
{
	return 1000;
}

char *getlogin(void)
{
	return "vitauser";
}

int execvp(const char *file, char *const argv[])
{
	errno = ENOTSUP;
	return -1;
}

int usleep(useconds_t usec)
{
	SDL_Delay(usec / 1000); // loss of precision, but doesn't matter
}

int uname(struct utsname *u)
{
	u->sysname = "PS Vita";
	u->nodename = "vita";
	u->release = "1.0"; // TODO: get firmware version?
	u->version = "1.0"; // TODO: get firmware version?
	u->machine = "arm";
	return 0;
}
