
/*	$NetBSD: memalloc.c,v 1.28 2003/08/07 09:05:34 agc Exp $	*/


#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)memalloc.c	8.3 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: memalloc.c,v 1.28 2003/08/07 09:05:34 agc Exp $");
#endif
#endif /* not lint */

#include <stdlib.h>
#include <unistd.h>

#include "shell.h"
#include "output.h"
#include "memalloc.h"
#include "error.h"
#include "machdep.h"
#include "mystring.h"


pointer
ckmalloc(int nbytes)
{
	pointer p;

	p = malloc(nbytes);
	if (p == NULL)
		error("Out of space");
	return p;
}



pointer
ckrealloc(pointer p, int nbytes)
{
	p = realloc(p, nbytes);
	if (p == NULL)
		error("Out of space");
	return p;
}



char *
savestr(const char *s)
{
	char *p;

	p = ckmalloc(strlen(s) + 1);
	scopy(s, p);
	return p;
}



#define MINSIZE 504		/* minimum size of a block */

struct stack_block {
	struct stack_block *prev;
	char space[MINSIZE];
};

struct stack_block stackbase;
struct stack_block *stackp = &stackbase;
struct stackmark *markp;
char *stacknxt = stackbase.space;
int stacknleft = MINSIZE;
int sstrnleft;
int herefd = -1;

pointer
stalloc(int nbytes)
{
	char *p;

	nbytes = SHELL_ALIGN(nbytes);
	if (nbytes > stacknleft) {
		int blocksize;
		struct stack_block *sp;

		blocksize = nbytes;
		if (blocksize < MINSIZE)
			blocksize = MINSIZE;
		INTOFF;
		sp = ckmalloc(sizeof(struct stack_block) - MINSIZE + blocksize);
		sp->prev = stackp;
		stacknxt = sp->space;
		stacknleft = blocksize;
		stackp = sp;
		INTON;
	}
	p = stacknxt;
	stacknxt += nbytes;
	stacknleft -= nbytes;
	return p;
}


void
stunalloc(pointer p)
{
	if (p == NULL) {		/*DEBUG */
		write(2, "stunalloc\n", 10);
		abort();
	}
	stacknleft += stacknxt - (char *)p;
	stacknxt = p;
}



void
setstackmark(struct stackmark *mark)
{
	mark->stackp = stackp;
	mark->stacknxt = stacknxt;
	mark->stacknleft = stacknleft;
	mark->marknext = markp;
	markp = mark;
}


void
popstackmark(struct stackmark *mark)
{
	struct stack_block *sp;

	INTOFF;
	markp = mark->marknext;
	while (stackp != mark->stackp) {
		sp = stackp;
		stackp = sp->prev;
		ckfree(sp);
	}
	stacknxt = mark->stacknxt;
	stacknleft = mark->stacknleft;
	INTON;
}



void
growstackblock(void)
{
	int newlen = SHELL_ALIGN(stacknleft * 2 + 100);

	if (stacknxt == stackp->space && stackp != &stackbase) {
		struct stack_block *oldstackp;
		struct stackmark *xmark;
		struct stack_block *sp;

		INTOFF;
		oldstackp = stackp;
		sp = stackp;
		stackp = sp->prev;
		sp = ckrealloc((pointer)sp,
		    sizeof(struct stack_block) - MINSIZE + newlen);
		sp->prev = stackp;
		stackp = sp;
		stacknxt = sp->space;
		stacknleft = newlen;

		/*
		 * Stack marks pointing to the start of the old block
		 * must be relocated to point to the new block 
		 */
		xmark = markp;
		while (xmark != NULL && xmark->stackp == oldstackp) {
			xmark->stackp = stackp;
			xmark->stacknxt = stacknxt;
			xmark->stacknleft = stacknleft;
			xmark = xmark->marknext;
		}
		INTON;
	} else {
		char *oldspace = stacknxt;
		int oldlen = stacknleft;
		char *p = stalloc(newlen);

		(void)memcpy(p, oldspace, oldlen);
		stacknxt = p;			/* free the space */
		stacknleft += newlen;		/* we just allocated */
	}
}

void
grabstackblock(int len)
{
	len = SHELL_ALIGN(len);
	stacknxt += len;
	stacknleft -= len;
}


char *
growstackstr(void)
{
	int len = stackblocksize();
	if (herefd >= 0 && len >= 1024) {
		xwrite(herefd, stackblock(), len);
		sstrnleft = len - 1;
		return stackblock();
	}
	growstackblock();
	sstrnleft = stackblocksize() - len - 1;
	return stackblock() + len;
}


char *
makestrspace(void)
{
	int len = stackblocksize() - sstrnleft;
	growstackblock();
	sstrnleft = stackblocksize() - len;
	return stackblock() + len;
}

void
ungrabstackstr(char *s, char *p)
{
	stacknleft += stacknxt - s;
	stacknxt = s;
	sstrnleft = stacknleft - (p - s);

}
