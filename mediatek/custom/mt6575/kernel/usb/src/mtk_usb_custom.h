#ifndef MTK_USB_CUSTOM_H
#define MTK_USB_CUSTOM_H

/* Force full-speed is not guaranteed with adb function, use it with care! */
//#define USB_FORCE_FULL_SPEED

/* Force full-speed When enter META/ATE_FACTORY mode*/
//#define USB_META_FULL_SPEED

//modified by songlinhua 2012-10-31, bug#: 346589, upadate PID,VID, model name

/*
 * modified by songlinhua 2012-12-26 begin:
 * follow diablo to unify MTK plaform PID/VID
 */
#define USB_MS_PRODUCT_ID           0x0166
#define USB_MS_ADB_PRODUCT_ID       0x0165	
#define USB_RNDIS_PRODUCT_ID        0x0174
#define USB_RNDIS_ADB_PRODUCT_ID    0x0173
#define USB_MS_ADB_ACM_PRODUCT_ID   0x4003
#define USB_ACM_PRODUCT_ID          0x0006
/*modified by songlinhua 2012-12-26 end */

#define VENDOR_ID      0x1bbb                /* USB vendor id  */
#define PRODUCT_ID     USB_MS_PRODUCT_ID     /* USB default product id */

/* for META/ATE_FACTORY mode */
#define SINGLE_ACM_VENDOR_ID  0x0E8D
#define SINGLE_ACM_PRODUCT_ID 0x2007

#define MANUFACTURER_STRING "Alcatel"
#define PRODUCT_STRING      "Alcatel Android Phone"

#define USB_ETH_VENDORID     0
#define USB_ETH_VENDORDESCR  "Alcatel"

#define USB_MS_VENDOR        "Alcatel"
#define USB_MS_PRODUCT       "Alcatel MS"
#define USB_MS_RELEASE       0x0100

#define CHR_TYPE_DETECT_DEB  400  /* debounce time for charger type detection, in ms */

#endif /* MTK_USB_CUSTOM_H */
