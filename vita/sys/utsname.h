#ifndef __VITA_SYS_UTSNAME__
#define __VITA_SYS_UTSNAME__

struct utsname {
	const char *sysname;
	const char *nodename;
	const char *release;
	const char *version;
	const char *machine;
};

int uname(struct utsname *u);

#endif /* __VITA_SYS_UTSNAME__ */
