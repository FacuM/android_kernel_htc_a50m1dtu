#define LOG_TAG "ddp_drv"

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <generated/autoconf.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/param.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/timer.h>
#include <mach/mt_smi.h>
#include <linux/xlog.h>
#include <linux/proc_fs.h>  
#include <linux/miscdevice.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <asm/io.h>
#include <mach/mt_smi.h>
#include <mach/irqs.h>
#include <mach/mt_reg_base.h>
#include <mach/mt_irq.h>
#include <mach/irqs.h>
#include <mach/mt_clkmgr.h> 
#include <mach/mt_irq.h>
#include <mach/sync_write.h>
#include <mach/m4u.h>

#include "ddp_drv.h"
#include "ddp_reg.h"
#include "ddp_hal.h"
#include "ddp_log.h"
#include "ddp_irq.h"
#include "ddp_info.h"
#include "ddp_dpi_reg.h"

#include <linux/kobject.h>
#include <linux/sysfs.h>


extern PDPI_REGS DPI_REG;
#define DISP_DEVNAME "DISPSYS"

typedef struct
{
    pid_t open_pid;
    pid_t open_tgid;
    struct list_head testList;
    spinlock_t node_lock;
} disp_node_struct;


static struct kobject kdispobj;
extern unsigned int ddp_dump_reg_to_buf(unsigned int start_module,unsigned long * addr);

static ssize_t disp_kobj_show(struct kobject *kobj, 
                               struct attribute *attr, 
                               char *buffer) 
{ 
    int size=0x0;
		
    if (0 == strcmp(attr->name, "dbg1")) 
    {    
        size = ddp_dump_reg_to_buf(2,(unsigned long *) buffer);
    }
    else if(0 == strcmp(attr->name, "dbg2"))
    {
        size = ddp_dump_reg_to_buf(1,(unsigned long *) buffer);
    }
    else if(0 == strcmp(attr->name, "dbg3"))
    {
        size = ddp_dump_reg_to_buf(0,(unsigned long *) buffer);
    }
	
    return size;
}

static struct kobj_type disp_kobj_ktype = { 
        .sysfs_ops = &(struct sysfs_ops){ 
                .show = disp_kobj_show, 
                .store = NULL 
        }, 
        .default_attrs = (struct attribute*[]){ 
                &(struct attribute){ 
                        .name = "dbg1",   
                        .mode = S_IRUGO 
                }, 
                &(struct attribute){ 
                        .name = "dbg2",   
                        .mode = S_IRUGO 
                }, 
                &(struct attribute){ 
                        .name = "dbg3",   
                        .mode = S_IRUGO 
                }, 
                NULL 
        } 
};


static unsigned int ddp_ms2jiffies(unsigned long ms)
{
    return ((ms*HZ + 512) >> 10);
}

#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT) && defined(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)

#include "mobicore_driver_api.h"
#include "tlcApitplay.h"

static unsigned int *tplay_handle_virt_addr = NULL;
static dma_addr_t handle_pa = 0;

void init_tplay_handle(struct device *dev)
{
    void *va;
    va = dma_alloc_coherent(dev, sizeof(unsigned int), &handle_pa, GFP_KERNEL);
    if (NULL != va)
    {
        DDPDBG("[SVP] allocate handle_pa[%pa]\n", &va);
    }
    else
    {
        DDPERR("[SVP] failed to allocate handle_pa \n");
    }
    tplay_handle_virt_addr = (unsigned int *)va;
}

static int write_tplay_handle(unsigned int handle_value)
{
    if (NULL != tplay_handle_virt_addr)
    {
        DDPDBG("[SVP] write_tplay_handle 0x%x \n", handle_value);
        *tplay_handle_virt_addr = handle_value;
		return 0;
    }
    return -EFAULT;
}

static const uint32_t mc_deviceId = MC_DEVICE_ID_DEFAULT;

