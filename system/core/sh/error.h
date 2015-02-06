
/*	$NetBSD: error.h,v 1.16 2003/08/07 09:05:30 agc Exp $	*/


#include <stdarg.h>


#define E_OPEN 01	/* opening a file */
#define E_CREAT 02	/* creating a file */
#define E_EXEC 04	/* executing a program */



#include <setjmp.h>

struct jmploc {
	jmp_buf loc;
};

extern struct jmploc *handler;
extern int exception;
extern int exerrno;	/* error for EXEXEC */

/* exceptions */
#define EXINT 0		/* SIGINT received */
#define EXERROR 1	/* a generic error */
#define EXSHELLPROC 2	/* execute a shell procedure */
#define EXEXEC 3	/* command execution failed */



extern volatile int suppressint;
extern volatile int intpending;

#define INTOFF suppressint++
#define INTON { if (--suppressint == 0 && intpending) onint(); }
#define FORCEINTON {suppressint = 0; if (intpending) onint();}
#define CLEAR_PENDING_INT intpending = 0
#define int_pending() intpending

void exraise(int) __attribute__((__noreturn__));
void onint(void);
void error(const char *, ...) __attribute__((__noreturn__));
void exerror(int, const char *, ...) __attribute__((__noreturn__));
const char *errmsg(int, int);

void sh_err(int, const char *, ...) __attribute__((__noreturn__));
void sh_verr(int, const char *, va_list) __attribute__((__noreturn__));
void sh_errx(int, const char *, ...) __attribute__((__noreturn__));
void sh_verrx(int, const char *, va_list) __attribute__((__noreturn__));
void sh_warn(const char *, ...);
void sh_vwarn(const char *, va_list);
void sh_warnx(const char *, ...);
void sh_vwarnx(const char *, va_list);

void sh_exit(int) __attribute__((__noreturn__));



#if defined(BSD) && !defined(__SVR4) && !defined(__linux__)
#define setjmp(jmploc)	_setjmp(jmploc)
#define longjmp(jmploc, val)	_longjmp(jmploc, val)
#endif
