#ifndef _WD_KICKER_H_
#define _WD_KICKER_H_

enum wk_wdt_mode {
	WK_WDT_NORMAL_MODE,
	WK_WDT_EXP_MODE
};

struct wk_wdt {
	int (*config)(enum wk_wdt_mode, int timeout);
	void (*kick_wdt)(void);
};

int wk_register_wdt(struct wk_wdt *wk_wdt);

#endif
