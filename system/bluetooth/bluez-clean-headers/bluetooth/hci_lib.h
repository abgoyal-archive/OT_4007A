
#ifndef __HCI_LIB_H
#define __HCI_LIB_H

#ifdef __cplusplus
#endif
#ifdef __cplusplus
#endif
static inline int hci_test_bit(int nr, void *addr)
{
	return *((uint32_t *) addr + (nr >> 5)) & (1 << (nr & 31));
}
#endif
