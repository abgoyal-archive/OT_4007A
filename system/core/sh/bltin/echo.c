
/*	$NetBSD: echo.c,v 1.12 2005/02/06 04:43:43 perry Exp $	*/



//#define main echocmd

#include "bltin.h"

int
echocmd(int argc, char **argv)
{
	char **ap;
	char *p;
	char c;
	int count;
	int nflag = 0;
	int eflag = 0;

	ap = argv;
	if (argc)
		ap++;

	if ((p = *ap) != NULL) {
		if (equal(p, "-n")) {
			nflag = 1;
			ap++;
		} else if (equal(p, "-e")) {
			eflag = 1;
			ap++;
		}
	}

	while ((p = *ap++) != NULL) {
		while ((c = *p++) != '\0') {
			if (c == '\\' && eflag) {
				switch (*p++) {
				case 'a':  c = '\a';  break;	/* bell */
				case 'b':  c = '\b';  break;
				case 'c':  return 0;		/* exit */
				case 'e':  c =  033;  break;	/* escape */
				case 'f':  c = '\f';  break;
				case 'n':  c = '\n';  break;
				case 'r':  c = '\r';  break;
				case 't':  c = '\t';  break;
				case 'v':  c = '\v';  break;
				case '\\':  break;		/* c = '\\' */
				case '0':
					c = 0;
					count = 3;
					while (--count >= 0 && (unsigned)(*p - '0') < 8)
						c = (c << 3) + (*p++ - '0');
					break;
				default:
					/* Output the '/' and char following */
					p--;
					break;
				}
			}
			putchar(c);
		}
		if (*ap)
			putchar(' ');
	}
	if (! nflag)
		putchar('\n');
	return 0;
}