static const struct mc_uuid_t MC_UUID_TPLAY = { {0x05, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

static struct mc_session_handle tplaySessionHandle;
static tplay_tciMessage_t *pTplayTci  = NULL;

volatile static unsigned int opened_device = 0;
static enum mc_result late_open_mobicore_device(void)
{
    enum mc_result mcRet = MC_DRV_OK;

    if (0 == opened_device)
    {
        DDPDBG("=============== open mobicore device ===============\n");
        
        mcRet = mc_open_device(mc_deviceId);
        if (MC_DRV_ERR_INVALID_OPERATION == mcRet)
        {
            
            DDPDBG("mc_open_device already done\n");
        }
        else if (MC_DRV_OK != mcRet)
        {
            DDPERR("mc_open_device failed: %d @%s line %d\n", mcRet, __func__, __LINE__);
            return mcRet;
        }
        opened_device = 1;
    }
    return MC_DRV_OK;
}

static int open_tplay_driver_connection(void)
{
    enum mc_result mcRet = MC_DRV_OK;

    if (tplaySessionHandle.session_id != 0)
    {
        DDPMSG("tplay TDriver session already created\n");
        return 0;
    }

    DDPDBG("=============== late init tplay TDriver session ===============\n");
    do
    {
        late_open_mobicore_device();

        
        mcRet = mc_malloc_wsm(mc_deviceId, 0, sizeof(tplay_tciMessage_t), (uint8_t **) &pTplayTci, 0);
        if (MC_DRV_OK != mcRet)
        {
            DDPERR("mc_malloc_wsm failed: %d @%s line %d\n", mcRet, __func__, __LINE__);
            return -1;
        }

        
        memset(&tplaySessionHandle, 0, sizeof(tplaySessionHandle));
        tplaySessionHandle.device_id = mc_deviceId;
        mcRet = mc_open_session(&tplaySessionHandle,
                                &MC_UUID_TPLAY,
                                (uint8_t *) pTplayTci,
                                (uint32_t) sizeof(tplay_tciMessage_t));
        if (MC_DRV_OK != mcRet)
        {
            DDPERR("mc_open_session failed: %d @%s line %d\n", mcRet, __func__, __LINE__);
            
            memset(&tplaySessionHandle, 0, sizeof(tplaySessionHandle));
            return -1;
        }
    } while (0);

    return (MC_DRV_OK == mcRet) ? 0 : -1;
}

static int close_tplay_driver_connection(void)
{
    enum mc_result mcRet = MC_DRV_OK;
    DDPDBG("=============== close tplay TDriver session ===============\n");
        
    if (tplaySessionHandle.session_id != 0) 
    {
        mcRet = mc_close_session(&tplaySessionHandle);
        if (MC_DRV_OK != mcRet)
        {
            DDPERR("mc_close_session failed: %d @%s line %d\n", mcRet, __func__, __LINE__);
            memset(&tplaySessionHandle, 0, sizeof(tplaySessionHandle)); 
            return -1;
        }
    }
    memset(&tplaySessionHandle, 0, sizeof(tplaySessionHandle));

    mcRet = mc_free_wsm(mc_deviceId, (uint8_t *)pTplayTci);
    if (MC_DRV_OK != mcRet)
    {
        DDPERR("mc_free_wsm failed: %d @%s line %d\n", mcRet, __func__, __LINE__);
        return -1;
    }

    return 0;
}

static int set_tplay_handle_addr_request(void)
{
    int ret = 0;
    enum mc_result mcRet = MC_DRV_OK;

    DDPDBG("[SVP] set_tplay_handle_addr_request \n");

    open_tplay_driver_connection();
    if (tplaySessionHandle.session_id == 0)
    {
        DDPERR("[SVP] invalid tplay session \n");
        return -1;
    }

    DDPDBG("[SVP] handle_pa=0x%pa \n", &handle_pa);
    
    pTplayTci->tplay_handle_low_addr = (uint32_t)handle_pa;
    pTplayTci->tplay_handle_high_addr = (uint32_t)(handle_pa >> 32);
    
    pTplayTci->cmd.header.commandId = CMD_TPLAY_REQUEST;

    
    DDPDBG("[SVP] notify Tlsec trustlet CMD_TPLAY_REQUEST \n");
    mcRet = mc_notify(&tplaySessionHandle);
    if (MC_DRV_OK != mcRet)
    {
        DDPERR("[SVP] mc_notify failed: %d @%s line %d\n", mcRet, __func__, __LINE__);
        ret = -1;
        goto _notify_to_trustlet_fail;
    }

    
	mcRet = mc_wait_notification(&tplaySessionHandle, MC_INFINITE_TIMEOUT);
    if (MC_DRV_OK != mcRet)
    {
        DDPERR("[SVP] mc_wait_notification failed: %d @%s line %d\n", mcRet, __func__, __LINE__);
        ret = -1;
        goto _notify_from_trustlet_fail;
    }

    DDPDBG("[SVP] CMD_TPLAY_REQUEST result=%d, return code=%d\n", pTplayTci->result, pTplayTci->rsp.header.returnCode);

_notify_from_trustlet_fail:
_notify_to_trustlet_fail:
    close_tplay_driver_connection();

    return ret;
}

#ifdef TPLAY_DUMP_PA_DEBUG
static int dump_tplay_physcial_addr(void)
{
    DDPDBG("[SVP] dump_tplay_physcial_addr \n");
    int ret = 0;   
    enum mc_result mcRet = MC_DRV_OK;
    open_tplay_driver_connection();
    if (tplaySessionHandle.session_id == 0)
    {
        DDPERR("[SVP] invalid tplay session \n");
        return -1;
    }

    
    pTplayTci->cmd.header.commandId = CMD_TPLAY_DUMP_PHY; 
   
            
    DDPMSG("[SVP] notify Tlsec trustlet CMD_TPLAY_DUMP_PHY \n");
    mcRet = mc_notify(&tplaySessionHandle);        
    if (MC_DRV_OK != mcRet)        
    {            
        DDPERR("[SVP] mc_notify failed: %d @%s line %d\n", mcRet, __func__, __LINE__); 
        ret = -1;
        goto _notify_to_trustlet_fail;
    }

    
    mcRet = mc_wait_notification(&tplaySessionHandle, MC_INFINITE_TIMEOUT);
    if (MC_DRV_OK != mcRet)
    {            
        DDPERR("[SVP] mc_wait_notification failed: %d @%s line %d\n", mcRet, __func__, __LINE__);
        ret = -1;
        goto _notify_from_trustlet_fail;
    }

    DDPDBG("[SVP] CMD_TPLAY_DUMP_PHY result=%d, return code=%d\n", pTplayTci->result, pTplayTci->rsp.header.returnCode);

_notify_from_trustlet_fail:
_notify_to_trustlet_fail:
    close_tplay_driver_connection();

    return ret;
}
#endif 

static int disp_path_notify_tplay_handle(unsigned int handle_value)
{
    int ret;
    static int executed = 0; 

    if (0 == executed)
    {
        if (set_tplay_handle_addr_request())
			return -EFAULT;
        executed = 1;
    }

    ret = write_tplay_handle(handle_value);

#ifdef TPLAY_DUMP_PA_DEBUG
    dump_tplay_physcial_addr();
#endif 

    return ret;
}
#endif 

static long disp_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    disp_node_struct *pNode = (disp_node_struct *)file->private_data;

#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT) && defined(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)
    if (DISP_IOCTL_SET_TPLAY_HANDLE == cmd)
    {
        unsigned int value;
        if(copy_from_user(&value, (void*)arg , sizeof(unsigned int)))
        {
            DDPERR("DISP_IOCTL_SET_TPLAY_HANDLE, copy_from_user failed\n");
            return -EFAULT;
        }
        if (disp_path_notify_tplay_handle(value)) {
            DDPERR("DISP_IOCTL_SET_TPLAY_HANDLE, disp_path_notify_tplay_handle failed\n");
            return -EFAULT;
        }
    }
#endif

    return 0;        
}

static long disp_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT) && defined(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)
    if (DISP_IOCTL_SET_TPLAY_HANDLE == cmd)
    {
        return disp_unlocked_ioctl(file, cmd, arg);
    }
