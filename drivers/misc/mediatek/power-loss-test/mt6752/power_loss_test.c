#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <generated/autoconf.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/processor.h>
#include <asm/uaccess.h>
#include <linux/delay.h>  
#include <linux/wakelock.h>
#include <mach/power_loss_test.h>


#include <linux/seq_file.h>

#define TAG                 "[MVG_TEST]:"
#define PWR_LOSS_MT6571


#if defined(PWR_LOSS_SPOH)
static struct wake_lock	spoh_wake_lock;
#endif

#if defined(PWR_LOSS_SPOH) || defined(PWR_LOSS_SW_RESET)
#define PWR_LOSS_PROC
#endif

#ifdef CONFIG_PWR_LOSS_MTK_DEBUG
#define PWR_LOSS_DEBUG
#endif


#define PWR_LOSS_DEVNAME            "power_loss_test"  
#define PWR_LOSS_FIRST_MINOR         0
#define PWR_LOSS_MAX_MINOR_COUNT     1
#define PWR_LOSS_CBUF_LEN            128

#ifndef HZ
#define HZ	100
#endif

#ifdef PWR_LOSS_SW_RESET
    #ifdef PWR_LOSS_RANDOM_SW_RESET
        #define PWR_LOSS_SLEEP_MAX_TIME      (8000)
    #else
        #define PWR_LOSS_SLEEP_TIME          (6000)    
    #endif 
#endif 

#ifdef PWR_LOSS_MT6575
extern void wdt_arch_reset(char mode);
#elif defined PWR_LOSS_MT6573
    #include "../../../core/include/mach/mt6573_reg_base.h"
    #include "../../../core/include/mach/mt6573_typedefs.h"
    #define PWR_LOSS_WDT_MODE               (AP_RGU_BASE+0)
    #define PWR_LOSS_WDT_LENGTH             (AP_RGU_BASE+4)
    #define PWR_LOSS_WDT_RESTART            (AP_RGU_BASE+8)
#elif defined PWR_LOSS_MT6571
    #include <mach/mt_reg_base.h>
    extern void wdt_arch_reset(char mode);
    #define WDT_NON_RST_REG                 (TOPRGU_BASE+0x410)
    #define WDT_NON_RST_REG2                (TOPRGU_BASE+0x414)
    #define PWR_LOSS_MAGIC_PATTERN          0x4C50      
    #define PWR_LOSS_MAGIC(a)               (((a)&0xFFFF0000)>>16)
    #define PWR_LOSS_COUNTER(a)             ((a)&0x0000FFFF)
#endif

#include <mach/mt_wdt.h>
#include <mach/mt_typedefs.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
    struct device_node *mvg_toprgu_node = NULL;
    void __iomem *mvg_toprgu_base;
#endif

#ifdef WDT_NON_RST_REG2
int mvg_get_rgu_counter(void)
{
    int result;
    volatile unsigned int *reg = (unsigned int *)WDT_NON_RST_REG2;

    if (PWR_LOSS_MAGIC(*reg) == PWR_LOSS_MAGIC_PATTERN)
        result = PWR_LOSS_COUNTER(*reg);
    else
        result = -1;

    printk(KERN_ERR "%s Power Loss Test: mvg_get_rgu_counter() %X !\n", TAG, *reg);

    return result;
}
int mvg_set_rgu_counter(int val)
{
    unsigned int i;

    volatile unsigned int *reg = (unsigned int *)WDT_NON_RST_REG2;

    i = (PWR_LOSS_MAGIC_PATTERN << 16) | PWR_LOSS_COUNTER((unsigned int)val);
    *reg = i;

    printk(KERN_ERR "%s Power Loss Test: mvg_set_rgu_counter() %X !\n", TAG, i);

    mvg_set_counter(val);

    return val;
}
#else 
int mvg_get_rgu_counter(void)
{
    return mvg_get_counter();
}
int mvg_set_rgu_counter(int val)
{
    mvg_set_counter(val);
    return -1;
}
#endif

static dev_t sg_pwr_loss_devno;
static struct cdev*   sg_pwr_loss_dev       = NULL;
static struct class*  sg_pwr_loss_dev_class = NULL;
static struct device* sg_pwr_loss_dev_file  = NULL;
#ifdef PWR_LOSS_PROC
static wdt_reboot_info power_loss_info;
static char cmd_buf[256];
#endif


int pwr_loss_open(struct inode *inode, struct file *file)
{
#ifdef PWR_LOSS_DEBUG
    printk(KERN_NOTICE "%s Power Loss Test: Open operation !\n", TAG);
#endif
    return 0;
}

int pwr_loss_release(struct inode *inode, struct file *file)
{
#ifdef PWR_LOSS_DEBUG
    printk(KERN_NOTICE "%s Power Loss Test: Release operation !\n", TAG);
#endif
    return 0;
}

