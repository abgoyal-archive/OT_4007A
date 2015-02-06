#ifndef _MT6575_BOOT_MODE_H_
#define _MT6575_BOOT_MODE_H_

/* MT6575 boot type definitions */
typedef enum 
{
    NORMAL_BOOT = 0,
    META_BOOT = 1,
    RECOVERY_BOOT = 2,    
    SW_REBOOT = 3,
    FACTORY_BOOT = 4,
    ADVMETA_BOOT = 5,
    ATE_FACTORY_BOOT = 6,
    ALARM_BOOT = 7,
    UNKNOWN_BOOT
} BOOTMODE;

typedef enum {
    BR_POWER_KEY = 0,
    BR_USB,
    BR_RTC,
    BR_WDT,
    BR_WDT_BY_PASS_PWK,
    BR_UNKNOWN
} boot_reason_t;

/*META COM port type*/
 typedef enum
{
    META_UNKNOWN_COM = 0,
    META_UART_COM,
    META_USB_COM
} META_COM_TYPE;

typedef struct {
  unsigned int maggic_number;
  BOOTMODE boot_mode;
  unsigned int  e_flag;
  unsigned int  log_port;
  unsigned int  log_baudrate;
  unsigned char log_enable;
  unsigned char reserved[3];
  unsigned int dram_rank_num;
  unsigned int dram_rank_size;
  unsigned int boot_reason;
  META_COM_TYPE meta_com_type;
  unsigned int meta_com_id;
   //PR330845 yongming.li  for show version added  begin
  unsigned char version[9];
   //PR330845 yongming.li  for show version added  end
} BOOT_ARGUMENT;


/* MT6575 chip definitions */
typedef enum 
{
    CHIP_E1 = 0xca00,
    CHIP_E2 = 0xcb00,
    //CHIP_E3 = 0x8a02, //TBD   
} CHIP_VER;

#define BOOT_ARGUMENT_MAGIC 0x504c504c

extern unsigned int BOOT_ARGUMENT_LOCATION;
extern void boot_mode_select(void);
extern BOOTMODE g_boot_mode;
extern CHIP_VER get_chip_eco_ver(void);

#endif