#endif

    return 0;		 
}

static int disp_open(struct inode *inode, struct file *file)
{
    disp_node_struct *pNode = NULL;

    DDPDBG("enter disp_open() process:%s\n",current->comm);

    
    file->private_data = kmalloc(sizeof(disp_node_struct) , GFP_ATOMIC);
    if(NULL == file->private_data)
    {
        DDPMSG("Not enough entry for DDP open operation\n");
        return -ENOMEM;
    }

    pNode = (disp_node_struct *)file->private_data;
    pNode->open_pid = current->pid;
    pNode->open_tgid = current->tgid;
    INIT_LIST_HEAD(&(pNode->testList));
    spin_lock_init(&pNode->node_lock);

    return 0;

}

static ssize_t disp_read(struct file *file, char __user *data, size_t len, loff_t *ppos)
{
    return 0;
}

static int disp_release(struct inode *inode, struct file *file)
{
    disp_node_struct *pNode = NULL;
    unsigned int index = 0;
    DDPDBG("enter disp_release() process:%s\n",current->comm);

    pNode = (disp_node_struct *)file->private_data;

    spin_lock(&pNode->node_lock);

    spin_unlock(&pNode->node_lock);

    if(NULL != file->private_data)
    {
        kfree(file->private_data);
        file->private_data = NULL;
    }

    return 0;
}