long pwr_loss_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    char l_buf[PWR_LOSS_CBUF_LEN] = {0};
    struct mvg_addr addr_entry;

#ifdef PWR_LOSS_DEBUG
#endif

    switch (cmd){
        case PRINT_REBOOT_TIMES:
            ret  = copy_from_user((int *)l_buf, (int *)arg, sizeof(int));
            if (ret != 0){
                printk(KERN_ERR "%s Power Loss Test: IOCTL->PRINT_REBOOT_TIMES %d Bytes can't be copied \n", TAG, ret);
            }
            printk(KERN_ERR "%s Power Loss Test: -----------System Reboot Successfully Times= %d---------------!\n", TAG, ((int *)l_buf)[0]);
            break;
        case PRINT_DATA_COMPARE_ERR:
            printk(KERN_ERR "%s Power Loss Test: -----------Data Compare Error---------------!\n", TAG);
            break;
        case PRINT_FILE_OPERATION_ERR:
            printk(KERN_ERR "%s Power Loss Test: -----------File Operation Error---------------!\n", TAG);
            break;
        case PRINT_GENERAL_INFO:
            ret  = copy_from_user(l_buf,(int *)arg,(sizeof(l_buf[0])*(sizeof(l_buf))));
            if (ret != 0){
                printk(KERN_ERR "%s Power Loss Test: IOCTL->PRINT_REBOOT_TIMES %d Bytes can't be copied \n", TAG, ret);
            }

            l_buf[(sizeof(l_buf[0])*(sizeof(l_buf)))-1] = '\0';

#ifdef PWR_LOSS_DEBUG
            printk(KERN_WARNING "%s %s", TAG, l_buf);
#endif
            break;
        case PRINT_RAW_RW_INFO:
            ret  = copy_from_user(l_buf,(int *)arg,(sizeof(l_buf[0])*(sizeof(l_buf))));
            if (ret != 0){
                printk(KERN_ERR "%s Power Loss Test: %d Bytes can't be copied \n", TAG, ret);
            }

            l_buf[(sizeof(l_buf[0])*(sizeof(l_buf)))-1] = '\0';

#ifdef PWR_LOSS_DEBUG
            printk(KERN_WARNING "%s %s\n", TAG, l_buf);
#endif
            break;

        
        
        
        case MVG_WDT_CONTROL:
            ret=(int)arg;
            printk(KERN_ERR "%s::IOCTL->MVG_WDT_CONTROL: %d\n",TAG, ret);

            switch(ret) {
                case MVG_WDT_DISABLE:
                case MVG_WDT_ENABLE:
                    mvg_set_wdt(arg);
                    break;
                case MVG_WDT_RESET_NOW:
                    mvg_wdt_reset();
                default:
                    break;
            }
            break;
#ifdef CONFIG_MTK_MTD_NAND
        case MVG_NAND_WDT_CONTROL:
            ret=(int)arg;
            printk(KERN_ERR "%s::IOCTL->MVG_NAND_WDT_CONTROL: %d\n",TAG, ret);

            switch(ret) {
                case MVG_WDT_PROGRAM_DISABLE:
                case MVG_WDT_PROGRAM_ENABLE:
                    mvg_nand_set_program_wdt(arg);
                    break;
                case MVG_WDT_ERASE_DISABLE:
                case MVG_WDT_ERASE_ENABLE:
                    mvg_nand_set_erase_wdt(arg-2);
                default:
                    break;
            }
            break;
#endif
        case MVG_COUNTER_SET:
            mvg_set_rgu_counter(arg);
            break;

        case MVG_COUNTER_GET:
            ret = mvg_get_rgu_counter();
            break;

        case MVG_ADDR_RANGE_ADD:
            ret  = copy_from_user(&addr_entry, (int *)arg, sizeof(struct mvg_addr));
            if (ret != 0){
                printk(KERN_ERR "%s: IOCTL->MVG_ADDR_RANGE_ADD address can't be copied %d\n", TAG, ret);
                break;
            }
            mvg_addr_range_add(addr_entry.base, addr_entry.end);
            break;

        case MVG_ADDR_RANGE_DELETE:
            mvg_addr_range_del((u64)arg);
            break;

        case MVG_ADDR_RANGE_CHECK:
            ret=mvg_addr_range_check((u64)arg);
            break;

        case MVG_ADDR_RANGE_CLEAR:
            mvg_addr_range_clear();
            break;

        case MVG_SET_CURR_CASE_NAME:
            ret  = copy_from_user(l_buf,(int *)arg,(sizeof(l_buf[0])*(sizeof(l_buf))));
            if (ret != 0){
                printk(KERN_ERR "%s:IOCTL->MVG_SET_CURR_CASE_NAME can't be copied %d\n", TAG, ret);
                break;
            }
            l_buf[(sizeof(l_buf[0])*(sizeof(l_buf)))-1] = '\0';
            mvg_set_curr_case_name(l_buf);
            break;

        case MVG_SET_CURR_GROUP_NAME:
            ret  = copy_from_user(l_buf,(int *)arg,(sizeof(l_buf[0])*(sizeof(l_buf))));
            if (ret != 0){
                printk(KERN_ERR "%s:IOCTL->MVG_SET_CURR_CASE_NAME can't be copied %d\n", TAG, ret);
                break;
            }
            l_buf[(sizeof(l_buf[0])*(sizeof(l_buf)))-1] = '\0';
            mvg_set_curr_group_name(l_buf);
            break;

        case MVG_SET_TRIGGER:
            mvg_set_trigger(arg);
            break;

        case MVG_METHOD_SET:
            mvg_set_method(arg);
            break;

        case MVG_METHOD_GET:
            ret = mvg_get_method();
            break;

#ifdef CONFIG_MTK_EMMC_SUPPORT
        case MVG_EMMC_TIME_DIVISOR_SET:
            mvg_emmc_set_reset_time_divisor(&power_loss_info, arg);
            break;

        case MVG_EMMC_TIME_DIVISOR_GET:
            ret = mvg_emmc_get_reset_time_divisor(&power_loss_info);
            break;

        case MVG_EMMC_RESET_MODE_SET:
            mvg_emmc_reset_mode_set(&power_loss_info, arg);
            break;

        case MVG_EMMC_RESET_TIME_MODE_SET:
            mvg_emmc_reset_time_mode_set(&power_loss_info, arg);
            break;

        case MVG_EMMC_SET_ERASE_GROUP_SIZE:
            mvg_emmc_set_erase_group_sector(&power_loss_info, arg);
            break;

        case MVG_EMMC_GET_DELAY_RESULT:
            ret = mvg_emmc_get_delay_result(&power_loss_info);
            break;

        case MVG_EMMC_SET_DELAY_TABLE:
            ret = mvg_emmc_get_set_delay_table(&power_loss_info, (unsigned char *)arg);
            break;
#endif

        default:
            ret=-1;
            printk(KERN_ERR "%s Power Loss Test: cmd code Error! %d\n", TAG, cmd);
            break;
    }

    return ret;
}

