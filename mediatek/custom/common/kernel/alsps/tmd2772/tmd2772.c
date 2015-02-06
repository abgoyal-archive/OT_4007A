

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>
#ifdef MT6516
#include <mach/mt6516_devs.h>
#include <mach/mt6516_typedefs.h>
#include <mach/mt6516_gpio.h>
#include <mach/mt6516_pll.h>
#endif

#ifdef MT6573
#include <mach/mt6573_devs.h>
#include <mach/mt6573_typedefs.h>
#include <mach/mt6573_gpio.h>
#include <mach/mt6573_pll.h>
#endif

#ifdef MT6575
#include <mach/mt6575_devs.h>
#include <mach/mt6575_typedefs.h>
#include <mach/mt6575_gpio.h>
#include <mach/mt6575_pm_ldo.h>
#endif

#ifdef MT6516
#define POWER_NONE_MACRO MT6516_POWER_NONE
#endif

#ifdef MT6573
#define POWER_NONE_MACRO MT65XX_POWER_NONE
#endif

#ifdef MT6575
#define POWER_NONE_MACRO MT65XX_POWER_NONE
#endif

#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <asm/io.h>
#include <cust_eint.h>
#include <cust_alsps.h>
static struct tmd2771_priv *tmd2771_obj = NULL;
static struct platform_driver tmd2771_alsps_driver;
/*----------------------------------------------------------------------------*/

#define TMD2771_DEV_NAME     "TMD2771"
/*----------------------------------------------------------------------------*/
#define APS_TAG                  "[ALS/PS] "
#define APS_FUN(f)               printk(KERN_INFO APS_TAG"%s\n", __FUNCTION__)
/********************************************/
 
extern   int mstar2133_pls_enable(void);
 
extern  int mstar2133_pls_disable(void);

extern  int  get_msg2133_data(void );

extern int ft6306_pls_enable(void);
 
extern int ft6306_pls_disable(void);

extern int  get_ft6306_data(void );

int tp_pls_status=-1;

static int pls_enable(void)
{
	if(tp_pls_status==0) //tp is msg2133;
		return mstar2133_pls_enable();
	else if(tp_pls_status==1)//tp is ft6306;
		return ft6306_pls_enable();
	else
		return -1;
}

static int pls_disable(void)
{
	if(tp_pls_status==0) //tp is msg2133;
		return mstar2133_pls_disable();
	else if(tp_pls_status==1)//tp is ft6306;
		return ft6306_pls_disable();
	else
		return -1;
}

static int get_data(void)
{
	int alsps_value=-1;
	if(tp_pls_status==0) //tp is msg2133;
		alsps_value= get_msg2133_data();
	if(tp_pls_status==1)//tp is ft6306;
		alsps_value= get_ft6306_data();
	if(alsps_value<0)
		return 1 ;//1 is far;
	else
		return alsps_value;
}
/********************************************/
struct tmd2771_priv {
    struct alsps_hw  *hw;
    struct i2c_client *client;
    struct work_struct  eint_work;

    /*i2c address group*/
  //  struct tmd2771_i2c_addr  addr;
    
    /*misc*/
    u16		    als_modulus;
    atomic_t    i2c_retry;
    atomic_t    als_suspend;
    atomic_t    als_debounce;   /*debounce time after enabling als*/
    atomic_t    als_deb_on;     /*indicates if the debounce is on*/
    atomic_t    als_deb_end;    /*the jiffies representing the end of debounce*/
    atomic_t    ps_mask;        /*mask ps: always return far away*/
    atomic_t    ps_debounce;    /*debounce time after enabling ps*/
    atomic_t    ps_deb_on;      /*indicates if the debounce is on*/
    atomic_t    ps_deb_end;     /*the jiffies representing the end of debounce*/
    atomic_t    ps_suspend;


    /*data*/
    u16         als;
    u16          ps;
    u8          _align;
    u16         als_level_num;
    u16         als_value_num;
    u32         als_level[C_CUST_ALS_LEVEL-1];
    u32         als_value[C_CUST_ALS_LEVEL];