static int disp_flush(struct file * file , fl_owner_t a_id)
{
    return 0;
}

static int disp_mmap(struct file * file, struct vm_area_struct * a_pstVMArea)
{
#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT) && (CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)
    a_pstVMArea->vm_page_prot = pgprot_noncached(a_pstVMArea->vm_page_prot);
    if(remap_pfn_range(a_pstVMArea , 
                 a_pstVMArea->vm_start , 
                 a_pstVMArea->vm_pgoff , 
                 (a_pstVMArea->vm_end - a_pstVMArea->vm_start) , 
                 a_pstVMArea->vm_page_prot))
    {
        DDPERR("MMAP failed!!\n");
        return -1;
    }
#endif

    return 0;
}

struct dispsys_device
{
	void __iomem *regs[DISP_REG_NUM];
	struct device *dev;
	int irq[DISP_REG_NUM];
};
static struct dispsys_device *dispsys_dev;
static int nr_dispsys_dev = 0;
unsigned int dispsys_irq[DISP_REG_NUM] = {0};
unsigned long dispsys_reg[DISP_REG_NUM] = {0};
unsigned long mipi_tx_reg=0;
unsigned long dsi_reg_va = 0;

extern irqreturn_t disp_irq_handler(int irq, void *dev_id);
extern int ddp_path_init();
unsigned int ddp_reg_pa_base[DISP_REG_NUM] = {
0x14007000,0x14008000,0x14009000,0x1400A000,
0x1400B000,0x1400C000,0x1400D000,0x1400E000,
0x1400F000,0x14010000,0x14011000,0x14014000,
0x14018000,0x14015000,0x14012000,0x14013000,
0x14000000,0x14016000,0x14017000,0x10206000, 0x10000000, 0x100020F0, 0x10215000};
unsigned int ddp_irq_num[DISP_REG_NUM] = {
184,185,186,187,188,189,190,191,192,193,
194,0  ,198,177,195,196,197  ,0  ,0,  0, 0, 0, 0};

static int disp_is_intr_enable(DISP_REG_ENUM module)
{
    switch(module)
    {
        case DISP_REG_OVL0   :
        case DISP_REG_OVL1   :
        case DISP_REG_RDMA0  :
        case DISP_REG_RDMA1  :
        case DISP_REG_MUTEX  :
        case DISP_REG_DSI0   :  
        case DISP_REG_AAL    :
        case DISP_REG_CONFIG :
            return 1;     
        case DISP_REG_WDMA1  :  
        case DISP_REG_COLOR  :
        case DISP_REG_CCORR  :
        case DISP_REG_GAMMA  :
        case DISP_REG_DITHER :
        case DISP_REG_UFOE   :
        case DISP_REG_PWM    :
        case DISP_REG_DPI0   :
        case DISP_REG_SMI_LARB0 :
        case DISP_REG_SMI_COMMON:
        case DISP_REG_CONFIG2:
        case DISP_REG_CONFIG3:
		case DISP_REG_IO_DRIVING:
		case DISP_REG_MIPI:
            return 0;

        case DISP_REG_WDMA0  :
#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT) && defined(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)
			return 0;
#else
			return 1;
#endif

        default: return 0;
    }
}

