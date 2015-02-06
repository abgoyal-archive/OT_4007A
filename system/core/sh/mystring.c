
/*	$NetBSD: mystring.c,v 1.16 2003/08/07 09:05:35 agc Exp $	*/


#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)mystring.c	8.2 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: mystring.c,v 1.16 2003/08/07 09:05:35 agc Exp $");
#endif
#endif /* not lint */


#include <stdlib.h>
#include "shell.h"
#include "syntax.h"
#include "error.h"
#include "mystring.h"


char nullstr[1];		/* zero length string */





void
scopyn(const char *from, char *to, int size)
{

	while (--size > 0) {
		if ((*to++ = *from++) == '\0')
			return;
	}
	*to = '\0';
}



int
prefix(const char *pfx, const char *string)
{
	while (*pfx) {
		if (*pfx++ != *string++)
			return 0;
	}
	return 1;
}



int
number(const char *s)
{

	if (! is_number(s))
		error("Illegal number: %s", s);
	return atoi(s);
}




int
is_number(const char *p)
{
	do {
		if (! is_digit(*p))
			return 0;
	} while (*++p != '\0');
	return 1;
}
