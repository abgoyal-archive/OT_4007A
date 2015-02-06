
/*	$NetBSD: machdep.h,v 1.11 2003/08/07 09:05:33 agc Exp $	*/



#define SHELL_SIZE (sizeof(union {int i; char *cp; double d; }) - 1)
#define SHELL_ALIGN(nbytes) (((nbytes) + SHELL_SIZE) & ~SHELL_SIZE)