extern int m4u_enable_tf(int port, bool fgenable);
m4u_callback_ret_t disp_m4u_callback(int port, unsigned long mva, void* data)
{
    DISP_MODULE_ENUM module = DISP_MODULE_OVL0;
    DDPERR("fault call port=%d, mva=0x%lx, data=0x%p\n", port, mva, data);
    switch(port)
    {
		case M4U_PORT_DISP_OVL0 : module = DISP_MODULE_OVL0; break;
		case M4U_PORT_DISP_RDMA0: module = DISP_MODULE_RDMA0; break;
		case M4U_PORT_DISP_WDMA0: module = DISP_MODULE_WDMA0; break;
		case M4U_PORT_DISP_OVL1 : module = DISP_MODULE_OVL1; break;
		case M4U_PORT_DISP_RDMA1: module = DISP_MODULE_RDMA1; break;
		case M4U_PORT_DISP_WDMA1: module = DISP_MODULE_WDMA1; break;
		default: DDPERR("unknown port=%d\n", port);
    }
    ddp_dump_analysis(module);
    ddp_dump_reg(module);

    
    if(module==DISP_MODULE_OVL0)
    {
        ddp_dump_analysis(DISP_MODULE_OVL1);
    }
    else if(module==DISP_MODULE_OVL1)
    {
        ddp_dump_analysis(DISP_MODULE_OVL0);
    }
    m4u_enable_tf(port, 0);  

    return M4U_CALLBACK_HANDLED;
}

void disp_m4u_tf_disable(void)
{
    
    m4u_enable_tf(M4U_PORT_DISP_OVL1, 0);
}

#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT) && (CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)
static struct miscdevice disp_misc_dev;
#endif
static struct file_operations disp_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl = disp_unlocked_ioctl,
	.compat_ioctl   = disp_compat_ioctl,
	.open		= disp_open,
	.release	= disp_release,
	.flush		= disp_flush,
	.read       = disp_read,
#if 0
	.mmap       = disp_mmap
#endif
};

