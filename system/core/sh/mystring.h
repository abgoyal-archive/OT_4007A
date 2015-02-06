
/*	$NetBSD: mystring.h,v 1.11 2003/08/07 09:05:35 agc Exp $	*/


#include <string.h>

void scopyn(const char *, char *, int);
int prefix(const char *, const char *);
int number(const char *);
int is_number(const char *);

#define equal(s1, s2)	(strcmp(s1, s2) == 0)
#define scopy(s1, s2)	((void)strcpy(s2, s1))
