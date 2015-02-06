
/*	$NetBSD: var.c,v 1.36 2004/10/06 10:23:43 enami Exp $	*/


#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)var.c	8.3 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: var.c,v 1.36 2004/10/06 10:23:43 enami Exp $");
#endif
#endif /* not lint */

#include <unistd.h>
#include <stdlib.h>
#include <paths.h>


#include "shell.h"
#include "output.h"
#include "expand.h"
#include "nodes.h"	/* for other headers */
#include "eval.h"	/* defines cmdenviron */
#include "exec.h"
#include "syntax.h"
#include "options.h"
#include "var.h"
#include "memalloc.h"
#include "error.h"
#include "mystring.h"
#include "parser.h"
#include "show.h"
#ifndef SMALL
#include "myhistedit.h"
#endif

#ifdef SMALL
#define VTABSIZE 39
#else
#define VTABSIZE 517
#endif


struct varinit {
	struct var *var;
	int flags;
	const char *text;
	void (*func)(const char *);
};


#if ATTY
struct var vatty;
#endif
#ifdef WITH_HISTORY
struct var vhistsize;
struct var vterm;
#endif
struct var vifs;
struct var vmpath;
struct var vpath;
struct var vps1;
struct var vps2;
struct var vps4;
struct var vvers;
struct var voptind;

const struct varinit varinit[] = {
#if ATTY
	{ &vatty,	VSTRFIXED|VTEXTFIXED|VUNSET,	"ATTY=",
	  NULL },
#endif
#ifdef WITH_HISTORY
	{ &vhistsize,	VSTRFIXED|VTEXTFIXED|VUNSET,	"HISTSIZE=",
	  sethistsize },
#endif
	{ &vifs,	VSTRFIXED|VTEXTFIXED,		"IFS= \t\n",
	  NULL },
	{ &vmpath,	VSTRFIXED|VTEXTFIXED|VUNSET,	"MAILPATH=",
	  NULL },
	{ &vpath,	VSTRFIXED|VTEXTFIXED,		"PATH=" _PATH_DEFPATH,
	  changepath },
	/*
	 * vps1 depends on uid
	 */
	{ &vps2,	VSTRFIXED|VTEXTFIXED,		"PS2=> ",
	  NULL },
	{ &vps4,	VSTRFIXED|VTEXTFIXED,		"PS4=+ ",
	  NULL },
#ifdef WITH_HISTORY
	{ &vterm,	VSTRFIXED|VTEXTFIXED|VUNSET,	"TERM=",
	  setterm },
#endif
	{ &voptind,	VSTRFIXED|VTEXTFIXED|VNOFUNC,	"OPTIND=1",
	  getoptsreset },
	{ NULL,	0,				NULL,
	  NULL }
};

struct var *vartab[VTABSIZE];

STATIC int strequal(const char *, const char *);
STATIC struct var *find_var(const char *, struct var ***, int *);


#ifdef mkinit
INCLUDE "var.h"
MKINIT char **environ;
INIT {
	char **envp;

	initvar();
	for (envp = environ ; *envp ; envp++) {
		if (strchr(*envp, '=')) {
			setvareq(*envp, VEXPORT|VTEXTFIXED);
		}
	}
}
#endif



void
initvar(void)
{
	const struct varinit *ip;
	struct var *vp;
	struct var **vpp;

	for (ip = varinit ; (vp = ip->var) != NULL ; ip++) {
		if (find_var(ip->text, &vpp, &vp->name_len) != NULL)
			continue;
		vp->next = *vpp;
		*vpp = vp;
		vp->text = strdup(ip->text);
		vp->flags = ip->flags;
		vp->func = ip->func;
	}
	/*
	 * PS1 depends on uid
	 */
	if (find_var("PS1", &vpp, &vps1.name_len) == NULL) {
		vps1.next = *vpp;
		*vpp = &vps1;
		vps1.text = strdup(geteuid() ? "PS1=$ " : "PS1=# ");
		vps1.flags = VSTRFIXED|VTEXTFIXED;
	}
}


int
setvarsafe(const char *name, const char *val, int flags)
{
	struct jmploc jmploc;
	struct jmploc *volatile savehandler = handler;
	int err = 0;
#ifdef __GNUC__
	(void) &err;
#endif

	if (setjmp(jmploc.loc))
		err = 1;
	else {
		handler = &jmploc;
		setvar(name, val, flags);
	}
	handler = savehandler;
	return err;
}