extern void get_random_bytes(void *buf, int nbytes);
static struct file_operations pwr_loss_fops =
{
    .owner = THIS_MODULE,
    .open = pwr_loss_open,
    .release = pwr_loss_release,
    .unlocked_ioctl	= pwr_loss_ioctl,
};

#ifdef PWR_LOSS_SW_RESET
#if PWR_LOSS_RANDOM_SW_RESET
int pwr_loss_reset_thread(void *p)
{
    signed long ret = 0;
    int HZ_val = HZ;
    struct timespec current_time;
    long sec_time = 0;
    long nsec_time = 0;
    signed long sleep_time = 0;

#ifdef PWR_LOSS_MT6573
    volatile unsigned short *Reg1 = (unsigned short *)PWR_LOSS_WDT_MODE;
    volatile unsigned short *Reg2 = (unsigned short *)PWR_LOSS_WDT_LENGTH;
    volatile unsigned short *Reg3 = (unsigned short *)PWR_LOSS_WDT_RESTART;
#endif

    get_random_bytes(&sleep_time, sizeof(signed long));
    if (sleep_time < 0)
        sleep_time &= 0x7fffffff;
    sleep_time %= PWR_LOSS_SLEEP_MAX_TIME;

#ifdef PWR_LOSS_DEBUG
    printk(KERN_NOTICE "%s Power Loss Test: sleep time =%d\n", TAG, sleep_time);
#endif

    while (1){
        printk(KERN_WARNING "%s Power Loss Test: wait for reset...!\n", TAG);
        set_current_state(TASK_UNINTERRUPTIBLE);
        ret = schedule_timeout(sleep_time);
        down_read(&power_loss_info.rwsem);
        if(power_loss_info.wdt_reboot_support == WDT_REBOOT_OFF) {
            up_read(&power_loss_info.rwsem);
            msleep(1000);
            printk(KERN_WARNING "%s Power Loss Test: wdt reboot pause...!\n", TAG);
            continue;
        }
        up_read(&power_loss_info.rwsem);
        printk(KERN_ERR "%s Power Loss Test: ret = %d, do reset now...\n", TAG, ret);

#ifdef PWR_LOSS_MT6575
    #ifdef CONFIG_MTK_MTD_NAND
    #endif
        wdt_arch_reset(0xff);
#elif defined PWR_LOSS_MT6573
    #ifdef CONFIG_MTK_MTD_NAND
        if(!mt6573_nandchip_Reset()){
            printk(KERN_ERR "%s NAND_MVG mt6573_nandchip_Reset Failed!\n", TAG);
        }
    #endif

        
        *Reg1 = 0x2200;
        *Reg2 = (0x3F<<5)|0x8;
        *Reg3 = 0x1971;
        *Reg1 = 0x2217;
#endif
        while(1);
    }
}
#else
int pwr_loss_reset_thread(void *p)
{
    signed long ret = 0;
    signed long l_val1 = 0;
    signed long l_val2 = 0;
    signed long l_count = 0;

#ifdef PWR_LOSS_MT6573
    volatile unsigned short *Reg1 = (unsigned short *)PWR_LOSS_WDT_MODE;
    volatile unsigned short *Reg2 = (unsigned short *)PWR_LOSS_WDT_LENGTH;
    volatile unsigned short *Reg3 = (unsigned short *)PWR_LOSS_WDT_RESTART;
#endif

#ifdef PWR_LOSS_DEBUG
    printk(KERN_NOTICE "%s Power Loss Test: sleep time = 100sec\n", TAG);
#endif

    while (1){
        printk(KERN_WARNING "%s Power Loss Test: wait for reset...!\n", TAG);
        set_current_state(TASK_UNINTERRUPTIBLE);
        ret = schedule_timeout(PWR_LOSS_SLEEP_TIME);
        down_read(&power_loss_info.rwsem);
        if(power_loss_info.wdt_reboot_support == WDT_REBOOT_OFF) {
            up_read(&power_loss_info.rwsem);
            printk(KERN_WARNING "%s Power Loss Test: wdt reboot pause...!\n", TAG);
            msleep(1000);
            continue;
        }
        up_read(&power_loss_info.rwsem);
        printk(KERN_ERR "%s Power Loss Test: ret = %d, do reset now...\n", TAG, ret);

#ifdef PWR_LOSS_MT6575
    #ifdef CONFIG_MTK_MTD_NAND
    #endif
        wdt_arch_reset(0xff);
#elif defined  PWR_LOSS_MT6573
    #ifdef CONFIG_MTK_MTD_NAND
        if(!mt6573_nandchip_Reset()){
            printk(KERN_ERR "%s NAND_MVG mt6573_nandchip_Reset Failed!\n", TAG);
        }
    #endif

        
        *Reg1 = 0x2200;
        *Reg2 = (0x3F<<5)|0x8;
        *Reg3 = 0x1971;
        *Reg1 = 0x2217;
#endif
        while(1);
    }
}
#endif 
#endif 

