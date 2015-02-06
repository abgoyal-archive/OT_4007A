#ifndef CUST_BT_H
#define CUST_BT_H

#if defined(MTK_MT6620) || defined(MTK_MT6628)
/* SERIAL PORT */
#define CUST_BT_SERIAL_PORT "/dev/stpbt"
/* BAUDRATE */
#define CUST_BT_BAUD_RATE   4000000 /*use 4M but is not controlled by bt directly */
#else
/* SERIAL PORT */
#define CUST_BT_SERIAL_PORT "/dev/ttyMT2"
/* BAUDRATE */
#define CUST_BT_BAUD_RATE   3250000
//#define CUST_BT_BAUD_RATE   921600
#endif


/* MTK BT SOLUTION */
//#define MTK_MT6611_E4
//#define MTK_MT6611_E4
//#define MTK_MT6616_E3
#endif /* CUST_BT_H */
