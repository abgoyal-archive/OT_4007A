
/*	$NetBSD: alias.h,v 1.6 2003/08/07 09:05:29 agc Exp $	*/


#define ALIASINUSE	1

struct alias {
	struct alias *next;
	char *name;
	char *val;
	int flag;
};

struct alias *lookupalias(char *, int);
char *get_alias_text(char *);
int aliascmd(int, char **);
int unaliascmd(int, char **);
void rmaliases(void);