static int disp_probe(struct platform_device *pdev)
{
    struct class_device;
	int ret;
	int i;
    int new_count;
    static unsigned int disp_probe_cnt = 0;

    if(disp_probe_cnt!=0)
    {
        return 0;
    }

#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT) && (CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)
	disp_misc_dev.minor = MISC_DYNAMIC_MINOR;
	disp_misc_dev.name = "mtk_disp";
	disp_misc_dev.fops = &disp_fops;
	disp_misc_dev.parent = NULL;
	ret = misc_register(&disp_misc_dev);
	if (ret) {
		pr_err("disp: fail to create mtk_disp node\n");
		return ERR_PTR(ret);
	}

	
	init_tplay_handle(&(pdev->dev)); 
#endif
	
    new_count = nr_dispsys_dev + 1;
    dispsys_dev = krealloc(dispsys_dev, 
        sizeof(struct dispsys_device) * new_count, GFP_KERNEL);
    if (!dispsys_dev) {
        DDPERR("Unable to allocate dispsys_dev\n");
        return -ENOMEM;
    }
	
    dispsys_dev = &(dispsys_dev[nr_dispsys_dev]);
    dispsys_dev->dev = &pdev->dev;

	
	disp_init_irq();

    
    for(i=0;i<DISP_REG_NUM;i++)
    {
        dispsys_dev->regs[i] = of_iomap(pdev->dev.of_node, i);
        if (!dispsys_dev->regs[i]) {
            DDPERR("Unable to ioremap registers, of_iomap fail, i=%d \n", i);
            return -ENOMEM;
        }
		dispsys_reg[i] = dispsys_dev->regs[i];
        
        if(i<DISP_REG_CONFIG && (i!=DISP_REG_PWM))
        {
            dispsys_dev->irq[i] = irq_of_parse_and_map(pdev->dev.of_node, i);
            dispsys_irq[i] = dispsys_dev->irq[i];
            if(disp_is_intr_enable(i)==1)
            {
                ret = request_irq(dispsys_dev->irq[i], (irq_handler_t)disp_irq_handler, 
                            IRQF_TRIGGER_NONE, DISP_DEVNAME,  NULL);  
                if (ret) {
                    DDPERR("Unable to request IRQ, request_irq fail, i=%d, irq=%d \n", i, dispsys_dev->irq[i]);
                    return ret;
                }
            }            
        }
        DDPMSG("DT, i=%d, module=%s, map_addr=%p, map_irq=%d, reg_pa=0x%x, irq=%d \n", 
            i, ddp_get_reg_module_name(i), dispsys_dev->regs[i], dispsys_dev->irq[i], ddp_reg_pa_base[i], ddp_irq_num[i]);
    }    
    nr_dispsys_dev = new_count;
    
	dsi_reg_va = dispsys_reg[DISP_REG_DSI0];
    mipi_tx_reg = dispsys_reg[DISP_REG_MIPI];
	DPI_REG = (PDPI_REGS)dispsys_reg[DISP_REG_DPI0];

	
#ifdef FPGA_EARLY_PORTING
	printk("[DISP Probe] power MMSYS:0x%lx,0x%lx\n",DISP_REG_CONFIG_MMSYS_CG_CLR0,DISP_REG_CONFIG_MMSYS_CG_CLR1);
	DISP_REG_SET(0,DISP_REG_CONFIG_MMSYS_CG_CLR0,0xFFFFFFFF);
	DISP_REG_SET(0,DISP_REG_CONFIG_MMSYS_CG_CLR1,0xFFFFFFFF);
	DISP_REG_SET(0,DISPSYS_CONFIG_BASE + 0xC04,0x1C000);
#endif
	

	
	ddp_path_init();

    
    DDPMSG("register m4u callback\n");
    m4u_register_fault_callback(M4U_PORT_DISP_OVL0, disp_m4u_callback, 0);
    m4u_register_fault_callback(M4U_PORT_DISP_RDMA0, disp_m4u_callback, 0);
    m4u_register_fault_callback(M4U_PORT_DISP_WDMA0, disp_m4u_callback, 0);
    m4u_register_fault_callback(M4U_PORT_DISP_OVL1, disp_m4u_callback, 0);
    m4u_register_fault_callback(M4U_PORT_DISP_RDMA1, disp_m4u_callback, 0);
    m4u_register_fault_callback(M4U_PORT_DISP_WDMA1, disp_m4u_callback, 0);
    

	DDPMSG("dispsys probe done.\n");	
	

	
	
	
	
	
	

	
	
	
	
    
     DDPMSG("sysfs disp +");
    
    if(kobject_init_and_add(&kdispobj, &disp_kobj_ktype, NULL, "disp") <0){
		DDPERR("fail to add disp\n");
		return -ENOMEM;
    }	
	
	return 0;
}

static int disp_remove(struct platform_device *pdev)
{
    
    kobject_put(&kdispobj);

#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT) && (CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)
	misc_deregister(&disp_misc_dev);
#endif
    return 0;
}

static void disp_shutdown(struct platform_device *pdev)
{
	
}


static int disp_suspend(struct platform_device *pdev, pm_message_t mesg)
{
    printk("\n\n==== DISP suspend is called ====\n");


    return 0;
}

static int disp_resume(struct platform_device *pdev)
{
    printk("\n\n==== DISP resume is called ====\n");


    return 0;
}


static const struct of_device_id dispsys_of_ids[] = {
	{ .compatible = "mediatek,DISPSYS", },
	{}
};

static struct platform_driver dispsys_of_driver = {
	.driver = {
		.name = DISP_DEVNAME,
		.owner = THIS_MODULE,
		.of_match_table = dispsys_of_ids,
	},
	.probe    = disp_probe,
	.remove   = disp_remove,
    .shutdown = disp_shutdown,
    .suspend  = disp_suspend,
    .resume   = disp_resume,
};

static int __init disp_init(void)
{
    int ret = 0;

    DDPMSG("Register the disp driver\n");
    if(platform_driver_register(&dispsys_of_driver))
    {
        DDPERR("failed to register disp driver\n");
        
        ret = -ENODEV;
        return ret;
    }

    return 0;
}

static void __exit disp_exit(void)
{
    platform_driver_unregister(&dispsys_of_driver);
}

module_init(disp_init);
module_exit(disp_exit);
MODULE_AUTHOR("Tzu-Meng, Chung <Tzu-Meng.Chung@mediatek.com>");
MODULE_DESCRIPTION("Display subsystem Driver");
MODULE_LICENSE("GPL");