void
setvar(const char *name, const char *val, int flags)
{
	const char *p;
	const char *q;
	char *d;
	int len;
	int namelen;
	char *nameeq;
	int isbad;

	isbad = 0;
	p = name;
	if (! is_name(*p))
		isbad = 1;
	p++;
	for (;;) {
		if (! is_in_name(*p)) {
			if (*p == '\0' || *p == '=')
				break;
			isbad = 1;
		}
		p++;
	}
	namelen = p - name;
	if (isbad)
		error("%.*s: bad variable name", namelen, name);
	len = namelen + 2;		/* 2 is space for '=' and '\0' */
	if (val == NULL) {
		flags |= VUNSET;
	} else {
		len += strlen(val);
	}
	d = nameeq = ckmalloc(len);
	q = name;
	while (--namelen >= 0)
		*d++ = *q++;
	*d++ = '=';
	*d = '\0';
	if (val)
		scopy(val, d);
	setvareq(nameeq, flags);
}




void
setvareq(char *s, int flags)
{
	struct var *vp, **vpp;
	int nlen;

	if (aflag)
		flags |= VEXPORT;
	vp = find_var(s, &vpp, &nlen);
	if (vp != NULL) {
		if (vp->flags & VREADONLY)
			error("%.*s: is read only", vp->name_len, s);
		if (flags & VNOSET)
			return;
		INTOFF;

		if (vp->func && (flags & VNOFUNC) == 0)
			(*vp->func)(s + vp->name_len + 1);

		if ((vp->flags & (VTEXTFIXED|VSTACK)) == 0)
			ckfree(vp->text);

		vp->flags &= ~(VTEXTFIXED|VSTACK|VUNSET);
		vp->flags |= flags & ~VNOFUNC;
		vp->text = s;

		INTON;
		return;
	}
	/* not found */
	if (flags & VNOSET)
		return;
	vp = ckmalloc(sizeof (*vp));
	vp->flags = flags & ~VNOFUNC;
	vp->text = s;
	vp->name_len = nlen;
	vp->next = *vpp;
	vp->func = NULL;
	*vpp = vp;
}




void
listsetvar(struct strlist *list, int flags)
{
	struct strlist *lp;

	INTOFF;
	for (lp = list ; lp ; lp = lp->next) {
		setvareq(savestr(lp->text), flags);
	}
	INTON;
}

void
listmklocal(struct strlist *list, int flags)
{
	struct strlist *lp;

	for (lp = list ; lp ; lp = lp->next)
		mklocal(lp->text, flags);
}



char *
lookupvar(const char *name)
{
	struct var *v;

	v = find_var(name, NULL, NULL);
	if (v == NULL || v->flags & VUNSET)
		return NULL;
	return v->text + v->name_len + 1;
}




char *
bltinlookup(const char *name, int doall)
{
	struct strlist *sp;
	struct var *v;

	for (sp = cmdenviron ; sp ; sp = sp->next) {
		if (strequal(sp->text, name))
			return strchr(sp->text, '=') + 1;
	}

	v = find_var(name, NULL, NULL);

	if (v == NULL || v->flags & VUNSET || (!doall && !(v->flags & VEXPORT)))
		return NULL;
	return v->text + v->name_len + 1;
}




char **
environment(void)
{
	int nenv;
	struct var **vpp;
	struct var *vp;
	char **env;
	char **ep;

	nenv = 0;
	for (vpp = vartab ; vpp < vartab + VTABSIZE ; vpp++) {
		for (vp = *vpp ; vp ; vp = vp->next)
			if (vp->flags & VEXPORT)
				nenv++;
	}
	ep = env = stalloc((nenv + 1) * sizeof *env);
	for (vpp = vartab ; vpp < vartab + VTABSIZE ; vpp++) {
		for (vp = *vpp ; vp ; vp = vp->next)
			if (vp->flags & VEXPORT)
				*ep++ = vp->text;
	}
	*ep = NULL;
	return env;
}



#ifdef mkinit
void shprocvar(void);

SHELLPROC {
	shprocvar();
}
#endif

