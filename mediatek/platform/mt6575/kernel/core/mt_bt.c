
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

#include <mach/mt_bt.h>

#ifdef MTK_BT_SUPPORT
struct hci_dev* btpm_hdev = NULL;
wait_queue_head_t eint_wait;
int eint_gen;

void mt_bt_eirq_handler(void* par)
{
    struct hci_dev* hdev = NULL;

    printk(KERN_ERR "[BT]mt_bt_eirq_handler\r\n ");

    if (btpm_hdev != NULL){  /*to avoid the early interrupt before we register the btpm_hdev*/
        //hci_send_cmd(btpm_hdev, 0xfcc1, 0, NULL);
    #ifdef CONFIG_BT_HCIUART
        hdev = hci_dev_get(0);
        if(hdev == NULL){
            printk(KERN_ERR "[BT]NULL dev\r\n ");
        }else{
            printk(KERN_ERR "[BT] set mt_bt_eirq_handler send cmd\r\n ");
            hci_send_cmd(hdev, 0xfcc1, 0, NULL);
            /* mt_bt_eirq_handler in invoked in request_irq. Should not invoke task let scheduler?? */
        }
    #endif
    }
    else  //for ESI stack
    {
        eint_gen = 1;
        wake_up_interruptible(&eint_wait);
    }
    
    return;
}

void mt_bt_pm_init(void* hdev)
{
    printk(KERN_ERR "[BT]mt_bt_pm_init\r\n ");
    btpm_hdev = (struct hci_dev*)hdev;
    //mt_bt_enable_irq() is enabled when HCI0 is up
    return;
}

void mt_bt_pm_deinit(void* hdev)
{
    printk(KERN_ERR "[BT]mt_bt_pm_deinit\r\n ");
    //mt_bt_enable_irq() is disabled when HCI0 is down
    btpm_hdev = NULL;
    return;
}
#else
void mt_bt_eirq_handler(void* par)
{
     //Do nothing
}
#endif