#ifdef PWR_LOSS_PROC
static int power_loss_info_init(void)
{
    memset(&power_loss_info, 0, sizeof(wdt_reboot_info));

    init_rwsem(&power_loss_info.rwsem);
    down_write(&power_loss_info.rwsem);

    INIT_LIST_HEAD(&power_loss_info.addr_list);

    #ifdef CONFIG_PWR_LOSS_MTK_SPOH
    power_loss_info.pl_counter = -1;
    #else
    power_loss_info.wdt_reboot_support = WDT_REBOOT_ON;
    #endif

    #ifdef CONFIG_MTK_EMMC_SUPPORT
    power_loss_info.drv_priv=(void *)&mvg_spoh_emmc_priv;
    #endif

    up_write(&power_loss_info.rwsem);

    return 0;
}

static int power_loss_debug_proc_write(struct file *file, const char __user* buf, size_t count, loff_t *data)
{
    int ret;
    int wdt_reboot_support;

    if (count == 0) return -1;
    if (count > 255) count = 255;

    ret = copy_from_user(cmd_buf, buf, count);
    if (ret < 0) return -1;

    cmd_buf[count] = '\0';

    if (1 == sscanf(cmd_buf, "%x", &wdt_reboot_support)) {
        if(wdt_reboot_support < 0){
            printk(KERN_ERR "%s [%s] : command format is error, please help to check!!!\n", TAG, __func__);
            return -1;
        }
        else {
            down_write(&power_loss_info.rwsem);
            power_loss_info.wdt_reboot_support = (wdt_reboot_support == WDT_REBOOT_OFF) ? WDT_REBOOT_OFF : WDT_REBOOT_ON;
            up_write(&power_loss_info.rwsem);

            printk(KERN_NOTICE "%s [****PWR_LOSS_DEBUG****]\n", TAG);
            printk(KERN_WARNING "%s WDT REBOOT\t", TAG);
            if(wdt_reboot_support == WDT_REBOOT_ON)
            {
                printk(KERN_WARNING "%s ON\n", TAG);
            }
            else
                printk(KERN_WARNING "%s OFF\n", TAG);
        }
    }else {
        printk(KERN_ERR "%s [%s] : command format is error, please help to check!!!\n", TAG, __func__);
        return -1;
    }

    return count;
}

static int power_loss_debug_proc_show(struct seq_file *m, void *data)
{
    int wdt_reboot_support = 0;

    down_read(&power_loss_info.rwsem);
    wdt_reboot_support = power_loss_info.wdt_reboot_support;
    up_read(&power_loss_info.rwsem);
    seq_printf(m, "WDT REBOOT STATUS\t%d\n", wdt_reboot_support);

    return 0;
}

