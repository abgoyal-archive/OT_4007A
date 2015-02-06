
/*	$NetBSD: output.h,v 1.17 2003/08/07 09:05:36 agc Exp $	*/


#ifndef OUTPUT_INCL

#include <stdarg.h>

struct output {
	char *nextc;
	int nleft;
	char *buf;
	int bufsize;
	short fd;
	short flags;
};

extern struct output output;
extern struct output errout;
extern struct output memout;
extern struct output *out1;
extern struct output *out2;

void open_mem(char *, int, struct output *);
void out1str(const char *);
void out2str(const char *);
void outstr(const char *, struct output *);
void emptyoutbuf(struct output *);
void flushall(void);
void flushout(struct output *);
void freestdout(void);
void outfmt(struct output *, const char *, ...)
    __attribute__((__format__(__printf__,2,3)));
void out1fmt(const char *, ...)
    __attribute__((__format__(__printf__,1,2)));
void dprintf(const char *, ...)
    __attribute__((__format__(__printf__,1,2)));
void fmtstr(char *, size_t, const char *, ...)
    __attribute__((__format__(__printf__,3,4)));
void doformat(struct output *, const char *, va_list);
int xwrite(int, char *, int);
int xioctl(int, unsigned long, char *);

#define outc(c, file)	(--(file)->nleft < 0? (emptyoutbuf(file), *(file)->nextc++ = (c)) : (*(file)->nextc++ = (c)))
#define out1c(c)	outc(c, out1);
#define out2c(c)	outc(c, out2);

#define OUTPUT_INCL
#endif