void
shprocvar(void)
{
	struct var **vpp;
	struct var *vp, **prev;

	for (vpp = vartab ; vpp < vartab + VTABSIZE ; vpp++) {
		for (prev = vpp ; (vp = *prev) != NULL ; ) {
			if ((vp->flags & VEXPORT) == 0) {
				*prev = vp->next;
				if ((vp->flags & VTEXTFIXED) == 0)
					ckfree(vp->text);
				if ((vp->flags & VSTRFIXED) == 0)
					ckfree(vp);
			} else {
				if (vp->flags & VSTACK) {
					vp->text = savestr(vp->text);
					vp->flags &=~ VSTACK;
				}
				prev = &vp->next;
			}
		}
	}
	initvar();
}




void
print_quoted(const char *p)
{
	const char *q;

	if (strcspn(p, "|&;<>()$`\\\"' \t\n*?[]#~=%") == strlen(p)) {
		out1fmt("%s", p);
		return;
	}
	while (*p) {
		if (*p == '\'') {
			out1fmt("\\'");
			p++;
			continue;
		}
		q = index(p, '\'');
		if (!q) {
			out1fmt("'%s'", p );
			return;
		}
		out1fmt("'%.*s'", (int)(q - p), p );
		p = q;
	}
}

static int
sort_var(const void *v_v1, const void *v_v2)
{
	const struct var * const *v1 = v_v1;
	const struct var * const *v2 = v_v2;

	/* XXX Will anyone notice we include the '=' of the shorter name? */
	return strcmp((*v1)->text, (*v2)->text);
}


int
showvars(const char *name, int flag, int show_value)
{
	struct var **vpp;
	struct var *vp;
	const char *p;

	static struct var **list;	/* static in case we are interrupted */
	static int list_len;
	int count = 0;

	if (!list) {
		list_len = 32;
		list = ckmalloc(list_len * sizeof *list);
	}

	for (vpp = vartab ; vpp < vartab + VTABSIZE ; vpp++) {
		for (vp = *vpp ; vp ; vp = vp->next) {
			if (flag && !(vp->flags & flag))
				continue;
			if (vp->flags & VUNSET && !(show_value & 2))
				continue;
			if (count >= list_len) {
				list = ckrealloc(list,
					(list_len << 1) * sizeof *list);
				list_len <<= 1;
			}
			list[count++] = vp;
		}
	}

	qsort(list, count, sizeof *list, sort_var);

	for (vpp = list; count--; vpp++) {
		vp = *vpp;
		if (name)
			out1fmt("%s ", name);
		for (p = vp->text ; *p != '=' ; p++)
			out1c(*p);
		if (!(vp->flags & VUNSET) && show_value) {
			out1fmt("=");
			print_quoted(++p);
		}
		out1c('\n');
	}
	return 0;
}




int
exportcmd(int argc, char **argv)
{
	struct var *vp;
	char *name;
	const char *p;
	int flag = argv[0][0] == 'r'? VREADONLY : VEXPORT;
	int pflag;

	pflag = nextopt("p") == 'p' ? 3 : 0;
	if (argc <= 1 || pflag) {
		showvars( pflag ? argv[0] : 0, flag, pflag );
		return 0;
	}

	while ((name = *argptr++) != NULL) {
		if ((p = strchr(name, '=')) != NULL) {
			p++;
		} else {
			vp = find_var(name, NULL, NULL);
			if (vp != NULL) {
				vp->flags |= flag;
				continue;
			}
		}
		setvar(name, p, flag);
	}
	return 0;
}



int
localcmd(int argc, char **argv)
{
	char *name;

	if (! in_function())
		error("Not in a function");
	while ((name = *argptr++) != NULL) {
		mklocal(name, 0);
	}
	return 0;
}