    atomic_t    als_cmd_val;    /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_cmd_val;     /*the cmd value can't be read, stored in ram*/
    atomic_t    ps_thd_val_high;     /*the cmd value can't be read, stored in ram*/
	atomic_t    ps_thd_val_low;     /*the cmd value can't be read, stored in ram*/
    ulong       enable;         /*enable mask*/
    ulong       pending_intr;   /*pending interrupt*/

    /*early suspend*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend    early_drv;
#endif     
};

typedef enum {
    CMC_BIT_ALS    = 1,
    CMC_BIT_PS     = 2,
} CMC_BIT;

int tmd2771_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
		void* buff_out, int size_out, int* actualout)
{
	int err=0;
		int value;
	hwm_sensor_data* sensor_data;
	struct tmd2771_priv *obj = (struct tmd2771_priv *)self;
	switch (command)
	{
		case SENSOR_DELAY:
					printk("[FT6306_PS] SENSOR_DELAY  \n");

			break;

		case SENSOR_ENABLE:
			printk("[FT6306_PS] SENSOR_ENABLE  \n");

			if((buff_in == NULL) || (size_in < sizeof(int)))
			{
				printk("Enable sensor parameter error!\n");
				err = -EINVAL;
			}
			else
			{		
				value = *(int *)buff_in;
				if(value)
				{
					if(err != pls_enable())
					{
						printk("enable ps fail: %d\n", err); 
						return -1;
					}
					set_bit(CMC_BIT_PS, &obj->enable);

				}
				else
				{
					if(err != pls_disable())
					{
						printk("disable ps fail: %d\n", err); 
						return -1;
					}
					clear_bit(CMC_BIT_PS, &obj->enable);

				}
			}
			break;

		case SENSOR_GET_DATA:
			//printk("[FT6306_PS] SENSOR_GET_DATA  \n");
			if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
			{
				printk("get sensor data parameter error!\n");
				err = -EINVAL;
			}
			else
			{
				sensor_data = (hwm_sensor_data *)buff_out;	
                                //mdelay(160);
				//printk("[FT6306_PS] tmd2772_ps_operate ps data=%d!\n",get_data());
				sensor_data->values[0] = get_data();
				sensor_data->value_divide = 1;
				sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;			
			}
			
			break;
		default:
			
			break;
	}
	
	return 0;
}
/*----------------------------------------------------------------------------*/
static int tmd2771_probe(struct platform_device *pdev) 
{	
	struct tmd2771_priv *obj;
	struct hwmsen_object obj_ps, obj_als;
	int err = 0;

	if(!(obj = kzalloc(sizeof(*obj), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(*obj));
	tmd2771_obj = obj;

	obj_ps.self = tmd2771_obj;
	/*for interrup work mode support -- by liaoxl.lenovo 12.08.2011*/
	
	obj_ps.polling = 1;
	


	obj_ps.sensor_operate = tmd2771_ps_operate;
	if(err = hwmsen_attach(ID_PROXIMITY, &obj_ps))
	{
		printk("attach fail = %d\n", err);
		goto exit;
	}
		printk("wwwlllllll7");

	return 0;

exit :
	return -1;
}
/*----------------------------------------------------------------------------*/
static int tmd2771_remove(struct platform_device *pdev)
{
	APS_FUN();    
	return 0;
}
/*----------------------------------------------------------------------------*/
static struct platform_driver tmd2771_alsps_driver = {
	.probe      = tmd2771_probe,
	.remove     = tmd2771_remove,    
	.driver     = {
		.name  = "als_ps",
//		.owner = THIS_MODULE,
	}
};
/*----------------------------------------------------------------------------*/
static int __init tmd2771_init(void)
{
	APS_FUN();
	//i2c_register_board_info(0, &i2c_TMD2771, 1);
	if(platform_driver_register(&tmd2771_alsps_driver))
	{
		printk("failed to register driver");
		return -ENODEV;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static void __exit tmd2771_exit(void)
{
	APS_FUN();
	platform_driver_unregister(&tmd2771_alsps_driver);
}
/*----------------------------------------------------------------------------*/
module_init(tmd2771_init);
module_exit(tmd2771_exit);
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("wenli.wang");
MODULE_DESCRIPTION("tmd2771 driver");
MODULE_LICENSE("GPL");