static int power_loss_debug_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, power_loss_debug_proc_show, inode->i_private);
}

static const struct file_operations power_loss_debug_fops = {
    .open = power_loss_debug_proc_open,
    .write = power_loss_debug_proc_write,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static int power_loss_debug_init(void)
{
    struct proc_dir_entry *power_loss_debug;

    power_loss_debug = proc_create("power_loss_debug", 0660, NULL, &power_loss_debug_fops);
    if(power_loss_debug)
    {
        printk(KERN_NOTICE "%s [%s]: Successfully create /proc/power_loss_debug\n", TAG, __func__);
    }else{
        printk(KERN_ERR "%s [%s]: Failed to create /proc/power_loss_debug\n", TAG, __func__);
        return -1;
    }

    return 0;
}

#endif

static int __init power_loss_init(void)
{
    int err;

    printk(KERN_NOTICE "%s Power Loss Test Module Init\n", TAG);

    err = alloc_chrdev_region(&sg_pwr_loss_devno, PWR_LOSS_FIRST_MINOR, PWR_LOSS_MAX_MINOR_COUNT, PWR_LOSS_DEVNAME);
    if (err != 0){
        printk(KERN_ERR "%s Power Loss Test: alloc_chardev_region Failed!\n", TAG);
        return err;
    }

#ifdef PWR_LOSS_DEBUG
    printk(KERN_NOTICE "%s Power Loss Test: MAJOR =%d, MINOR=%d\n", TAG,
                      MAJOR(sg_pwr_loss_devno), MINOR(sg_pwr_loss_devno));
#endif

    sg_pwr_loss_dev = cdev_alloc();
    if (NULL == sg_pwr_loss_dev){
        printk(KERN_ERR "%s Power Loss Test: cdev_alloc Failed\n", TAG);
        goto out2;
    }

    sg_pwr_loss_dev->owner = THIS_MODULE;
    sg_pwr_loss_dev->ops   = &pwr_loss_fops;

    err = cdev_add(sg_pwr_loss_dev, sg_pwr_loss_devno, 1);
    if (err != 0){
        printk(KERN_ERR "%s Power Loss Test: cdev_add Failed!\n", TAG);
        goto out2;
    }

    sg_pwr_loss_dev_class = class_create(THIS_MODULE, PWR_LOSS_DEVNAME);
    if (NULL == sg_pwr_loss_dev_class){
        printk(KERN_ERR "%s Power Loss Test: class_create Failed!\n", TAG);
        goto out1;
    }

    sg_pwr_loss_dev_file = device_create(sg_pwr_loss_dev_class, NULL, sg_pwr_loss_devno, NULL, PWR_LOSS_DEVNAME);
    if (NULL == sg_pwr_loss_dev_file){
        printk(KERN_ERR "%s Power Loss Test: device_create Failed!\n", TAG);
        goto out;
    }

#ifdef PWR_LOSS_SPOH
    wake_lock_init(&spoh_wake_lock, WAKE_LOCK_SUSPEND, "spoh");
    wake_lock(&spoh_wake_lock);
#endif

#ifdef PWR_LOSS_PROC
    power_loss_info_init();
    err = power_loss_debug_init();
    if(err  < 0)
        goto out;
#endif

#ifdef CONFIG_OF
    if (mvg_toprgu_node == NULL) {
        mvg_toprgu_node = of_find_compatible_node(NULL, NULL, "mediatek,TOPRGU");
        printk(KERN_ERR "mvg_toprgu_node at %p\n",mvg_toprgu_node);
        if ( mvg_toprgu_node ) {
            mvg_toprgu_base = of_iomap(mvg_toprgu_node, 0);
            printk(KERN_ERR "of_iomap for toprgu base @ 0x%p\n", mvg_toprgu_base);
        } else {
            printk(KERN_ERR TAG "Power Loss Test: Get toprgu Failed!\n");
            goto out;
        }
    }
#endif

    printk(KERN_ERR "%s Power Loss Test: Init Successfully!\n", TAG);

#ifdef PWR_LOSS_SW_RESET
    kernel_thread(pwr_loss_reset_thread, NULL, CLONE_VM);    
    printk(KERN_ERR "%s Power Loss Test: kernel thread create Successful!\n", TAG);
#endif

    return 0;
out:
    class_destroy(sg_pwr_loss_dev_class);
out1:
    cdev_del(sg_pwr_loss_dev);
out2:
    unregister_chrdev_region(sg_pwr_loss_devno, PWR_LOSS_MAX_MINOR_COUNT);
#ifdef PWR_LOSS_PROC
    remove_proc_entry("power_loss_debug", NULL);
#endif

    return err;
}

static void __exit power_loss_exit(void)
{
#ifdef PWR_LOSS_DEBUG
    printk(KERN_NOTICE "%s Power Loss Test: Module Exit\n", TAG);
#endif

#ifdef PWR_LOSS_SPOH
    wake_lock_destroy(&spoh_wake_lock);
#endif

    unregister_chrdev_region(sg_pwr_loss_devno, PWR_LOSS_MAX_MINOR_COUNT);
    cdev_del(sg_pwr_loss_dev);
    device_destroy(sg_pwr_loss_dev_class, sg_pwr_loss_devno);
    class_destroy(sg_pwr_loss_dev_class);
#ifdef PWR_LOSS_PROC
    remove_proc_entry("power_loss_debug", NULL);
#endif

    printk(KERN_ERR "%s Power Loss Test:module exit Successfully!\n", TAG);
}

#ifdef PWR_LOSS_SPOH

int mvg_get_method(void)
{
    int ret;
    down_read(&power_loss_info.rwsem);
    ret=power_loss_info.pl_method;
    up_read(&power_loss_info.rwsem);
    return ret;
}

void mvg_set_method(int method)
{
    down_write(&power_loss_info.rwsem);
    power_loss_info.pl_method = method;
    up_write(&power_loss_info.rwsem);
    printk(KERN_ERR "%s set mehtod to %X\n", TAG, power_loss_info.pl_method);
}

int mvg_get_wdt(void)
{
    int ret;
    down_read(&power_loss_info.rwsem);
    ret=power_loss_info.wdt_enable;
    up_read(&power_loss_info.rwsem);
    return ret;
}

void mvg_set_wdt(int enable)
{
    down_write(&power_loss_info.rwsem);
    power_loss_info.wdt_enable = enable;
    up_write(&power_loss_info.rwsem);
    printk(KERN_ERR "%s set wdt to %X\n", TAG, power_loss_info.wdt_enable);
}

int mvg_get_counter(void)
{
    int ret;

    down_read(&power_loss_info.rwsem);
    ret=power_loss_info.pl_counter;
    up_read(&power_loss_info.rwsem);
    return ret;
}

void mvg_set_counter(int counter)
{
    down_write(&power_loss_info.rwsem);
    power_loss_info.pl_counter = counter;
    up_write(&power_loss_info.rwsem);
    printk(KERN_ERR "%s set counter to %X\n", TAG, power_loss_info.pl_counter);
}


void mvg_set_trigger(int trigger)
{
    down_write(&power_loss_info.rwsem);
    power_loss_info.pl_trigger= trigger;
    up_write(&power_loss_info.rwsem);
    printk(KERN_ERR "%s set trigger to %X\n", TAG, power_loss_info.pl_trigger);
}


void mvg_set_flag(u32 flag)
{
    down_write(&power_loss_info.rwsem);
    power_loss_info.flags = flag;
    up_write(&power_loss_info.rwsem);
    printk(KERN_ERR "%s set flag to %X\n", TAG, power_loss_info.flags);
}
u32  mvg_get_flag(void)
{
    int ret;
    down_read(&power_loss_info.rwsem);
    ret=power_loss_info.flags;
    up_read(&power_loss_info.rwsem);
    return ret;
}

void mvg_set_curr_case_name(char *str)
{
    down_write(&power_loss_info.rwsem);
    strncpy(power_loss_info.current_case_name, str, MVG_NAME_LIMIT);
    up_write(&power_loss_info.rwsem);
    printk(KERN_ERR "%s set case name to %s\n", TAG, power_loss_info.current_case_name);
}
void mvg_set_curr_group_name(char *str)
{
    down_write(&power_loss_info.rwsem);
    strncpy(power_loss_info.current_group_name, str, MVG_NAME_LIMIT);
    up_write(&power_loss_info.rwsem);
    printk(KERN_ERR "%s set group name to %s\n", TAG, power_loss_info.current_group_name);
}

void mvg_get_curr_case_name(char *str, int size)
{
    down_read(&power_loss_info.rwsem);
    strncpy(str, power_loss_info.current_case_name,
        (size>MVG_NAME_LIMIT) ? MVG_NAME_LIMIT : size);
    up_read(&power_loss_info.rwsem);
}

void mvg_get_curr_group_name(char *str, int size)
{
    down_read(&power_loss_info.rwsem);
    strncpy(str, power_loss_info.current_group_name,
        (size>MVG_NAME_LIMIT) ? MVG_NAME_LIMIT : size);
    up_read(&power_loss_info.rwsem);
}

int mvg_on_group_case(const char *group_name, const char *case_name)
{
    int s1, s2;
    down_read(&power_loss_info.rwsem);
    s1=strcmp(group_name, power_loss_info.current_group_name);
    s2=strcmp(case_name, power_loss_info.current_case_name);
    up_read(&power_loss_info.rwsem);

    if (s1==0 && s2==0) 
        return 1;
    else
        return 0;
}

int mvg_addr_range_add(u64 base, u64 end)
{
    struct list_head *list_p;
    struct mvg_addr_list_entry *entry;

    printk(KERN_ERR "%s mvg_addr_range_add: %llX ~ %llX\n", TAG, base, end);

    if (end < base) {
        printk(KERN_ERR "%s mvg_addr_range_add: Invalid range\n", TAG);
        return 0;
    }

    entry = kzalloc(sizeof(struct mvg_addr_list_entry), GFP_NOFS);

    if (!entry) {
        return -ENOMEM;
    }

    entry->base = base;
    entry->end = end;

    down_write(&power_loss_info.rwsem);
    list_p = &power_loss_info.addr_list;
    INIT_LIST_HEAD( &(entry->list));
    list_add_tail(&(entry->list), list_p);
    up_write(&power_loss_info.rwsem);

    return 0;
}

void mvg_addr_range_del(u64 addr)
{
    struct mvg_addr_list_entry *ptr, *tmp;
    struct list_head *list_p;

    down_write(&power_loss_info.rwsem);
    list_p = &power_loss_info.addr_list;
    list_for_each_entry_safe(ptr, tmp, list_p, list) {
        if ((addr >= ptr->base) && (addr < ptr->end)) {
            list_del(&ptr->list);
            kfree(ptr);
        }
    }
    up_write(&power_loss_info.rwsem);
}

void mvg_addr_range_clear(void)
{
    struct mvg_addr_list_entry *ptr, *tmp;
    struct list_head *list_p;

    down_write(&power_loss_info.rwsem);
    list_p = &power_loss_info.addr_list;
    list_for_each_entry_safe(ptr, tmp, list_p, list) {
        list_del(&ptr->list);
        kfree(ptr);
    }
    up_write(&power_loss_info.rwsem);
}

int mvg_addr_range_check(u64 addr)
{
    struct mvg_addr_list_entry *ptr;
    struct list_head *list_p;

    int hit=0;

    down_read(&power_loss_info.rwsem);
    if (power_loss_info.wdt_enable) {
        list_p = &power_loss_info.addr_list;
        list_for_each_entry(ptr, list_p, list) {
            if ((addr >= ptr->base) && (addr < ptr->end)) {
                hit = 1;
                break;
            }
        }
    }
    up_read(&power_loss_info.rwsem);

    return hit;
}

int mvg_trigger(void)
{
    int ret;

    down_read(&power_loss_info.rwsem);
    ret=power_loss_info.pl_trigger;
    up_read(&power_loss_info.rwsem);

    if (MVG_TRIGGER_ALWAYS == ret)
        return ret;

    if (MVG_TRIGGER_NEVER != ret)
        ret = !(sched_clock() & ret);

    return ret;
}
#ifdef CONFIG_MTK_MTD_NAND

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <mach/mtk_nand.h>

extern struct mtk_nand_host *host;
void mvg_nand_set_program_wdt(int trigger)
{
    down_write(&power_loss_info.rwsem);
    host->pl.nand_program_wdt_enable = (u32)trigger;
    up_write(&power_loss_info.rwsem);
    printk(KERN_ERR "%s set nand_program_wdt_enable to %X\n", TAG, host->pl.nand_program_wdt_enable);
}

void mvg_nand_set_erase_wdt(int trigger)
{
    down_write(&power_loss_info.rwsem);
    host->pl.nand_erase_wdt_enable= (u32)trigger;
    up_write(&power_loss_info.rwsem);
    printk(KERN_ERR "%s set nand_erase_wdt_enable to %X\n", TAG, host->pl.nand_erase_wdt_enable);
}
#endif

#ifdef CONFIG_MTK_GPT_SCHEME_SUPPORT
#include <mach/partition.h>

int mvg_partition_info_gpt(const char *name, pt_resident *info)
{
    struct hd_struct *part_ptr;
    part_ptr= get_part(name);
    if ( !part_ptr ) {
        
    } else {
        if ( info ) {
            info->offset = (part_ptr->start_sect)<<9;
            info->size   = (part_ptr->nr_sects)<<9;
            put_part(part_ptr);
        }
    }
    
    return 0;
}

#else

#ifdef CONFIG_MTK_EMMC_SUPPORT
extern pt_resident *lastest_part; 
#else
extern pt_resident lastest_part[];  
#endif

int mvg_partition_info(const char *name, pt_resident *info)
{
    int i;
    for (i=0;i<PART_NUM;i++) {
        if (name && info) {
            if (strcmp(name, (char*)lastest_part[i].name)==0) {
                memcpy(info, &lastest_part[i], sizeof(pt_resident));
#ifdef CONFIG_MTK_EMMC_SUPPORT
                
                if (i>0) {
                    info->offset -= lastest_part[0].size;
                }
#endif
                return 0;
            }
        }
        else {
            #ifdef CONFIG_MTK_EMMC_SUPPORT
            printk(KERN_ERR "%s %s: %llx %llx\n",
                TAG,
                lastest_part[i].name,
                (i>0) ? (lastest_part[i].offset - lastest_part[0].size):
                    lastest_part[i].offset,
                lastest_part[i].size);
            #else
            printk(KERN_ERR "%s %s: %lx %lx\n",
                TAG,
                lastest_part[i].name,
                lastest_part[i].offset,
                lastest_part[i].size);
            #endif
        }
    }
    return -1;
}
#endif

#endif 


#ifdef CONFIG_MTK_MTD_NAND

#include <mach/mt_wdt.h>
#include <mach/mt_typedefs.h>

extern void emi_mem_psrv_diable(void);
extern void L1_rst_disable(unsigned int disable);
extern void L2_rst_disable(unsigned int disable);
extern void wdt_dump_reg(void);

static DEFINE_SPINLOCK(_mvg_spinlock);

void mvg_wdt_reset(void)
{
    unsigned int wdt_mode_val;

	spin_lock(&_mvg_spinlock);

	DRV_WriteReg32((SRAMROM_BASE + 0x24), 0);
	DRV_WriteReg32((SRAMROM_BASE + 0x28), 0);

	emi_mem_psrv_diable();
	L1_rst_disable(0);
	L2_rst_disable(0);

#ifndef CONFIG_EMULATION_EARLY_PORTING
	
	DRV_WriteReg32(MTK_WDT_RESTART_FAST, MTK_WDT_RESTART_FAST_KEY);

	wdt_mode_val = MTK_WDT_EN_FAST_KEY;
	DRV_WriteReg32(MTK_WDT_EN_FAST, wdt_mode_val);
#endif

	wdt_mode_val = DRV_Reg32(MTK_WDT_MODE);
	
	wdt_mode_val &=(~MTK_WDT_MODE_AUTO_RESTART);
	
	wdt_mode_val &=(~(MTK_WDT_MODE_IRQ | MTK_WDT_MODE_DUAL_MODE));
    wdt_mode_val = wdt_mode_val | (MTK_WDT_MODE_KEY|MTK_WDT_MODE_EXTEN|MTK_WDT_MODE_AUTO_RESTART);
	wdt_mode_val &= ~MTK_WDT_MODE_DDRRSV_MODE;
	DRV_WriteReg32(MTK_WDT_MODE,wdt_mode_val);
	DRV_WriteReg32(MTK_WDT_SWRST, MTK_WDT_SWRST_KEY);
	spin_unlock(&_mvg_spinlock);

	while (1)
	{
	    wdt_dump_reg();
		printk("mvg_wdt_reset error\n");
	}

}

#else

void mvg_wdt_reset(void)
{
    #ifdef CONFIG_OF
    unsigned int wdt_mode_val;
    DRV_WriteReg32(mvg_toprgu_base+0x8, MTK_WDT_RESTART_KEY); 
    wdt_mode_val = DRV_Reg32(mvg_toprgu_base);                
    
    wdt_mode_val &=(~MTK_WDT_MODE_AUTO_RESTART);
    
    wdt_mode_val &=(~(MTK_WDT_MODE_IRQ|MTK_WDT_MODE_ENABLE | MTK_WDT_MODE_DUAL_MODE));
    
    wdt_mode_val = wdt_mode_val | (MTK_WDT_MODE_KEY|MTK_WDT_MODE_EXTEN|MTK_WDT_MODE_AUTO_RESTART);
    DRV_WriteReg32(mvg_toprgu_base, wdt_mode_val);
    DRV_WriteReg32(mvg_toprgu_base+0x14, MTK_WDT_SWRST_KEY);  
    #else
    
    
    #if 1
    unsigned int ap_rgu_remap_base=(unsigned int)ioremap_nocache(0x10007000, 0x90);
    #else
    unsigned int ap_rgu_remap_base=AP_RGU_BASE;
    #endif
    unsigned int wdt_mode_val;
    DRV_WriteReg32(ap_rgu_remap_base+0x8, MTK_WDT_RESTART_KEY);  
    wdt_mode_val = DRV_Reg32(ap_rgu_remap_base);                 
    
    wdt_mode_val &=(~MTK_WDT_MODE_AUTO_RESTART);
    
    wdt_mode_val &=(~(MTK_WDT_MODE_IRQ|MTK_WDT_MODE_ENABLE | MTK_WDT_MODE_DUAL_MODE));
    
    wdt_mode_val = wdt_mode_val | (MTK_WDT_MODE_KEY|MTK_WDT_MODE_EXTEN|MTK_WDT_MODE_AUTO_RESTART);
    DRV_WriteReg32(ap_rgu_remap_base,wdt_mode_val);
    DRV_WriteReg32(ap_rgu_remap_base+0x14, MTK_WDT_SWRST_KEY);  
    #endif
}

#endif

module_init(power_loss_init);
module_exit(power_loss_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("feifei.wang <feifei.wang@mediatek.com>");
MODULE_DESCRIPTION(" This module is for power loss test");

