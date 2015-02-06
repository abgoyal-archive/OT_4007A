#if !defined(_XLOG_INTERNAL_H)
#define _XLOG_INTERNAL_H

int xLog_isOn(const char *name, int level);

void xLog_set(const char *name, int level, int status);

#endif
