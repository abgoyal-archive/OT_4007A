

#ifndef __BLUEDROID_BLUETOOTH_H__
#define __BLUEDROID_BLUETOOTH_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <bluetooth/bluetooth.h>

int bt_enable();

int bt_disable();

/* Returns 1 if enabled, 0 if disabled, and -ve on error */
int bt_is_enabled();

int ba2str(const bdaddr_t *ba, char *str);
int str2ba(const char *str, bdaddr_t *ba);

#ifdef __cplusplus
}
#endif
#endif //__BLUEDROID_BLUETOOTH_H__