void
mklocal(const char *name, int flags)
{
	struct localvar *lvp;
	struct var **vpp;
	struct var *vp;

	INTOFF;
	lvp = ckmalloc(sizeof (struct localvar));
	if (name[0] == '-' && name[1] == '\0') {
		char *p;
		p = ckmalloc(sizeof_optlist);
		lvp->text = memcpy(p, optlist, sizeof_optlist);
		vp = NULL;
	} else {
		vp = find_var(name, &vpp, NULL);
		if (vp == NULL) {
			if (strchr(name, '='))
				setvareq(savestr(name), VSTRFIXED|flags);
			else
				setvar(name, NULL, VSTRFIXED|flags);
			vp = *vpp;	/* the new variable */
			lvp->text = NULL;
			lvp->flags = VUNSET;
		} else {
			lvp->text = vp->text;
			lvp->flags = vp->flags;
			vp->flags |= VSTRFIXED|VTEXTFIXED;
			if (name[vp->name_len] == '=')
				setvareq(savestr(name), flags);
		}
	}
	lvp->vp = vp;
	lvp->next = localvars;
	localvars = lvp;
	INTON;
}



void
poplocalvars(void)
{
	struct localvar *lvp;
	struct var *vp;

	while ((lvp = localvars) != NULL) {
		localvars = lvp->next;
		vp = lvp->vp;
		TRACE(("poplocalvar %s", vp ? vp->text : "-"));
		if (vp == NULL) {	/* $- saved */
			memcpy(optlist, lvp->text, sizeof_optlist);
			ckfree(lvp->text);
		} else if ((lvp->flags & (VUNSET|VSTRFIXED)) == VUNSET) {
			(void)unsetvar(vp->text, 0);
		} else {
			if (vp->func && (vp->flags & VNOFUNC) == 0)
				(*vp->func)(lvp->text + vp->name_len + 1);
			if ((vp->flags & VTEXTFIXED) == 0)
				ckfree(vp->text);
			vp->flags = lvp->flags;
			vp->text = lvp->text;
		}
		ckfree(lvp);
	}
}


int
setvarcmd(int argc, char **argv)
{
	if (argc <= 2)
		return unsetcmd(argc, argv);
	else if (argc == 3)
		setvar(argv[1], argv[2], 0);
	else
		error("List assignment not implemented");
	return 0;
}



int
unsetcmd(int argc, char **argv)
{
	char **ap;
	int i;
	int flg_func = 0;
	int flg_var = 0;
	int ret = 0;

	while ((i = nextopt("evf")) != '\0') {
		if (i == 'f')
			flg_func = 1;
		else
			flg_var = i;
	}
	if (flg_func == 0 && flg_var == 0)
		flg_var = 1;

	for (ap = argptr; *ap ; ap++) {
		if (flg_func)
			ret |= unsetfunc(*ap);
		if (flg_var)
			ret |= unsetvar(*ap, flg_var == 'e');
	}
	return ret;
}



int
unsetvar(const char *s, int unexport)
{
	struct var **vpp;
	struct var *vp;

	vp = find_var(s, &vpp, NULL);
	if (vp == NULL)
		return 1;

	if (vp->flags & VREADONLY)
		return (1);

	INTOFF;
	if (unexport) {
		vp->flags &= ~VEXPORT;
	} else {
		if (vp->text[vp->name_len + 1] != '\0')
			setvar(s, nullstr, 0);
		vp->flags &= ~VEXPORT;
		vp->flags |= VUNSET;
		if ((vp->flags & VSTRFIXED) == 0) {
			if ((vp->flags & VTEXTFIXED) == 0)
				ckfree(vp->text);
			*vpp = vp->next;
			ckfree(vp);
		}
	}
	INTON;
	return 0;
}



STATIC int
strequal(const char *p, const char *q)
{
	while (*p == *q++) {
		if (*p++ == '=')
			return 1;
	}
	if (*p == '=' && *(q - 1) == '\0')
		return 1;
	return 0;
}


STATIC struct var *
find_var(const char *name, struct var ***vppp, int *lenp)
{
	unsigned int hashval;
	int len;
	struct var *vp, **vpp;
	const char *p = name;

	hashval = 0;
	while (*p && *p != '=')
		hashval = 2 * hashval + (unsigned char)*p++;
	len = p - name;

	if (lenp)
		*lenp = len;
	vpp = &vartab[hashval % VTABSIZE];
	if (vppp)
		*vppp = vpp;

	for (vp = *vpp ; vp ; vpp = &vp->next, vp = *vpp) {
		if (vp->name_len != len)
			continue;
		if (memcmp(vp->text, name, len) != 0)
			continue;
		if (vppp)
			*vppp = vpp;
		return vp;
	}
	return NULL;
}
