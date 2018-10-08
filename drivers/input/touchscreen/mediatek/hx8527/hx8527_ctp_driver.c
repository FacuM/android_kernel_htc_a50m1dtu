/* Himax Android Driver base on  MTK driver sample code Ver 2.4
*
* Copyright (C) 2012 Himax Corporation.
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/


#include "tpd.h"
#include <linux/interrupt.h>
#include <cust_eint.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>
#include "cust_gpio_usage.h"
#include <linux/hwmsen_helper.h>
#include <linux/dma-mapping.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/hwmsensor.h> 
#include <linux/hwmsen_dev.h> 
#include <linux/sensors_io.h> 

#ifdef CONFIG_TOUCHSCREEN_TOUCH_FW_UPDATE
#include <linux/input/touch_fw_update.h>
#include <linux/async.h>
#include <linux/wakelock.h>
#include <linux/firmware.h>
#endif



#ifdef SLT_DEVINFO_CTP
#include<linux/dev_info.h>
struct devinfo_struct *s_DEVINFO_ctp = NULL;

static void devinfo_ctp_regchar(char *module,char * vendor,char *version,char *used)
{
 	s_DEVINFO_ctp =(struct devinfo_struct*) kmalloc(sizeof(struct devinfo_struct), GFP_KERNEL);
	s_DEVINFO_ctp->device_type="CTP";
	s_DEVINFO_ctp->device_module=module;
	s_DEVINFO_ctp->device_vendor=vendor;
	s_DEVINFO_ctp->device_ic="Hx8527";
	s_DEVINFO_ctp->device_info=DEVINFO_NULL;
	s_DEVINFO_ctp->device_version=version;
	s_DEVINFO_ctp->device_used=used;
    devinfo_check_add_device(s_DEVINFO_ctp);
}
#endif

extern struct tpd_device *tpd;

#define HIMAX8528_NAME "HX8527-D40"

#define D(x...) printk(KERN_DEBUG "[TP] " x)
#define I(x...) printk(KERN_INFO "[TP] " x)
#define W(x...) printk(KERN_WARNING "[TP][WARNING] " x)
#define E(x...) printk(KERN_ERR "[TP][ERROR] " x)

#define Android4_0
#define HX_MTK_DMA                             

#define HX_TP_SYS_DIAG                         
#define HX_TP_SYS_REGISTER                     
#define HX_TP_SYS_DEBUG_LEVEL                  
#define HX_TP_SYS_FLASH_DUMP                   
#define HX_TP_SYS_SELF_TEST                    
#define HX_TP_SYS_VENDOR                       
#define HX_TP_SYS_ATTN                        
#define HX_TP_SYS_INT                         
#define HX_TP_SYS_RESET                    	   

#define HX_RST_PIN_FUNC                        
#define HX_FW_UPDATE_BY_I_FILE                 

#define HX_FLASH_TEST								

#ifdef HX_RST_PIN_FUNC
#define HX_ESD_WORKAROUND                
#define ENABLE_CHIP_STATUS_MONITOR		
#endif

#ifdef ENABLE_CHIP_STATUS_MONITOR
int running_status;
struct delayed_work himax_chip_monitor;
int suspend_state;
#endif

DEFINE_MUTEX(tp_wr_access);



#ifdef GN_MTK_BSP_DEVICECHECK
#include <linux/gn_device_check.h>
extern int gn_set_device_info(struct gn_device_info gn_dev_info);
#endif

#define TPD_BUTTON_HEIGH        (100)
#define TPD_BUTTON_WIDTH        (120)
#define TPD_KEY_COUNT  4
#define key_1           115,1961               
#define key_2           654,1961
#define key_3           1040,1961
#define key_4           1640,1961
#define TPD_KEYS        {KEY_MENU,KEY_HOMEPAGE,KEY_BACK}
#define TPD_KEYS_DIM    {{key_1,TPD_BUTTON_WIDTH,TPD_BUTTON_HEIGH},{key_2,TPD_BUTTON_WIDTH,TPD_BUTTON_HEIGH},{key_3,TPD_BUTTON_WIDTH,TPD_BUTTON_HEIGH}}



#define FLASH_DUMP_FILE "/sdcard/Flash_Dump.bin"

#define DIAG_COORDINATE_FILE "/data/Coordinate_Dump.csv"
#define HX_85XX_A_SERIES_PWON		1
#define HX_85XX_B_SERIES_PWON		2
#define HX_85XX_C_SERIES_PWON		3
#define HX_85XX_D_SERIES_PWON		4

#define HX_TP_BIN_CHECKSUM_SW		1
#define HX_TP_BIN_CHECKSUM_HW		2
#define HX_TP_BIN_CHECKSUM_CRC	        3

static char touchdown_flag =  0;
static char rec_msg_flag   =  0;
static char rec_data_flag  =  0;

#define Himax_fcover
#ifdef Himax_fcover
int fcover_close_flag = 0x01; 
#define fcover_x_min   11
#define fcover_x_max   708
#define fcover_y_min   11
#define fcover_y_max   1218
#endif

#ifdef Himax_Gesture
int wake_switch = 1;
int gesture_switch = 1;
static char FC_debug_flag = 0;
static char charge_debug_flag = 0;
static char ges_debug_flag = 0;
#endif

#ifdef HX_FLASH_TEST
static unsigned char IrefTable_1[16][2] =
{
	{0x18,0x07},	{0x18,0x17},	{0x18,0x27},	{0x18,0x37},
	{0x18,0x47},	{0x18,0x57},	{0x18,0x67},	{0x18,0x77},
	{0x18,0x87},	{0x18,0x97},	{0x18,0xA7},	{0x18,0xB7},
	{0x18,0xC7},	{0x18,0xD7},	{0x18,0xE7},	{0x18,0xF7}
};

static unsigned char IrefTable_2[16][2] =
{
	{0x98,0x06},	{0x98,0x16},	{0x98,0x26},	{0x98,0x36},
	{0x98,0x46},	{0x98,0x56},	{0x98,0x66},	{0x98,0x76},
	{0x98,0x86},	{0x98,0x96},	{0x98,0xA6},	{0x98,0xB6},
	{0x98,0xC6},	{0x98,0xD6},	{0x98,0xE6},	{0x98,0xF6}
};

static unsigned char IrefTable_3[16][2] =
{
	{0x18,0x06},	{0x18,0x16},	{0x18,0x26},	{0x18,0x36},
	{0x18,0x46},	{0x18,0x56},	{0x18,0x66},	{0x18,0x76},
	{0x18,0x86},	{0x18,0x96},	{0x18,0xA6},	{0x18,0xB6},
	{0x18,0xC6},	{0x18,0xD6},	{0x18,0xE6},	{0x18,0xF6}
};

static unsigned char IrefTable_4[16][2] =
{
	{0x98,0x05},	{0x98,0x15},	{0x98,0x25},	{0x98,0x35},
	{0x98,0x45},	{0x98,0x55},	{0x98,0x65},	{0x98,0x75},
	{0x98,0x85},	{0x98,0x95},	{0x98,0xA5},	{0x98,0xB5},
	{0x98,0xC5},	{0x98,0xD5},	{0x98,0xE5},	{0x98,0xF5}
};

static unsigned char IrefTable_5[16][2] =
{
	{0x18,0x05},	{0x18,0x15},	{0x18,0x25},	{0x18,0x35},
	{0x18,0x45},	{0x18,0x55},	{0x18,0x65},	{0x18,0x75},
	{0x18,0x85},	{0x18,0x95},	{0x18,0xA5},	{0x18,0xB5},
	{0x18,0xC5},	{0x18,0xD5},	{0x18,0xE5},	{0x18,0xF5}
};

static unsigned char IrefTable_6[16][2] =
{
	{0x98,0x04},	{0x98,0x14},	{0x98,0x24},	{0x98,0x34},
	{0x98,0x44},	{0x98,0x54},	{0x98,0x64},	{0x98,0x74},
	{0x98,0x84},	{0x98,0x94},	{0x98,0xA4},	{0x98,0xB4},
	{0x98,0xC4},	{0x98,0xD4},	{0x98,0xE4},	{0x98,0xF4}
};

static unsigned char IrefTable_7[16][2] =
{
	{0x18,0x04},	{0x18,0x14},	{0x18,0x24},	{0x18,0x34},
	{0x18,0x44},	{0x18,0x54},	{0x18,0x64},	{0x18,0x74},
	{0x18,0x84},	{0x18,0x94},	{0x18,0xA4},	{0x18,0xB4},
	{0x18,0xC4},	{0x18,0xD4},	{0x18,0xE4},	{0x18,0xF4}
};
static int iref_number = 11;
static bool iref_found = false;
static bool flash_checksum_pass = true;		
#endif

struct touch_info
{
	int y[10];	
	int x[10];	
	int p[10];	
	int id[10];	
	int count;	
};


#ifdef BUTTON_CHECK
struct t_pos_queue {
    int pos;
    unsigned int timestamp;
};
#endif


static struct i2c_client *i2c_client    = NULL;
static struct task_struct *touch_thread = NULL;
static DECLARE_WAIT_QUEUE_HEAD(waiter);
static DEFINE_MUTEX(i2c_access);

struct workqueue_struct	*himax_wq;
static int tpd_flag  = 0;
static int tpd_halt  = 0;
static int point_num = 0;

static struct kobject *android_touch_kobj = NULL;

uint8_t	IC_STATUS_CHECK	  = 0xAA; 
unsigned char IC_CHECKSUM = 0;
static unsigned char IC_TYPE     = 0;
unsigned char power_ret   = 0;

static int HX_TOUCH_INFO_POINT_CNT = 0;
static int HX_RX_NUM               = 0;
static int HX_TX_NUM               = 0;
static int HX_RX_NUM_2		   = 0;
static int HX_TX_NUM_2             = 0;
static int HX_BT_NUM               = 0;
static int HX_X_RES                = 0;
static int HX_Y_RES                = 0;
static int HX_MAX_PT               = 0;
static bool HX_INT_IS_EDGE         = false;
static int HX_XY_REV               =0;
static unsigned int HX_Gesture     =0;
static bool HX_KEY_HIT_FLAG=false;
static bool point_key_flag=false;
static bool last_point_key_flag=false;

unsigned char FW_VER_MAJ_FLASH_ADDR;
unsigned char FW_VER_MAJ_FLASH_LENG;
unsigned char FW_VER_MIN_FLASH_ADDR;
unsigned char FW_VER_MIN_FLASH_LENG;
unsigned char CFG_VER_MAJ_FLASH_ADDR;
unsigned char CFG_VER_MAJ_FLASH_LENG;
unsigned char CFG_VER_MIN_FLASH_ADDR;
unsigned char CFG_VER_MIN_FLASH_LENG;
unsigned char CFG_VER_MIN_FLASH_buff[12]={0};	

unsigned char CFG_VER_buff[12]={0};	
unsigned char CFG_Module_buff[12]={0};

extern kal_bool upmu_is_chr_det(void);
static int himax_charge_switch(s32 dir_update);
static u16 FW_VER_MAJ_buff[1]; 
static u16 FW_VER_MIN_buff[1];
static u16 CFG_VER_MAJ_buff[12];
static u16 CFG_VER_MIN_buff[12];
static int g_fw_verion = 0;
static int g_ifile_version = 0;

static int hx_point_num	= 0; 
static int p_point_num	= 0xFFFF;
static int tpd_key      = 0;
static int tpd_key_old  = 0xFF;

#ifdef HX_MTK_DMA
	static uint8_t *gpDMABuf_va = NULL;
	static uint32_t gpDMABuf_pa = NULL;
#endif

static int tpd_keys_local[TPD_KEY_COUNT] = TPD_KEYS;
static int tpd_keys_dim_local[TPD_KEY_COUNT][4] = TPD_KEYS_DIM;



	#ifdef TPD_PROXIMITY
	static u8 tpd_proximity_flag = 0;
	static u8 tpd_proximity_detect = 1;
	static int point_proximity_position;
	#define TPD_PROXIMITY_DMESG(a,arg...) printk("TPD_himax8526" ": " a,##arg)
	#define TPD_PROXIMITY_DEBUG(a,arg...) printk("TPD_himax8526" ": " a,##arg)
	#endif


	#ifdef HX_LOADIN_CONFIG
	unsigned char c1[] 	 =	{ 0x37, 0xFF, 0x08, 0xFF, 0x08};
	unsigned char c2[] 	 =	{ 0x3F, 0x00};
	unsigned char c3[] 	 =	{ 0x62, 0x01, 0x00, 0x01, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c4[] 	 =	{ 0x63, 0x10, 0x00, 0x10, 0x30, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c5[] 	 =	{ 0x64, 0x01, 0x00, 0x01, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c6[] 	 =	{ 0x65, 0x10, 0x00, 0x10, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c7[] 	 =	{ 0x66, 0x01, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c8[] 	 =	{ 0x67, 0x10, 0x00, 0x10, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c9[] 	 =	{ 0x68, 0x01, 0x00, 0x01, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c10[]	 =	{ 0x69, 0x10, 0x00, 0x10, 0x30, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c11[]	 =	{ 0x6A, 0x01, 0x00, 0x01, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c12[]	 =	{ 0x6B, 0x10, 0x00, 0x10, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c13[]	 =	{ 0x6C, 0x01, 0x00, 0x01, 0x30, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c14[]	 =	{ 0x6D, 0x10, 0x00, 0x10, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c15[]	 =	{ 0x6E, 0x01, 0x00, 0x01, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c16[]	 =	{ 0x6F, 0x10, 0x00, 0x10, 0x20, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c17[]	 =	{ 0x70, 0x01, 0x00, 0x01, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c18[]	 =	{ 0x7B, 0x03};
	unsigned char c19[]	 =	{ 0x7C, 0x00, 0xD8, 0x8C};
	unsigned char c20[]	 =	{ 0x7F, 0x00, 0x04, 0x0A, 0x0A, 0x04, 0x00, 0x00, 0x00};
	unsigned char c21[]	 =	{ 0xA4, 0x94, 0x62, 0x94, 0x86};
	unsigned char c22[]	 =	{ 0xB4, 0x04, 0x01, 0x01, 0x01, 0x01, 0x03, 0x0F, 0x04, 0x07, 0x04, 0x07, 0x04, 0x07, 0x00};
	unsigned char c23[]	 =	{ 0xB9, 0x01, 0x36};
	unsigned char c24[]	 =	{ 0xBA, 0x00};
	unsigned char c25[]	 =	{ 0xBB, 0x00};
	unsigned char c26[]	 =	{ 0xBC, 0x00, 0x00, 0x00, 0x00};
	unsigned char c27[]	 =	{ 0xBD, 0x04, 0x0C};
	unsigned char c28[]	 =	{ 0xC2, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char c29[]	 =	{ 0xC5, 0x0A, 0x1D, 0x00, 0x10, 0x1A, 0x1E, 0x0B, 0x1D, 0x08, 0x16};
	unsigned char c30[]	 =	{ 0xC6, 0x1A, 0x10, 0x1F};
	unsigned char c31[]	 =	{ 0xC9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x15, 0x17, 0x17, 0x19, 0x19, 0x1F, 0x1F, 0x1B, 0x1B, 0x1D, 0x1D, 0x21, 0x21, 0x23, 0x23,
														0x25, 0x25, 0x27, 0x27, 0x29, 0x29, 0x2B, 0x2B, 0x2D, 0x2D, 0x2F, 0x2F, 0x16, 0x16, 0x18, 0x18, 0x1A, 0x1A, 0x20, 0x20, 0x1C, 0x1C,
														0x1E, 0x1E, 0x22, 0x22, 0x24, 0x24, 0x26, 0x26, 0x28, 0x28, 0x2A, 0x2A, 0x2C, 0x2C, 0x2E, 0x2E, 0x30, 0x30, 0x00, 0x00, 0x00};
	unsigned char c32[] 	= { 0xCB, 0x01, 0xF5, 0xFF, 0xFF, 0x01, 0x00, 0x05, 0x00, 0x9F, 0x00, 0x00, 0x00};
	unsigned char c33[] 	= { 0xD0, 0x06, 0x01};
	unsigned char c34[] 	= { 0xD3, 0x06, 0x01};
	unsigned char c35[] 	= { 0xD5, 0xA5, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	unsigned char c36[] 	= { 0x40,0x01, 0x5A
													, 0x77, 0x02, 0xF0, 0x13, 0x00, 0x00
													, 0x56, 0x10, 0x14, 0x18, 0x06, 0x10, 0x0C, 0x0F, 0x0F, 0x0F, 0x52, 0x34, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	unsigned char c37[] 	= { 0x40, 0xA5, 0x00, 0x80, 0x82, 0x85, 0x00
													, 0x35, 0x25, 0x0F, 0x0F, 0x83, 0x3C, 0x00, 0x00
													, 0x11, 0x11, 0x00, 0x00
													, 0x01, 0x01, 0x00, 0x0A, 0x00, 0x00
													, 0x10, 0x02, 0x10, 0x64, 0x00, 0x00}; 

	unsigned char c38[] 	= {	0x40, 0x40, 0x38, 0x38, 0x02, 0x14, 0x00, 0x00, 0x00
													, 0x04, 0x03, 0x12, 0x06, 0x06, 0x00, 0x00, 0x00}; 

	unsigned char c39[] 	= {	0x40, 0x18, 0x18, 0x05, 0x00, 0x00, 0xD8, 0x8C, 0x00, 0x00, 0x42, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00
													, 0x10, 0x02, 0x80, 0x00, 0x00, 0x00, 0x00, 0x0C}; 

	unsigned char c40[] 	= {	0x40, 0x10, 0x12, 0x20, 0x32, 0x01, 0x04, 0x07, 0x09
													, 0xB4, 0x6E, 0x32, 0x00
													, 0x0F, 0x1C, 0xA0, 0x16
													, 0x00, 0x00, 0x04, 0x38, 0x07, 0x80}; 

	unsigned char c41[]	 	= {	0x40, 0x03, 0x2F, 0x08, 0x5B, 0x56, 0x2D, 0x05, 0x00, 0x69, 0x02, 0x15, 0x4B, 0x6C, 0x05
													, 0x03, 0xCE, 0x09, 0xFD, 0x58, 0xCC, 0x00, 0x00, 0x7F, 0x02, 0x85, 0x4C, 0xC7, 0x00};

	unsigned char c42[] 	= {	0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; 

	unsigned char c43_1[]	= {	0x40, 0x00, 0xFF, 0x15, 0x28, 0x01, 0xFF, 0x16, 0x29, 0x02, 0xFF, 0x1B, 0x2A, 0x03, 0xFF, 0x1C, 0xFF, 0x04, 0xFF, 0x1D, 0xFF, 0x05, 0x0F, 0x1E, 0xFF, 0x06, 0x10, 0x1F, 0xFF, 0x07, 0x11, 0x20}; 
	unsigned char c43_2[] = {	0x40, 0xFF, 0x08, 0x12, 0x21, 0xFF, 0x09, 0x13, 0x22, 0xFF, 0x0A, 0x14, 0x23, 0xFF, 0x0B, 0x17, 0x24, 0xFF, 0x0C, 0x18, 0x25, 0xFF, 0x0D, 0x19, 0x26, 0xFF, 0x0E, 0x1A, 0x27, 0xFF}; 

	unsigned char c44_1[] = {	0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; 
	unsigned char c44_2[] = {	0x40, 0x00, 0x00, 0x00, 0x00, 0x00}; 
	unsigned char c45[] 	= {	0x40, 0x1D, 0x00}; 
	#endif

extern void mt_eint_unmask(unsigned int line);
extern void mt_eint_mask(unsigned int line);
extern void mt_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern unsigned int mt_eint_set_sens(unsigned int eint_num, unsigned int sens);

static void tpd_eint_interrupt_handler(void);
static int  tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int  tpd_remove(struct i2c_client *client);
static int touch_event_handler(void *unused);

static int himax_ts_poweron(void);
static int himax_hang_shaking(void);
static bool himax_ic_package_check(void);
static int himax_touch_sysfs_init(void);
static void himax_touch_sysfs_deinit(void);
int himax_ManualMode(int enter);
int himax_FlashMode(int enter);
static int himax_lock_flash(void);
static int himax_unlock_flash(void);
static uint8_t himax_calculateChecksum(char *ImageBuffer, int fullLength);
static int himax_read_flash(unsigned char *buf, unsigned int addr_start, unsigned int length);
void himax_touch_information(void);

bool himax_debug_flag=false;

#ifdef HX_RST_PIN_FUNC
	void himax_HW_reset(void);
#endif

#ifdef HX_TP_SYS_DEBUG_LEVEL
	static uint8_t debug_log_level       = 0;
	static bool fw_update_complete       = false;
	static bool irq_enable               = false;
	static int handshaking_result        = 0;
	static unsigned char debug_level_cmd = 0;
	static unsigned char upgrade_fw[32*1024];

	static uint8_t getDebugLevel(void);
	int fts_ctpm_fw_upgrade_with_sys_fs(unsigned char *fw, int len);
#endif

#ifdef HX_TP_SYS_DIAG

	static bool Is_2T3R = false;

	static uint8_t x_channel    = 12;
	static uint8_t y_channel    = 21;
	static uint8_t *diag_mutual = NULL;

	static uint8_t x_channel_2 = 0;
	static uint8_t y_channel_2 = 0;
	static uint8_t *diag_mutual_2 = NULL;
	static uint8_t diag_command = 0;
	static uint8_t diag_coor[128];
	static uint8_t diag_self[100] = {0};

	static uint8_t *getMutualBuffer(void);
	static uint8_t *getSelfBuffer(void);
	static uint8_t 	getDiagCommand(void);
	static uint8_t 	getXChannel(void);
	static uint8_t 	getYChannel(void);


	static uint8_t *getMutualBuffer_2(void);
	static uint8_t 	getXChannel_2(void);
	static uint8_t 	getYChannel_2(void);

	static void setMutualBuffer(void);
	static void setXChannel(uint8_t x);
	static void setYChannel(uint8_t y);

	static void 	setMutualBuffer_2(void);
	static void 	setXChannel_2(uint8_t x);
	static void 	setYChannel_2(uint8_t y);
	static uint8_t coordinate_dump_enable = 0;
	struct file *coordinate_fn;
#endif

#ifdef HX_TP_SYS_REGISTER
	static uint8_t register_command       = 0;
	static uint8_t multi_register_command = 0;
	static uint8_t multi_register[8]      = {0x00};
	static uint8_t multi_cfg_bank[8]      = {0x00};
	static uint8_t multi_value[1024]      = {0x00};
	static bool config_bank_reg           = false;
#endif

#ifdef HX_TP_SYS_FLASH_DUMP
	struct workqueue_struct *flash_wq;
	struct work_struct flash_work;

	static uint8_t *flash_buffer       = NULL;
	static uint8_t flash_command       = 0;
	static uint8_t flash_read_step     = 0;
	static uint8_t flash_progress      = 0;
	static uint8_t flash_dump_complete = 0;
	static uint8_t flash_dump_fail     = 0;
	static uint8_t sys_operation       = 0;
	static uint8_t flash_dump_sector   = 0;
	static uint8_t flash_dump_page     = 0;
	static bool flash_dump_going       = false;

	static uint8_t getFlashCommand(void);
	static uint8_t getFlashDumpComplete(void);
	static uint8_t getFlashDumpFail(void);
	static uint8_t getFlashDumpProgress(void);
	static uint8_t getFlashReadStep(void);
	static uint8_t getSysOperation(void);
	static uint8_t getFlashDumpSector(void);
	static uint8_t getFlashDumpPage(void);
	static bool getFlashDumpGoing(void);

	static void setFlashBuffer(void);
	static void setFlashCommand(uint8_t command);
	static void setFlashReadStep(uint8_t step);
	static void setFlashDumpComplete(uint8_t complete);
	static void setFlashDumpFail(uint8_t fail);
	static void setFlashDumpProgress(uint8_t progress);
	static void setSysOperation(uint8_t operation);
	static void setFlashDumpSector(uint8_t sector);
	static void setFlashDumpPage(uint8_t page);
	static void setFlashDumpGoing(bool going);
#endif

#ifdef HX_TP_SYS_SELF_TEST
	static ssize_t himax_chip_self_test_function(struct device *dev, struct device_attribute *attr, char *buf);
	static int himax_chip_self_test(void);
#endif

#ifdef HX_FW_UPDATE_BY_I_FILE
enum wk_wdt_type {
	WK_WDT_LOC_TYPE,
	WK_WDT_EXT_TYPE,
	WK_WDT_LOC_TYPE_NOLOCK,
	WK_WDT_EXT_TYPE_NOLOCK,

};

#ifdef CONFIG_TOUCHSCREEN_TOUCH_FW_UPDATE
#define TOUCH_VENDOR "HIMAX"
#define FW_FLASH_TIMEOUT 200
struct touch_fwu_notifier himax_tp_notifier;
#endif


extern void mtk_wdt_restart(enum wk_wdt_type type);
void hx8526_kickdog(void *info)
{
	mtk_wdt_restart(WK_WDT_EXT_TYPE);
}
static bool i_Needupdate = true;
static unsigned char i_isTP_Updated = 0;
static int fw_size=0;
static unsigned char i_CTPM_FW[]=
{
	#include "LONGCHEER_CM620_Truly_T13_2015-05-22_1900.i"		
};
#endif

#define COMPARE_CTP_MODULE
#if defined(COMPARE_CTP_MODULE)
static unsigned char i_CTPM_FW_Tuly[]=
{
	#include "LONGCHEER_CM620_Truly_T13_2015-05-22_1900.i"
};

static unsigned char i_CTPM_FW_Yanghua[]=
{
	#include "LONGCHEER_CM620_Yanghua_V07_2015-05-22_1907.i"
};

static unsigned char i_CTPM_FW_Shenyue[]=
{
	#include "LONGCHEER_CM620_Shenyue_T01_2014-10-27_1347.i"
};

typedef enum hx8526_ctp
{
	HX8526_CTP_TRULY = 0,	
	HX8526_CTP_YANGHUA = 1,
	HX8526_CTP_SHENYUE = 2,
}HX8526_CTP_MODULE;

static HX8526_CTP_MODULE hx8526_ctp_module_name = HX8526_CTP_TRULY;
#endif

#define LCT_ADD_TP_VERSION       1
#if LCT_ADD_TP_VERSION
#define CTP_PROC_FILE "tp_info"

static int ctp_proc_read_show (struct seq_file* m, void* data)
{
	char temp[30] = {0};
	sprintf(temp, "pid:%s\n", CFG_VER_buff);
	seq_printf(m, "%s\n", temp);
	return 0;
}

static int ctp_proc_open (struct inode* inode, struct file* file)
{
    return single_open(file, ctp_proc_read_show, inode->i_private);
}

static const struct file_operations g_ctp_proc =
{
    .open = ctp_proc_open,
    .read = seq_read,
};
#endif


#ifdef HX_ESD_WORKAROUND
	static u8 ESD_RESET_ACTIVATE = 1;
	static u8 ESD_COUNTER = 0;
	static int ESD_COUNTER_SETTING = 3;
	unsigned char TOUCH_UP_COUNTER = 0;
	void ESD_HW_REST(void);
#endif

#ifdef PT_NUM_LOG
#define PT_ARRAY_SZ (200)
static int point_cnt_array[PT_ARRAY_SZ];
static int curr_ptr = 0;
#endif


#ifdef BUTTON_CHECK
static int bt_cnt = 0;
static int bt_confirm_cnt=40; 
static int obs_intvl = 200;	


#define POS_QUEUE_LEN   (8)
#define INVALID_POS        (0xffff)
#define LEAVE_POS      (0xfffe)
static struct t_pos_queue pos_queue[POS_QUEUE_LEN];
static int p_latest=1;
static int p_prev = 0;
static bool is_himax = true;

int pointFromAA(void)
{
    int i;
    int point_cnt = 0;
    int pt = p_prev;

    for(i=0;i<POS_QUEUE_LEN;i++)
    {
        pt = (p_prev+POS_QUEUE_LEN-i)%POS_QUEUE_LEN;

        if (unlikely(himax_debug_flag))
        {
            I("[himax] pos_queue check,p_prev = [%d], pt = [%d], pos =[%d] , timestamp=[%u]\n ",
                p_prev,pt,pos_queue[pt].pos,pos_queue[pt].timestamp);
        }

        if(pos_queue[pt].pos == INVALID_POS)   
            continue;

        if(jiffies - pos_queue[pt].timestamp >= (obs_intvl*HZ)/1000) 
            continue;

        if(pos_queue[pt].pos > 1000 && pos_queue[pt].pos < 1280)
            point_cnt +=1;
    }

    if (unlikely(himax_debug_flag))
    {
        I("[himax] pointFromAA, point_cnt = [%d]\n ",
            point_cnt);
    }

    if(point_cnt > 0)
        return 1;
    else
        return 0;
}

#endif

#define I2C_DMA_LIMIT 252
#define PAGE_SELECT_LEN (2)
#define SYN_I2C_RETRY_TIMES 10
static u8 *gpwDMABuf_va = NULL;
static u32 gpwDMABuf_pa = NULL;
static u8 *gprDMABuf_va = NULL;
static u32 gprDMABuf_pa = NULL;
struct i2c_msg *read_msg;
#define MASK_8BIT 0xFF

static int himax_i2c_read_data(struct i2c_client *i2c_client, uint8_t addr,uint8_t length,uint8_t *data)
{
#ifdef HX_MTK_DMA
	int retval;
	unsigned char retry;
	unsigned char buf;
	unsigned char *buf_va = NULL;
	int full = length / I2C_DMA_LIMIT;
	int partial = length % I2C_DMA_LIMIT;
	int total;
	int last;
	int ii;
	static int msg_length;

	mutex_lock(&tp_wr_access);
    gprDMABuf_va = (u8 *)dma_alloc_coherent(&i2c_client->dev, 4096, &gprDMABuf_pa, GFP_KERNEL);
    if(!gprDMABuf_va){
		E("Allocate DMA I2C Buffer failed!\n");
		mutex_unlock(&tp_wr_access);
		return -1;
    }

	buf_va = gprDMABuf_va;

	if ((full + 2) > msg_length) {
		kfree(read_msg);
		msg_length = full + 2;
		read_msg = kcalloc(msg_length, sizeof(struct i2c_msg), GFP_KERNEL);
	}

	read_msg[0].addr = i2c_client->addr;
	read_msg[0].flags = 0;
	read_msg[0].len = 1;
	read_msg[0].buf = &buf;
	read_msg[0].timing = 400;

	if (partial) {
		total = full + 1;
		last = partial;
	} else {
		total = full;
		last = I2C_DMA_LIMIT;
	}

	for (ii = 1; ii <= total; ii++) {
		read_msg[ii].addr = i2c_client->addr;
		read_msg[ii].flags = I2C_M_RD;
		read_msg[ii].len = (ii == total) ? last : I2C_DMA_LIMIT;
		read_msg[ii].buf = gprDMABuf_pa + I2C_DMA_LIMIT * (ii - 1);
		read_msg[ii].ext_flag = (i2c_client->ext_flag | I2C_ENEXT_FLAG | I2C_DMA_FLAG);
		read_msg[ii].timing = 400;
	}

	buf = addr & MASK_8BIT;
	for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(i2c_client->adapter, read_msg, (total + 1)) == (total + 1)) {
			retval = length;
			break;
		}
		dev_err(&i2c_client->dev,
				"%s: I2C retry %d\n",
				__func__, retry + 1);
		msleep(20);
	}

	if (retry == SYN_I2C_RETRY_TIMES) {
		dev_err(&i2c_client->dev,
				"%s: I2C read over retry limit\n",
				__func__);
		retval = -EIO;
	}

	memcpy(data, buf_va, length);

exit:
    if(gprDMABuf_va){
	    dma_free_coherent(&i2c_client->dev, 4096, gprDMABuf_va, gprDMABuf_pa);
	    gprDMABuf_va = NULL;
	    gprDMABuf_pa = NULL;
    }
	mutex_unlock(&tp_wr_access);

	return retval;
#else
	return i2c_smbus_read_i2c_block_data(i2c_client, command, len, buf);
#endif
}


static int himax_i2c_write_data(struct i2c_client *i2c_client, uint8_t addr,uint8_t length,uint8_t *data)
{
#ifdef HX_MTK_DMA
	int retval;
	unsigned char retry;
	unsigned char buf[length + 1];
	unsigned char *buf_va = NULL;

	mutex_lock(&tp_wr_access);
	gpwDMABuf_va = (u8 *)dma_alloc_coherent(&i2c_client->dev, 1024, &gpwDMABuf_pa, GFP_KERNEL);
    if(!gpwDMABuf_va){
		E("Allocate DMA I2C Buffer failed!\n");
		mutex_unlock(&tp_wr_access);
		return -1;
    }
	buf_va = gpwDMABuf_va;

	struct i2c_msg msg[] = {
		{
			.addr = i2c_client->addr,
			.flags = 0,
			.len = length + 1,
			.buf = gpwDMABuf_pa,
			.ext_flag=(i2c_client->ext_flag|I2C_ENEXT_FLAG|I2C_DMA_FLAG),
			.timing = 400,
		}
	};
	buf_va[0] = addr & MASK_8BIT;

	memcpy(&buf_va[1],&data[0] , length);

	for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(i2c_client->adapter, msg, 1) == 1) {
			retval = length;
			break;
		}
		dev_err(&i2c_client->dev,
				"%s: I2C retry %d\n",
				__func__, retry + 1);
		msleep(20);
	}

	if (retry == SYN_I2C_RETRY_TIMES) {
		dev_err(&i2c_client->dev,
				"%s: I2C write over retry limit\n",
				__func__);
		retval = -EIO;
	}

exit:

	if(gpwDMABuf_va){
                dma_free_coherent(&i2c_client->dev, 1024, gpwDMABuf_va, gpwDMABuf_pa);
                gpwDMABuf_va = NULL;
                gpwDMABuf_pa = NULL;
        }
	mutex_unlock(&tp_wr_access);

	return retval;
#else
	return i2c_smbus_write_i2c_block_data(i2c_client, command, len, buf);
#endif
}

int himax_ManualMode(int enter)
{
	uint8_t cmd[2];
	cmd[0] = enter;

	if( himax_i2c_write_data(i2c_client, 0x42, 1, &(cmd[0])) < 0 )
	{
		E("%s: i2c access fail!\n", __func__);
		return -1;
	}
	return 1;
}

int himax_FlashMode(int enter)
{
	uint8_t cmd[2];
	cmd[0] = enter;

	if( himax_i2c_write_data(i2c_client, 0x43, 1, &(cmd[0])) < 0 )
	{
		E("%s: i2c access fail!\n", __func__);
		return -1;
	}
	return 1;
}

static int himax_lock_flash(void)
{
	uint8_t cmd[5];

	
	cmd[0] = 0x01;cmd[1] = 0x00;cmd[2] = 0x06;
	if( himax_i2c_write_data(i2c_client, 0x43, 3, &(cmd[0])) < 0 )
	{
		E("%s: i2c access fail!\n", __func__);
		return -1;
	}

	cmd[0] = 0x03;cmd[1] = 0x00;cmd[2] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0 )
	{
		E("%s: i2c access fail!\n", __func__);
		return -1;
	}

	cmd[0] = 0x00;cmd[1] = 0x00;cmd[2] = 0x7D;cmd[3] = 0x03;
	if( himax_i2c_write_data(i2c_client, 0x45, 4, &(cmd[0])) < 0 )
	{
		E("%s: i2c access fail!\n", __func__);
		return -1;
	}

	if( himax_i2c_write_data(i2c_client, 0x4A, 0, &(cmd[0])) < 0 )
	{
		E("%s: i2c access fail!\n", __func__);
		return -1;
	}
	mdelay(50);
	return 1;
	
}

static int himax_unlock_flash(void)
{
	uint8_t cmd[5];

	
	cmd[0] = 0x01;cmd[1] = 0x00;cmd[2] = 0x06;
	if( himax_i2c_write_data(i2c_client, 0x43, 3, &(cmd[0])) < 0 )
	{
		E("%s: i2c access fail!\n", __func__);
		return -1;
	}

	cmd[0] = 0x03;cmd[1] = 0x00;cmd[2] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0 )
	{
		E("%s: i2c access fail!\n", __func__);
		return -1;
	}

	cmd[0] = 0x00;cmd[1] = 0x00;cmd[2] = 0x3D;cmd[3] = 0x03;
	if( himax_i2c_write_data(i2c_client, 0x45, 4, &(cmd[0])) < 0 )
	{
		E("%s: i2c access fail!\n", __func__);
		return -1;
	}

	if( himax_i2c_write_data(i2c_client, 0x4A, 0, &(cmd[0])) < 0 )
	{
		E("%s: i2c access fail!\n", __func__);
		return -1;
	}
	mdelay(50);

	return 1;
	
}
#ifdef HX_FLASH_TEST  
static void himax_changeIref(int selected_iref)
{
	unsigned char temp_iref[16][2] = {{0x00,0x00}, {0x00,0x00}, {0x00,0x00}, {0x00,0x00},
									  {0x00,0x00}, {0x00,0x00}, {0x00,0x00}, {0x00,0x00},
									  {0x00,0x00}, {0x00,0x00}, {0x00,0x00}, {0x00,0x00},
									  {0x00,0x00}, {0x00,0x00}, {0x00,0x00}, {0x00,0x00}};

	uint8_t cmd[10];

	int i = 0;
	int j = 0;

	for(i=0; i<16; i++)
	{
		for(j=0; j<2; j++)
		{
			if(selected_iref == 1)
			{
				temp_iref[i][j] = IrefTable_1[i][j];
			}
			else if(selected_iref == 2)
			{
				temp_iref[i][j] = IrefTable_2[i][j];
			}
			else if(selected_iref == 3)
			{
				temp_iref[i][j] = IrefTable_3[i][j];
			}
			else if(selected_iref == 4)
			{
				temp_iref[i][j] = IrefTable_4[i][j];
			}
			else if(selected_iref == 5)
			{
				temp_iref[i][j] = IrefTable_5[i][j];
			}
			else if(selected_iref == 6)
			{
				temp_iref[i][j] = IrefTable_6[i][j];
			}
			else if(selected_iref == 7)
			{
				temp_iref[i][j] = IrefTable_7[i][j];
			}
		}
	}

	if(!iref_found)
	{
		
		
		cmd[0] = 0x01;
		cmd[1] = 0x00;
		cmd[2] = 0x0A;
		if( himax_i2c_write_data(i2c_client, 0x43, 3, &cmd[0]) < 0)
		{
			E("%s: i2c access fail!\n", __func__);
			return;
		}

		
		cmd[0] = 0x00;
		cmd[1] = 0x00;
		cmd[2] = 0x00;
		if( himax_i2c_write_data(i2c_client, 0x44, 3, &cmd[0]) < 0)
		{
			E("%s: i2c access fail!\n", __func__);
			return ;
		}

		
		if( himax_i2c_write_data(i2c_client, 0x46, 0, &cmd[0]) < 0)
		{
			E("%s: i2c access fail!\n", __func__);
			return ;
		}

		
		if( himax_i2c_read_data(i2c_client, 0x59, 4, &(cmd[0])) < 0)
		{
			E("%s: i2c access fail!\n", __func__);
			return ;
		}

		
		for (i = 0; i < 16; i++)
		{
			if ((cmd[0] == IrefTable_3[i][0]) &&
					(cmd[1] == IrefTable_3[i][1]))
			{
				iref_number = i;
				iref_found = true;
				break;
			}
		}

		if(!iref_found )
		{
			E("%s: Can't find iref number!\n", __func__);
			return ;
		}
		else
		{
			I("%s: iref_number=%d, cmd[0]=0x%x, cmd[1]=0x%x\n", __func__, iref_number, cmd[0], cmd[1]);
		}
	}

	mdelay(5);
	
	
	cmd[0] = 0x01;
	cmd[1] = 0x00;
	cmd[2] = 0x06;
	if( himax_i2c_write_data(i2c_client, 0x43 , 3, &cmd[0]) < 0)
	{
		E("%s: i2c access fail!\n", __func__);
		return ;
	}

	
	cmd[0] = 0x00;
	cmd[1] = 0x00;
	cmd[2] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0x44, 3, &cmd[0]) < 0)
	{
		E("%s: i2c access fail!\n", __func__);
		return ;
	}

	
	cmd[0] = temp_iref[iref_number][0];
	cmd[1] = temp_iref[iref_number][1];
	cmd[2] = 0x27;
	cmd[3] = 0x27;
	if( himax_i2c_write_data(i2c_client, 0x45, 4, &cmd[0]) < 0)
	{
		E("%s: i2c access fail!\n", __func__);
		return ;
	}

	
	if( himax_i2c_write_data(i2c_client, 0x4A, 0, &cmd[0]) < 0)
	{
		E("%s: i2c access fail!\n", __func__);
		return ;
	}

	
	
	cmd[0] = 0x01;
	cmd[1] = 0x00;
	cmd[2] = 0x0A;
	if( himax_i2c_write_data(i2c_client, 0x43, 3, &cmd[0]) < 0)
	{
		E("%s: i2c access fail!\n", __func__);
		return ;
	}

	
	cmd[0] = 0x00;
	cmd[1] = 0x00;
	cmd[2] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0x44 , 3, &cmd[0]) < 0)
	{
		E("%s: i2c access fail!\n", __func__);
		return ;
	}

	
	if( himax_i2c_write_data(i2c_client, 0x46, 0, &cmd[0]) < 0)
	{
		E("%s: i2c access fail!\n", __func__);
		return ;
	}

	
	if( himax_i2c_read_data(i2c_client, 0x59, 4, &(cmd[0])) < 0)
	{
		E("%s: i2c access fail!\n", __func__);
		return ;
	}

	if(cmd[0] != temp_iref[iref_number][0] || cmd[1] != temp_iref[iref_number][1])
	{
		E("%s: IREF Read Back is not match.\n", __func__);
		E("%s: Iref [0]=%d,[1]=%d\n", __func__,cmd[0],cmd[1]);
	}
}
#endif
static bool himax_ic_package_check(void)
{
	uint8_t cmd[3];
	uint8_t data[3];

	himax_i2c_read_data(i2c_client, 0xD1, 3, &(cmd[0]));
	himax_i2c_read_data(i2c_client, 0x31, 3, &(data[0]));

	if((data[0] == 0x85 && data[1] == 0x28) || (cmd[0] == 0x04 && cmd[1] == 0x85 && (cmd[2] == 0x26 || cmd[2] == 0x27 || cmd[2] == 0x28)))
	{
		tpd_load_status = 1;
		IC_TYPE	= HX_85XX_D_SERIES_PWON;
		IC_CHECKSUM = HX_TP_BIN_CHECKSUM_CRC;
		
		FW_VER_MAJ_FLASH_ADDR = 133; 
		FW_VER_MAJ_FLASH_LENG = 1;
		FW_VER_MIN_FLASH_ADDR = 134; 
		FW_VER_MIN_FLASH_LENG = 1;
		CFG_VER_MAJ_FLASH_ADDR = 160; 
		CFG_VER_MAJ_FLASH_LENG = 12;
		CFG_VER_MIN_FLASH_ADDR = 172; 
		CFG_VER_MIN_FLASH_LENG = 12;

		I("[Himax] IC package 8528 D\n");
	}
	else if((data[0] == 0x85 && data[1] == 0x23) || (cmd[0] == 0x03 && cmd[1] == 0x85 && (cmd[2] == 0x26 || cmd[2] == 0x27 || cmd[2] == 0x28 || cmd[2] == 0x29)))
	{
		tpd_load_status = 1;
		IC_TYPE = HX_85XX_C_SERIES_PWON;
		IC_CHECKSUM = HX_TP_BIN_CHECKSUM_SW;
		
		FW_VER_MAJ_FLASH_ADDR = 133; 
		FW_VER_MAJ_FLASH_LENG = 1;
		FW_VER_MIN_FLASH_ADDR = 134; 
		FW_VER_MIN_FLASH_LENG = 1;
		CFG_VER_MAJ_FLASH_ADDR = 135; 
		CFG_VER_MAJ_FLASH_LENG = 12;
		CFG_VER_MIN_FLASH_ADDR = 147; 
		CFG_VER_MIN_FLASH_LENG = 12;

		I("[Himax] IC package 8523 C\n");
	}
	else if ((data[0] == 0x85 && data[1] == 0x26) || (cmd[0] == 0x02 && cmd[1] == 0x85 && (cmd[2] == 0x19 || cmd[2] == 0x25 || cmd[2] == 0x26)))
	{
		tpd_load_status = 1;
		IC_TYPE = HX_85XX_B_SERIES_PWON;
		IC_CHECKSUM = HX_TP_BIN_CHECKSUM_SW;
		
		FW_VER_MAJ_FLASH_ADDR = 133; 
		FW_VER_MAJ_FLASH_LENG = 1;
		FW_VER_MIN_FLASH_ADDR = 728; 
		FW_VER_MIN_FLASH_LENG = 1;
		CFG_VER_MAJ_FLASH_ADDR = 692; 
		CFG_VER_MAJ_FLASH_LENG = 3;
		CFG_VER_MIN_FLASH_ADDR = 704; 
		CFG_VER_MIN_FLASH_LENG = 3;

		I("[Himax] IC package 8526 B\n");
	}
	else if ((data[0] == 0x85 && data[1] == 0x20) || (cmd[0] == 0x01 && cmd[1] == 0x85 && cmd[2] == 0x19))
	{
		tpd_load_status = 1;
		IC_TYPE = HX_85XX_A_SERIES_PWON;
		IC_CHECKSUM = HX_TP_BIN_CHECKSUM_SW;
		I("[Himax] IC package 8520 A\n");
	}
	else
	{
		tpd_load_status = 0;
		I("[Himax] IC package incorrect!!\n");
		return false;
	}

	return true;
}

u8 himax_read_FW_ver(void)
{
	u16 fw_ver_maj_start_addr;
	u16 fw_ver_maj_end_addr;
	u16 fw_ver_maj_addr;
	u16 fw_ver_maj_length;

	u16 fw_ver_min_start_addr;
	u16 fw_ver_min_end_addr;
	u16 fw_ver_min_addr;
	u16 fw_ver_min_length;

	u16 cfg_ver_maj_start_addr;
	u16 cfg_ver_maj_end_addr;
	u16 cfg_ver_maj_addr;
	u16 cfg_ver_maj_length;

	u16 cfg_ver_min_start_addr;
	u16 cfg_ver_min_end_addr;
	u16 cfg_ver_min_addr;
	u16 cfg_ver_min_length;

	uint8_t cmd[3];
	u16 i = 0;
	u16 j = 0;
	u16 k = 0;

	fw_ver_maj_start_addr 	= FW_VER_MAJ_FLASH_ADDR / 4;                            
	fw_ver_maj_length       = FW_VER_MAJ_FLASH_LENG;                                
	fw_ver_maj_end_addr     = (FW_VER_MAJ_FLASH_ADDR + fw_ver_maj_length ) / 4 + 1;	
	fw_ver_maj_addr         = FW_VER_MAJ_FLASH_ADDR % 4;                            

	fw_ver_min_start_addr   = FW_VER_MIN_FLASH_ADDR / 4;                            
	fw_ver_min_length       = FW_VER_MIN_FLASH_LENG;                                
	fw_ver_min_end_addr     = (FW_VER_MIN_FLASH_ADDR + fw_ver_min_length ) / 4 + 1;	
	fw_ver_min_addr         = FW_VER_MIN_FLASH_ADDR % 4;                            

	cfg_ver_maj_start_addr  = CFG_VER_MAJ_FLASH_ADDR / 4;                             
	cfg_ver_maj_length      = CFG_VER_MAJ_FLASH_LENG;                                 
	cfg_ver_maj_end_addr    = (CFG_VER_MAJ_FLASH_ADDR + cfg_ver_maj_length ) / 4 + 1; 
	cfg_ver_maj_addr        = CFG_VER_MAJ_FLASH_ADDR % 4;                             

	cfg_ver_min_start_addr  = CFG_VER_MIN_FLASH_ADDR / 4;                             
	cfg_ver_min_length      = CFG_VER_MIN_FLASH_LENG;                                 
	cfg_ver_min_end_addr    = (CFG_VER_MIN_FLASH_ADDR + cfg_ver_min_length ) / 4 + 1; 
	cfg_ver_min_addr        = CFG_VER_MIN_FLASH_ADDR % 4;                             

	mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
	irq_enable = false;

#ifdef HX_RST_PIN_FUNC
	himax_HW_reset();
#endif

	

	if( himax_i2c_write_data(i2c_client, 0x81, 0, &(cmd[0])) < 0)
	{
		E("%s: i2c access fail!\n", __func__);
		return -1;
	}
	mdelay(120);

	
	himax_FlashMode(1);

	
	
	i = fw_ver_maj_start_addr;
	do
	{
		cmd[0] = i & 0x1F;         
		cmd[1] = (i >> 5) & 0x1F;  
		cmd[2] = (i >> 10) & 0x1F; 

		if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0)
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_write_data(i2c_client, 0x46, 0, &(cmd[0])) < 0)
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_read_data(i2c_client, 0x59, 4, &(cmd[0])) < 0)
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		if(i == fw_ver_maj_start_addr) 
		{
			j = 0;
			for( k = fw_ver_maj_addr; k < 4 && j < fw_ver_maj_length; k++)
			{
				FW_VER_MAJ_buff[j++] = cmd[k];
			}
		}
		else 
		{
			for( k = 0; k < 4 && j < fw_ver_maj_length; k++)
			{
				FW_VER_MAJ_buff[j++] = cmd[k];
			}
		}
		i++;
	}
	while(i < fw_ver_maj_end_addr);

	
	i = fw_ver_min_start_addr;
	do
	{
		cmd[0] = i & 0x1F;         
		cmd[1] = (i >> 5) & 0x1F;  
		cmd[2] = (i >> 10) & 0x1F; 

		if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0)
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}


		if( himax_i2c_write_data(i2c_client, 0x46, 0, &(cmd[0])) < 0)
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_read_data(i2c_client, 0x59, 4, &(cmd[0])) < 0)
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		if(i == fw_ver_min_start_addr) 
		{
			j = 0;
			for(k = fw_ver_min_addr; k < 4 && j < fw_ver_min_length; k++)
			{
				FW_VER_MIN_buff[j++] = cmd[k];
			}
		}
		else 
		{
			for(k = 0; k < 4 && j < fw_ver_min_length; k++)
			{
				FW_VER_MIN_buff[j++] = cmd[k];
			}
		}
		i++;
	}while(i < fw_ver_min_end_addr);


	
	i = cfg_ver_maj_start_addr;
	do
	{
		cmd[0] = i & 0x1F;         
		cmd[1] = (i >> 5) & 0x1F;  
		cmd[2] = (i >> 10) & 0x1F; 

		if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0)
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_write_data(i2c_client, 0x46, 0, &(cmd[0])) < 0)
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_read_data(i2c_client, 0x59, 4, &(cmd[0])) < 0)
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		if(i == cfg_ver_maj_start_addr) 
		{
			j = 0;
			for( k = cfg_ver_maj_addr; k < 4 && j < cfg_ver_maj_length; k++)
			{
				CFG_VER_MAJ_buff[j++] = cmd[k];
			}
		}
		else 
		{
			for(k = 0; k < 4 && j < cfg_ver_maj_length; k++)
			{
				CFG_VER_MAJ_buff[j++] = cmd[k];
			}
		}
		i++;
	}
	while(i < cfg_ver_maj_end_addr);

	
	i = cfg_ver_min_start_addr;
	do
	{
		cmd[0] = i & 0x1F;         
		cmd[1] = (i >> 5) & 0x1F;  
		cmd[2] = (i >> 10) & 0x1F; 

		if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0)
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_write_data(i2c_client, 0x46, 0, &(cmd[0])) < 0)
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}


		if( himax_i2c_read_data(i2c_client, 0x59, 4, &(cmd[0])) < 0)
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}
		if(i == cfg_ver_min_start_addr) 
		{
			j = 0;
			for(k = cfg_ver_min_addr; k < 4 && j < cfg_ver_min_length; k++)
			{
				CFG_VER_MIN_buff[j++] = cmd[k];
			}
		}
		else 
		{
			for(k = 0; k < 4 && j < cfg_ver_min_length; k++)
			{
				CFG_VER_MIN_buff[j++] = cmd[k];
			}
		}
		i++;
	}
	while(i < cfg_ver_min_end_addr);

	
	himax_FlashMode(0);


	I("FW_VER_MAJ_buff : %d \n",FW_VER_MAJ_buff[0]);
	I("FW_VER_MIN_buff : %d \n",FW_VER_MIN_buff[0]);

	I("CFG_VER_MAJ_buff : ");
	for(i=0; i<12; i++)
	{
		I(" %d ,",CFG_VER_MAJ_buff[i]);
	}
	printk("\n");

	I("CFG_VER_MIN_buff : ");
	for(i=0; i<12; i++)
	{
		I(" %d ,",CFG_VER_MIN_buff[i]);
	}


	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	irq_enable = true;
	return 0;
}

#ifdef Himax_fcover

static int sensitivity_state = -1;
static int tpd_hs_enable = 0xFF;

static int himax_cover_enable(int arg) 
{

	I("%s in, state = %d\n",__func__,arg);
	int state = 0;
	int state_compare = 0;
	int i = 0;
	int ret = 0;
	if(arg){
		sensitivity_state = 1;
		state = 0x04;
	}else{
		sensitivity_state = 0;
		state = 0x00;
	}
	mutex_lock(&i2c_access);
	if ((ret = himax_i2c_write_data(i2c_client, 0x92, 1, &state)) < 0)
		TPD_DEBUG("%s i2c write error: %d\n", __func__, ret);
	msleep(2);
	if ((ret = himax_i2c_write_data(i2c_client, 0x92, 1, &state)) < 0)
		TPD_DEBUG("%s i2c write error: %d\n", __func__, ret);
	msleep(2);
	mutex_unlock(&i2c_access);
	msleep(1);

	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	irq_enable = true;
	return 0;
}
int tpd_enable_high_Sensitivity_exe(int enable)
{
	himax_cover_enable(enable);
	return 0;
}
int tpd_enable_high_Sensitivity(int enable)
{
	tpd_hs_enable = enable;
	tpd_eint_interrupt_handler();
}
EXPORT_SYMBOL_GPL(tpd_enable_high_Sensitivity);
#endif


#ifdef HX_LOADIN_CONFIG
static int himax_config_flow()
{
	char data[4];
	uint8_t buf0[4];
	int i2c_CheckSum = 0;
	unsigned long i = 0;

	data[0] = 0xE3;
	data[1] = 0x00;	

	if( himax_i2c_write_data(i2c_client, &data[0], 1, &(data[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c1[0], sizeof(c1)-1, &(c1[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c2[0], sizeof(c2)-1, &(c2[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c3[0], sizeof(c3)-1, &(c3[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c4[0], sizeof(c4)-1, &(c4[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c5[0], sizeof(c5)-1, &(c5[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c6[0], sizeof(c6)-1, &(c6[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c7[0], sizeof(c7)-1, &(c7[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c8[0], sizeof(c8)-1, &(c8[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c9[0], sizeof(c9)-1, &(c9[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c10[0], sizeof(c10)-1, &(c10[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c11[0], sizeof(c11)-1, &(c11[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c12[0], sizeof(c12)-1, &(c12[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c13[0], sizeof(c13)-1, &(c13[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c14[0], sizeof(c14)-1, &(c14[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c15[0], sizeof(c15)-1, &(c15[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c16[0], sizeof(c16)-1, &(c16[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c17[0], sizeof(c17)-1, &(c17[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c18[0], sizeof(c18)-1, &(c18[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c19[0], sizeof(c19)-1, &(c19[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c20[0], sizeof(c20)-1, &(c20[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c21[0], sizeof(c21)-1, &(c21[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c22[0], sizeof(c22)-1, &(c22[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c23[0], sizeof(c23)-1, &(c23[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c24[0], sizeof(c24)-1, &(c24[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c25[0], sizeof(c25)-1, &(c25[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c26[0], sizeof(c26)-1, &(c26[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c27[0], sizeof(c27)-1, &(c27[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c28[0], sizeof(c28)-1, &(c28[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c29[0], sizeof(c29)-1, &(c29[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c30[0], sizeof(c30)-1, &(c30[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c31[0], sizeof(c31)-1, &(c31[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c32[0], sizeof(c32)-1, &(c32[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c33[0], sizeof(c33)-1, &(c33[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c34[0], sizeof(c34)-1, &(c34[1])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c35[0], sizeof(c35)-1, &(c35[1])) < 0 )

	{
		goto HimaxErr;
	}


	
	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xAB, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x01;
	if( himax_i2c_write_data(i2c_client, 0xAB, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	data[1] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c36[0], sizeof(c36)-1, &(c36[1])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	for( i=0 ; i<sizeof(c36) ; i++ )
	{
		i2c_CheckSum += c36[i];
	}
	I("Himax i2c_checksum_36_size = %d \n",sizeof(c36));
	I("Himax i2c_checksum_36 = %d \n",i2c_CheckSum);

	i2c_CheckSum += 0x2AF;

	I("Himax i2c_checksum_36 = %d \n",i2c_CheckSum);
	


	
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	data[1] = 0x1E;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c37[0], sizeof(c37)-1, &(c37[1])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	for( i=0 ; i<sizeof(c37) ; i++ )
	{
		i2c_CheckSum += c37[i];
	}
	I("Himax i2c_checksum_37_size = %d \n",sizeof(c37));
	I("Himax i2c_checksum_37 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x2CD;
	I("Himax i2c_checksum_37 = %d \n",i2c_CheckSum);
	

	
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	data[1] = 0x3C;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c38[0], sizeof(c38)-1, &(c38[1])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	for( i=0 ; i<sizeof(c38) ; i++ )
	{
		i2c_CheckSum += c38[i];
	}
	I("Himax i2c_checksum_38_size = %d \n",sizeof(c38));
	I("Himax i2c_checksum_38 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x2EB;
	I("Himax i2c_checksum_38 = %d \n",i2c_CheckSum);
	

	
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	data[1] = 0x4C;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c39[0], sizeof(c39)-1, &(c39[1])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	for( i=0 ; i<sizeof(c39) ; i++ )
	{
		i2c_CheckSum += c39[i];
	}
	I("Himax i2c_checksum_39_size = %d \n",sizeof(c39));
	I("Himax i2c_checksum_39 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x2FB;
	I("Himax i2c_checksum_39 = %d \n",i2c_CheckSum);
	

	
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	data[1] = 0x64;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c40[0], sizeof(c40)-1, &(c40[1])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	for( i=0 ; i<sizeof(c40) ; i++ )
	{
		i2c_CheckSum += c40[i];
	}
	I("Himax i2c_checksum_40_size = %d \n",sizeof(c40));
	I("Himax i2c_checksum_40 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x313;
	I("Himax i2c_checksum_40 = %d \n",i2c_CheckSum);
	

	
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	data[1] = 0x7A;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c41[0], sizeof(c41)-1, &(c41[1])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	for( i=0 ; i<sizeof(c41) ; i++ )
	{
		i2c_CheckSum += c41[i];
	}
	I("Himax i2c_checksum_41_size = %d \n",sizeof(c41));
	I("Himax i2c_checksum_41 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x329;
	I("Himax i2c_checksum_41 = %d \n",i2c_CheckSum);
	

	
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	data[1] = 0x96;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c42[0], sizeof(c42)-1, &(c42[1])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	for( i=0 ; i<sizeof(c42) ; i++ )
	{
		i2c_CheckSum += c42[i];
	}
	I("Himax i2c_checksum_42_size = %d \n",sizeof(c42));
	I("Himax i2c_checksum_42 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x345;
	I("Himax i2c_checksum_42 = %d \n",i2c_CheckSum);
	

	
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	data[1] = 0x9E;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c43_1[0], sizeof(c43_1)-1, &(c43_1[1])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	for( i=0 ; i<sizeof(c43_1) ; i++ )
	{
		i2c_CheckSum += c43_1[i];
	}
	I("Himax i2c_checksum_43_1_size = %d \n",sizeof(c43_1));
	I("Himax i2c_checksum_43_1 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x34D;
	I("Himax i2c_checksum_43_1 = %d \n",i2c_CheckSum);
	

	
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	data[1] = 0xBD;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c43_2[0], sizeof(c43_2)-1, &(c43_2[0])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	for( i=0 ; i<sizeof(c43_2) ; i++ )
	{
		i2c_CheckSum += c43_2[i];
	}
	I("Himax i2c_checksum_43_2_size = %d \n",sizeof(c43_2));
	I("Himax i2c_checksum_43_2 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x36C;
	I("Himax i2c_checksum_43_2 = %d \n",i2c_CheckSum);
	

	
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	data[1] = 0xDA;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	
	if( himax_i2c_write_data(i2c_client, c44_1[0], sizeof(c44_1)-1, &(c44_1[0])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	for( i=0 ; i<sizeof(c44_1) ; i++ )
	{
		i2c_CheckSum += c44_1[i];
	}
	I("Himax i2c_checksum_44_1_size = %d \n",sizeof(c44_1));
	I("Himax i2c_checksum_44_1 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x389;
	I("Himax i2c_checksum_44_1 = %d \n",i2c_CheckSum);
	

	
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	data[1] = 0xF9;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c44_2[0], sizeof(c44_2)-1, &(c44_2[1])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	for( i=0 ; i<sizeof(c44_2) ; i++ )
	{
		i2c_CheckSum += c44_2[i];
	}
	I("Himax i2c_checksum_44_2_size = %d \n",sizeof(c44_2));
	I("Himax i2c_checksum_44_2 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x3A8;
	I("Himax i2c_checksum_44_2 = %d \n",i2c_CheckSum);
	

	
	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	data[1] = 0xFE;
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	if( himax_i2c_write_data(i2c_client, c45[0], sizeof(c45)-1, &(c45[1])) < 0 )
	{
		goto HimaxErr;
	}

	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	for( i=0 ; i<sizeof(c45) ; i++ )
	{
		i2c_CheckSum += c45[i];
	}
	I("Himax i2c_checksum_45_size = %d \n",sizeof(c45));
	I("Himax i2c_checksum_45 = %d \n",i2c_CheckSum);
	i2c_CheckSum += 0x3AD;
	I("Himax i2c_checksum_45 = %d \n",i2c_CheckSum);
	

	data[0] = 0x10;
	if( himax_i2c_write_data(i2c_client, 0xAB, 1, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	i2c_CheckSum += 0xAB;
	i2c_CheckSum += 0x10;

	I("Himax i2c_checksum_Final = %d \n",i2c_CheckSum);

	data[0] = i2c_CheckSum & 0xFF;
	data[1] = (i2c_CheckSum >> 8) & 0xFF;
	if( himax_i2c_write_data(i2c_client, 0xAC, 2, &(data[0])) < 0 )
	{
		goto HimaxErr;
	}

	I("Himax i2c_checksum_AC = %d , %d \n",data[1],data[2]);

	int ret = himax_i2c_read_data(i2c_client, 0xAB, 2, &(buf0[0]));
	if(ret < 0)
	{
		E("[Himax]:himax_i2c_read_data 0xDA failed line: %d \n",__LINE__);
		goto HimaxErr;
	}

	if(buf0[0] == 0x18)
	{
		return -1;
	}

	if(buf0[0] == 0x10)
	{
		return 1;
	}

	return 1;
	HimaxErr:
	return -1;
}
#endif

#ifdef HX_FW_UPDATE_BY_I_FILE
int fts_ctpm_fw_upgrade_with_i_file(void)
{
	unsigned char* ImageBuffer;
	int fullFileLength = sizeof(i_CTPM_FW);
	int i, j;
	uint8_t cmd[5], last_byte, prePage;
	int FileLength;
	uint8_t checksumResult = 0;

	I("himax,[TP] %s\n", __func__);
#if defined(COMPARE_CTP_MODULE)
	if(HX8526_CTP_TRULY == hx8526_ctp_module_name)
	{
		ImageBuffer = i_CTPM_FW_Tuly;
		I("himax,[TP] %s, HX8526_CTP_TRULY\n", __func__);
	}
	else if(HX8526_CTP_YANGHUA == hx8526_ctp_module_name)
	{
		ImageBuffer = i_CTPM_FW_Yanghua;
		I("himax,[TP] %s, HX8526_CTP_YANGHUA\n", __func__);
	}
	else if(HX8526_CTP_SHENYUE == hx8526_ctp_module_name)
	{
		ImageBuffer = i_CTPM_FW_Shenyue;
		I("himax,[TP] %s, HX8526_CTP_SHENYUE\n", __func__);
	}
	else
	{
		ImageBuffer = i_CTPM_FW_Tuly;		
	}
#else
		ImageBuffer = i_CTPM_FW;
#endif

	I("himax,[TP] %s, fullFileLength=%d\n", __func__, fullFileLength);

	
	for (j = 0; j < 3; j++)
	{
		if(IC_CHECKSUM == HX_TP_BIN_CHECKSUM_CRC)
		{
			FileLength = fullFileLength;
		}
		else
		{
			FileLength = fullFileLength - 2;
		}

		#ifdef HX_RST_PIN_FUNC
		himax_HW_reset();
		#endif

		if( himax_i2c_write_data(i2c_client, 0x81, 0, &(cmd[0])) < 0 )
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		mdelay(120);

		himax_unlock_flash(); 

		cmd[0] = 0x05;
		cmd[1] = 0x00;
		cmd[2] = 0x02;
		if( himax_i2c_write_data(i2c_client, 0x43, 3, &(cmd[0])) < 0 )
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_write_data(i2c_client, 0x4F, 0, &(cmd[0])) < 0 )
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}
		mdelay(50);

		himax_ManualMode(1);
		himax_FlashMode(1);

		FileLength = (FileLength + 3) / 4;
		for (i = 0, prePage = 0; i < FileLength; i++)
		{
			last_byte = 0;

			cmd[0] = i & 0x1F;
			if (cmd[0] == 0x1F || i == FileLength - 1)
			{
				last_byte = 1;
			}
			cmd[1] = (i >> 5) & 0x1F;
			cmd[2] = (i >> 10) & 0x1F;
			if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0 )
			{
				E("%s: i2c access fail!\n", __func__);
				return -1;
			}

			if (prePage != cmd[1] || i == 0)
			{
				prePage = cmd[1];

				cmd[0] = 0x01;
				cmd[1] = 0x09;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					E("%s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;
				cmd[1] = 0x0D;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					E("%s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;
				cmd[1] = 0x09;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					E("%s: i2c access fail!\n", __func__);
					return -1;
				}
			}

			memcpy(&cmd[0], &ImageBuffer[4*i], 4);
			if( himax_i2c_write_data(i2c_client, 0x45, 4, &(cmd[0])) < 0 )
			{
				E("%s: i2c access fail!\n", __func__);
				return -1;
			}

			cmd[0] = 0x01;
			cmd[1] = 0x0D;
			if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
			{
				E("%s: i2c access fail!\n", __func__);
				return -1;
			}

			cmd[0] = 0x01;
			cmd[1] = 0x09;
			if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
			{
				E("%s: i2c access fail!\n", __func__);
				return -1;
			}

			if (last_byte == 1)
			{
				cmd[0] = 0x01;
				cmd[1] = 0x01;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{

					E("%s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;
				cmd[1] = 0x05;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					E("%s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;
				cmd[1] = 0x01;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					E("%s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;
				cmd[1] = 0x00;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					E("%s: i2c access fail!\n", __func__);
					return -1;
				}

				mdelay(10);
				if (i == (FileLength - 1))
				{
					himax_FlashMode(0);
					himax_ManualMode(0);
#ifdef HX_FLASH_TEST 
        			on_each_cpu((smp_call_func_t)hx8526_kickdog,NULL,0);
					
					himax_changeIref(3);
					checksumResult = himax_calculateChecksum(ImageBuffer, fullFileLength);

					if(checksumResult == 1)
					{
						on_each_cpu((smp_call_func_t)hx8526_kickdog,NULL,0);
						
						himax_changeIref(5);
						checksumResult = himax_calculateChecksum(ImageBuffer, fullFileLength);

						if(checksumResult == 1)
						{
							on_each_cpu((smp_call_func_t)hx8526_kickdog,NULL,0);
							
							himax_changeIref(2);
							checksumResult = himax_calculateChecksum(ImageBuffer, fullFileLength);
						}
					}
#else
					checksumResult = himax_calculateChecksum(ImageBuffer, fullFileLength);
#endif
					himax_lock_flash();

					if (checksumResult>0) 
					{
						return 1;
					}
					else 
					{
#ifdef HX_FLASH_TEST
						flash_checksum_pass = false;
						
#endif
						E("%s: checksum calculate fail , retry j = %d !\n", __func__, j);
						return 0;

					}
				}
			}
		}
	}
	
}




static int hx8526_read_flash(unsigned char *buf, unsigned int addr_start, unsigned int length) 
{
	u16 i;
	unsigned int j = 0;
	unsigned char add_buf[4];

	for (i = addr_start; i < addr_start+length; i++)
	{
		add_buf[0] = i & 0x1F;
		add_buf[1] = (i >> 5) & 0x1F;
		add_buf[2] = (i >> 10) & 0x1F;

		if((i2c_smbus_write_i2c_block_data(i2c_client, 0x44, 3, &add_buf[0]))< 0){
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}
		udelay(10);

		if((i2c_smbus_write_i2c_block_data(i2c_client, 0x46, 0, &add_buf[0]))< 0){
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}
		udelay(10);
		if((i2c_smbus_read_i2c_block_data(i2c_client, 0x59, 4, &buf[0+j]))< 0){
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}
		udelay(10);
		j=j+4;
	}

	return 1;
}


static int Check_FW_Version(void)
{
	int tp_ver, i_file_ver;
	unsigned char FW_VER_MAJ_FLASH_buff[FW_VER_MAJ_FLASH_LENG];
	unsigned char FW_VER_MIN_FLASH_buff[FW_VER_MIN_FLASH_LENG];
	unsigned char CFG_VER_MAJ_FLASH_buff[CFG_VER_MAJ_FLASH_LENG];
	
	unsigned char fw_maj_addr;
	unsigned char fw_min_addr;
	unsigned char cfg_maj_addr;
	unsigned char cfg_min_addr;
	unsigned int i;
	unsigned char cmd[5];
	unsigned char* ImageBuffer;

#if defined(COMPARE_CTP_MODULE)
	if(HX8526_CTP_TRULY == hx8526_ctp_module_name)
	{
		ImageBuffer = i_CTPM_FW_Tuly;
	}
	else if(HX8526_CTP_YANGHUA == hx8526_ctp_module_name)
	{
		ImageBuffer = i_CTPM_FW_Yanghua;
	}
	else if(HX8526_CTP_SHENYUE == hx8526_ctp_module_name)
	{
		ImageBuffer = i_CTPM_FW_Shenyue;
	}
	else
	{
		ImageBuffer = i_CTPM_FW_Tuly;		
	}
#else
		ImageBuffer = i_CTPM_FW;
#endif

        fw_maj_addr=FW_VER_MAJ_FLASH_ADDR/4;
	fw_min_addr=FW_VER_MIN_FLASH_ADDR/4;
	cfg_maj_addr=CFG_VER_MAJ_FLASH_ADDR/4;
    	cfg_min_addr=CFG_VER_MIN_FLASH_ADDR/4;
	cmd[0] =0x02;
	if((i2c_smbus_write_i2c_block_data(i2c_client, 0x42, 1, &cmd[0]))< 0)
	{
		E("himax, Check_FW_Version 0x42 fail!\n");
		return -1;
	}
	mdelay(1);

	if((i2c_smbus_write_i2c_block_data(i2c_client, 0x81, 0, &cmd[0]))< 0)
	{
		E("@line %d: himax, FW_Version fail!", __LINE__);
		return -1;
	}
	mdelay(120);

	
	cmd[0] = 0x01;	cmd[1] = 0x00;	cmd[2] = 0x02;
	if((i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]))< 0)
	{
		E("himax, Check_FW_Version 0x43 fail!\n");
		goto err;
	}
	
	mdelay(1);
	if (hx8526_read_flash(CFG_VER_MAJ_FLASH_buff, cfg_maj_addr, CFG_VER_MAJ_FLASH_LENG/4) < 0)	
		goto err;
	if (hx8526_read_flash(CFG_VER_MIN_FLASH_buff, cfg_min_addr, CFG_VER_MIN_FLASH_LENG/4) < 0)	
		goto err;

	
	cmd[0] = 0x00;	cmd[1] = 0x00;	cmd[2] = 0x02;
	if((i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]))< 0)
	{
		E("@line %d: himax, FW_Version i2c fail", __LINE__);
		udelay(10);
		return -1;
	}

	#ifdef GN_MTK_BSP_DEVICECHECK
    	struct gn_device_info gn_mydev_info;
    	gn_mydev_info.gn_dev_type = GN_DEVICE_TYPE_TP;
    	sprintf(gn_mydev_info.name,"HX_%s_ofilm",CFG_VER_MIN_FLASH_buff);
    	gn_set_device_info(gn_mydev_info);
	#endif

	if(sscanf(&(CFG_VER_MIN_FLASH_buff[1]), "%x", &tp_ver) == 0)
    {
        E("himax, ++++tp_fw=0x%x\n", tp_ver);
    	return -1;
   	}

	if(sscanf(ImageBuffer + CFG_VER_MIN_FLASH_ADDR  + 1, "%x", &i_file_ver) == 0)
    {
        E("himax,++++i_file_ver=0x%x\n", tp_ver);
        return -1;
    }

	g_fw_verion = tp_ver;
	g_ifile_version = i_file_ver;
	I("himax, ++++tp_fw=0x%x, i_file_ver=0x%x\n", tp_ver, i_file_ver);

#if defined(COMPARE_CTP_MODULE)
	if(((strcmp(CFG_VER_MAJ_FLASH_buff, "Truly") == 0) && (0 == hx8526_ctp_module_name))||((strcmp(CFG_VER_MAJ_FLASH_buff, "Yanghua") == 0) && (1 == hx8526_ctp_module_name)) || ((strcmp(CFG_VER_MAJ_FLASH_buff, "Shenyue") == 0) && (2 == hx8526_ctp_module_name)))
	{
		if ( tp_ver < i_file_ver)	
		{
			I("himax, Check_FW_Version, CTP firmware with lower version number detected\n");
			return 1;
		}
		else if (tp_ver > i_file_ver)	
		{
			I("himax, Check_FW_Version, disable firmware upgrade procedure with i-file\n");
			return -1;
		}
		else
		{
			I("himax, Check_FW_Version, firmware upgrade procedure depends on checksume data\n");
			return 0;					
		}
	}
	else
	{
		return 1;
	}
#else
	if ( tp_ver < i_file_ver)	
	{
		I("himax, Check_FW_Version, CTP firmware with lower version number detected\n");
		return 1;
	}
	else if (tp_ver > i_file_ver)	
	{
		I("himax, Check_FW_Version, disable firmware upgrade procedure with i-file\n");
		return -1;
	}
	else
	{
		I("himax, Check_FW_Version, firmware upgrade procedure depends on checksume data\n");
		return 0;					
	}
#endif

#if 0
	for (i = 0; i < CFG_VER_MIN_FLASH_LENG ; i++)
	{
		I("himax ++++read FW_VER buff[%d]=0x%x,i_file=0x%x\n",i,CFG_VER_MIN_FLASH_buff[i],*(ImageBuffer + (CFG_VER_MIN_FLASH_ADDR ) + i));
		if (CFG_VER_MIN_FLASH_buff[i] != *(ImageBuffer + (CFG_VER_MIN_FLASH_ADDR) + i))
		{
			I("himax, CFG_VER_MIN_FLASH_buff\n");
			return 1;
		}
	}
#endif
	return 0;

	err:
		E("Himax TP[%s]:[%d] read FW version error exit\n", __func__, __LINE__);
		return -1;
}

#if 1	
static void Hx_Read_FW_Version(void)
{
	unsigned char cfg_min_addr;
	unsigned char cfg_maj_addr;
	unsigned char cmd[5];

	cfg_min_addr=CFG_VER_MIN_FLASH_ADDR/4;
	cfg_maj_addr=CFG_VER_MAJ_FLASH_ADDR/4;
	cmd[0] =0x02;
	if((i2c_smbus_write_i2c_block_data(i2c_client, 0x42, 1, &cmd[0]))< 0)
	{
		E("himax, Check_FW_Version 0x42 fail!\n");
	}
	mdelay(1);

	if((i2c_smbus_write_i2c_block_data(i2c_client, 0x81, 0, &cmd[0]))< 0)
	{
		E("@line %d: himax, FW_Version fail!\n", __LINE__);
	}
	mdelay(120);

	
	cmd[0] = 0x01;	cmd[1] = 0x00;	cmd[2] = 0x02;
	if((i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]))< 0)
	{
		E("himax, Check_FW_Version 0x43 fail!\n");
	}
	mdelay(1);

	hx8526_read_flash(CFG_Module_buff, cfg_maj_addr, CFG_VER_MAJ_FLASH_LENG/4);	
	hx8526_read_flash(CFG_VER_buff, cfg_min_addr, CFG_VER_MIN_FLASH_LENG/4);

	
	cmd[0] = 0x00;	cmd[1] = 0x00;	cmd[2] = 0x02;
	if((i2c_smbus_write_i2c_block_data(i2c_client, 0x43, 3, &cmd[0]))< 0)
	{
		E("@line %d: himax, FW_Version i2c fail!\n", __LINE__);
	}
}
#endif

static int himax_read_FW_checksum(void)  
{
	int fullFileLength = sizeof(i_CTPM_FW); 
	
	u16 FLASH_VER_START_ADDR =1030;
	u16 FW_VER_END_ADDR = 4120;
	u16 i, j, k,m;
	unsigned char cmd[4];
	int ret;
	u8 fail_count = 0;
	unsigned char* ImageBuffer;

#if defined(COMPARE_CTP_MODULE)
	if(HX8526_CTP_TRULY == hx8526_ctp_module_name)
	{
		ImageBuffer = i_CTPM_FW_Tuly;
	}
	else if(HX8526_CTP_YANGHUA == hx8526_ctp_module_name)
	{
		ImageBuffer = i_CTPM_FW_Yanghua;
	}
	else if(HX8526_CTP_SHENYUE == hx8526_ctp_module_name)
	{
		ImageBuffer = i_CTPM_FW_Shenyue;
	}
	else
	{
		ImageBuffer = i_CTPM_FW_Tuly;		
	}
#else
		ImageBuffer = i_CTPM_FW;
#endif


	I("Himax  TP version check start!\n");
	
	

	
	
	FLASH_VER_START_ADDR = (fullFileLength / 4) - 1;
	
	
	
	
	
	FW_VER_END_ADDR = FLASH_VER_START_ADDR * 4;

	himax_FlashMode(1);

	D("Himax TP version check for loop start\n");
	D("FLASH_VER_START_ADDR=%d\n", FLASH_VER_START_ADDR);
	D("FW_VER_END_ADDR=%d\n", FW_VER_END_ADDR);
	for (k = 0; k < 3; k++)
	{
		ret = 1;
		j = FW_VER_END_ADDR;

		cmd[0] = FLASH_VER_START_ADDR & 0x1F;
		cmd[1] = (FLASH_VER_START_ADDR >> 5) & 0x1F;
		cmd[2] = (FLASH_VER_START_ADDR >> 10) & 0x1F;

		if((i2c_smbus_write_i2c_block_data(i2c_client, 0x44, 3, &cmd[0]))< 0)	{ret = -1;	goto ret_proc;}
		if((i2c_smbus_write_i2c_block_data(i2c_client, 0x46, 0, &cmd[0]))< 0)	{ret = -1;	goto ret_proc;}
		if((i2c_smbus_read_i2c_block_data(i2c_client, 0x59, 4, &cmd[0]))< 0)	{ret = -1; 	goto ret_proc;}

		for (i = 0; i < 4; i++)
		{
			I("Himax TP version check, CTPW[%x]:%x, cmd[0]:%x\n", j, ImageBuffer[j], cmd[i]);
			if (ImageBuffer[j] != cmd[i])
			{
				ret = 0;
				break;
			}
			j++;
		}

		if (ret == 0)	fail_count++;
		if (ret == 1)	break;
	}

	ret_proc:
		himax_FlashMode(0);

		I("Himax TP version check loop count[%d], fail count[%d]\n", k, fail_count);
		I("Himax TP version check for loop end, return:%d\n", ret);

		return ret;
}

static bool i_Check_FW_Version()
{
	u16 cfg_min_start_addr;
	u16 cfg_min_end_addr;
	u16 cfg_min_addr;
	u16 cfg_min_length;

	uint8_t cmd[12];
	u16 i = 0;
	u16 j = 0;
	u16 k = 0;
	unsigned char* ImageBuffer;

#if defined(COMPARE_CTP_MODULE)
	if(HX8526_CTP_TRULY == hx8526_ctp_module_name)
	{
		ImageBuffer = i_CTPM_FW_Tuly;
	}
	else if(HX8526_CTP_YANGHUA == hx8526_ctp_module_name)
	{
		ImageBuffer = i_CTPM_FW_Yanghua;
	}
	else if(HX8526_CTP_SHENYUE == hx8526_ctp_module_name)
	{
		ImageBuffer = i_CTPM_FW_Shenyue;
	}
	else
	{
		ImageBuffer = i_CTPM_FW_Tuly;		
	}
#else
		ImageBuffer = i_CTPM_FW;
#endif

	cfg_min_start_addr   = CFG_VER_MIN_FLASH_ADDR / 4;	
	cfg_min_length       = CFG_VER_MIN_FLASH_LENG;		
	cfg_min_end_addr     = (CFG_VER_MIN_FLASH_ADDR + cfg_min_length ) / 4 + 1;		
	cfg_min_addr         = CFG_VER_MIN_FLASH_ADDR % 4;	

	
	if( himax_i2c_write_data(i2c_client, 0x81, 0, &(cmd[0])) < 0)
	{
		E("%s: i2c access fail!\n", __func__);
	}
	mdelay(120);

	
	himax_FlashMode(1);

	
	i = cfg_min_start_addr;
	do
	{
		cmd[0] = i & 0x1F;		
		cmd[1] = (i >> 5) & 0x1F;	
		cmd[2] = (i >> 10) & 0x1F;	

		if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0)
		{
			E("%s: i2c access fail!\n", __func__);
		}

		if( himax_i2c_write_data(i2c_client, 0x46, 0, &(cmd[0])) < 0)
		{
			E("%s: i2c access fail!\n", __func__);
		}

		if( himax_i2c_read_data(i2c_client, 0x59, 4, &(cmd[0])) < 0)
		{
			E("%s: i2c access fail!\n", __func__);
		}
		if(i == cfg_min_start_addr) 
		{
			j = 0;
			for(k = cfg_min_addr; k < 4 && j < cfg_min_length; k++)
			{
				CFG_VER_MIN_buff[j++] = cmd[k];
			}
		}
		else 
		{
			for(k = 0; k < 4 && j < cfg_min_length; k++)
			{
				CFG_VER_MIN_buff[j++] = cmd[k];
			}
		}
		i++;
	}while(i < cfg_min_end_addr);

	I("Himax %s FW_VER_MIN_buff = %d \n",__func__,FW_VER_MIN_buff[0]);
	I("Himax %s ImageBuffer[%d] = %d \n",__func__,FW_VER_MIN_FLASH_ADDR,ImageBuffer[FW_VER_MIN_FLASH_ADDR]);

	for (i = 0; i < CFG_VER_MIN_FLASH_LENG ; i++)
	{
		I("++++read FW_VER buff[%d]=0x%x,i_file=0x%x\n",i,CFG_VER_MIN_FLASH_buff[i],*(ImageBuffer + (CFG_VER_MIN_FLASH_ADDR ) + i));
		if (CFG_VER_MIN_buff[i] != *(ImageBuffer + (CFG_VER_MIN_FLASH_ADDR) + i))
			return true;
	}


  
	return false;
}

static int i_update_func(void)
{
	unsigned char* ImageBuffer;
	uint8_t cmd[5];
  int fullFileLength = sizeof(i_CTPM_FW);
	int checksumResult = 0;
#if defined(COMPARE_CTP_MODULE)
	if(HX8526_CTP_TRULY == hx8526_ctp_module_name)
	{
		ImageBuffer = i_CTPM_FW_Tuly;
	}
	else if(HX8526_CTP_YANGHUA == hx8526_ctp_module_name)
	{
		ImageBuffer = i_CTPM_FW_Yanghua;
	}
	else if(HX8526_CTP_SHENYUE == hx8526_ctp_module_name)
	{
		ImageBuffer = i_CTPM_FW_Shenyue;
	}
	else
	{
		ImageBuffer = i_CTPM_FW_Tuly;		
	}
#else
		ImageBuffer = i_CTPM_FW;
#endif

if (i_Needupdate && ((Check_FW_Version() > 0 )|| (Check_FW_Version() == 0 && himax_calculateChecksum(ImageBuffer, fullFileLength) == 0)))
	{
		mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
		irq_enable = false;

		if(fts_ctpm_fw_upgrade_with_i_file() == 0)
			E("TP upgrade error, line: %d\n", __LINE__);
		else
			I("TP upgrade OK, line: %d\n", __LINE__);
	}
#ifdef HX_FLASH_TEST
	else
	{
		disable_irq(i2c_client->irq);
		if( himax_i2c_write_data(i2c_client, 0x81 ,0, &cmd) < 0)
		{
			E("%s: i2c access fail! line:%d\n", __func__,__LINE__);
			return 0;
		}

		himax_changeIref(3);		
		checksumResult = himax_calculateChecksum(ImageBuffer, fullFileLength);

		I("%s, checksumResult1=%d\n", __func__, checksumResult);
		if(checksumResult == 1)
		{
			himax_changeIref(5);
			checksumResult = himax_calculateChecksum(ImageBuffer, fullFileLength);

			I("%s, checksumResult2=%d\n", __func__, checksumResult);
			if(checksumResult == 1)
			{
				himax_changeIref(2);
				checksumResult = himax_calculateChecksum(ImageBuffer, fullFileLength);
				I("%s, checksumResult3=%d\n", __func__, checksumResult);
			}
		}
		enable_irq(i2c_client->irq);

		if(checksumResult == 0)
		{
			I("TP upgrade start: %d\n", __LINE__);
			disable_irq(i2c_client->irq);
			if(fts_ctpm_fw_upgrade_with_i_file() == 0)
				E("TP upgrade error, line: %d\n", __LINE__);
			else
				I("TP upgrade OK, line: %d\n", __LINE__);

			#ifdef HX_RST_PIN_FUNC
				himax_HW_reset();
			#endif

			msleep(50);
			himax_ts_poweron();

			enable_irq(i2c_client->irq);
		}
	}
#endif
	return 0;

}

#endif


#ifdef TPD_PROXIMITY
	static int tpd_get_ps_value(void)
	{
		return tpd_proximity_detect;
	}
	static int tpd_enable_ps(int enable)
	{
		int ret = 0;
		char data[1] = {0};

		if(enable)	
		{
			data[0] =0x01;
			if((i2c_smbus_write_i2c_block_data(i2c_client, 0x92, 1, &data[0]))< 0)
				ret = 1;
			else
				tpd_proximity_flag = 1;
		}
		else	
		{
			data[0] =0x00;
			if((i2c_smbus_write_i2c_block_data(i2c_client, 0x92, 1, &data[0]))< 0)
				ret = 1;
			else
				tpd_proximity_flag = 0;
		}
		msleep(1);

		return ret;
	}
	int tpd_ps_operate(void* self, uint32_t command, void* buff_in, int size_in,
			void* buff_out, int size_out, int* actualout)
	{
		int err = 0;
		int value;
		hwm_sensor_data *sensor_data;
		switch (command)
		{
			case SENSOR_DELAY:
				if((buff_in == NULL) || (size_in < sizeof(int)))
				{
					TPD_PROXIMITY_DMESG("Set delay parameter error!\n");
					err = -EINVAL;
				}
				break;
			case SENSOR_ENABLE:
				if((buff_in == NULL) || (size_in < sizeof(int)))
				{
					TPD_PROXIMITY_DMESG("Enable sensor parameter error!\n");
					err = -EINVAL;
				}
				else
				{
					value = *(int *)buff_in;
					if(value)
					{
						if((tpd_enable_ps(1) != 0))
						{
							TPD_PROXIMITY_DMESG("enable ps fail: %d\n", err);
							return -1;
						}
					}
					else
					{
						if((tpd_enable_ps(0) != 0))
						{
							TPD_PROXIMITY_DMESG("disable ps fail: %d\n", err);
							return -1;
						}
					}
				}
				break;
			case SENSOR_GET_DATA:
				if((buff_out == NULL) || (size_out< sizeof(hwm_sensor_data)))
				{
					TPD_PROXIMITY_DMESG("get sensor data parameter error!\n");
					err = -EINVAL;
				}
				else
				{
					sensor_data = (hwm_sensor_data *)buff_out;
					sensor_data->values[0] = tpd_get_ps_value();
					sensor_data->value_divide = 1;
					sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
				}
				break;
			default:
				TPD_PROXIMITY_DMESG("proxmy sensor operate function no this parameter %d!\n", command);
				err = -1;
				break;
		}
		return err;
	}
	#endif




static int himax_ts_poweron(void)
{
	uint8_t buf0[11];
	int ret = 0;
	int config_fail_retry = 0;

	
	if (IC_TYPE == HX_85XX_C_SERIES_PWON)
	{
		buf0[0] = 0x42;
		buf0[1] = 0x02;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 1, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0x35;
		buf0[1] = 0x02;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 1, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0x36;
		buf0[1] = 0x0F;
		buf0[2] = 0x53;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0xDD;
		buf0[1] = 0x05;
		buf0[2] = 0x03;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0xB9;
		buf0[1] = 0x01;
		buf0[2] = 0x2D;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0xE3;
		buf0[1] = 0x00;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 1, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0x83;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 0, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		msleep(120); 

		#ifdef HX_LOADIN_CONFIG
		if(himax_config_flow() == -1)
		{
			E("Himax send config fail\n");
			
		}
		msleep(100); 
		#endif

		buf0[0] = 0x81;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 0, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		msleep(120); 
	}
	else if (IC_TYPE == HX_85XX_A_SERIES_PWON)
	{
		buf0[0] = 0x42;
		buf0[1] = 0x02;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 1, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}

		udelay(100);

		buf0[0] = 0x35;
		buf0[1] = 0x02;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 1, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0x36;
		buf0[1] = 0x0F;
		buf0[2] = 0x53;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0xDD;
		buf0[1] = 0x06;
		buf0[2] = 0x02;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0x76;
		buf0[1] = 0x01;
		buf0[2] = 0x2D;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		#ifdef HX_LOADIN_CONFIG
		if(himax_config_flow() == -1)
		{
			E("Himax send config fail\n");
		}
		msleep(100); 
		#endif

		buf0[0] = 0x83;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 0, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		msleep(120); 

		buf0[0] = 0x81;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 0, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		msleep(120); 
	}
	else if (IC_TYPE == HX_85XX_B_SERIES_PWON)
	{
		buf0[0] = 0x42;
		buf0[1] = 0x02;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 1, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0x35;
		buf0[1] = 0x02;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 1, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0x36;
		buf0[1] = 0x0F;

		buf0[2] = 0x53;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0xDD;
		buf0[1] = 0x06;
		buf0[2] = 0x02;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0x76;
		buf0[1] = 0x01;
		buf0[2] = 0x2D;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		#ifdef HX_LOADIN_CONFIG
		if(himax_config_flow() == -1)
		{
			E("Himax send config fail\n");
		}
		msleep(100); 
		#endif

		buf0[0] = 0x83;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 0, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		msleep(120); 

		buf0[0] = 0x81;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 0, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		msleep(120); 
	}
	else if (IC_TYPE == HX_85XX_D_SERIES_PWON)
	{
		buf0[0] = 0x42;	
		buf0[1] = 0x02;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 1, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0x36;	
		buf0[1] = 0x0F;
		buf0[2] = 0x53;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0xDD;	
		buf0[1] = 0x04;
		buf0[2] = 0x03;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0xB9;	
		buf0[1] = 0x01;
		buf0[2] = 0x36;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);

		buf0[0] = 0xCB;
		buf0[1] = 0x01;
		buf0[2] = 0xF5;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 2, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		udelay(100);


		#ifdef HX_LOADIN_CONFIG
			
			I("Himax start load config.\n");
			config_fail_retry = 0;
			while(true)
			{
				if(himax_config_flow() == -1)
				{
					config_fail_retry++;
					if(config_fail_retry == 3)
					{
						E("himax_config_flow retry fail.\n");
						goto send_i2c_msg_fail;
					}
					I("Himax config retry = %d \n",config_fail_retry);
				}
				else
				{
					break;
				}
			}
			I("Himax end load config.\n");
			msleep(100); 
		#endif

		buf0[0] = 0x83;
		ret = himax_i2c_write_data(i2c_client, buf0[0], 0, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		msleep(120); 

		buf0[0] = 0x81;	
		ret = himax_i2c_write_data(i2c_client, buf0[0], 0, &(buf0[1]));
		if(ret < 0)
		{
			E("i2c_master_send failed addr = 0x%x\n",i2c_client->addr);
			goto send_i2c_msg_fail;
		}
		msleep(120); 
	}
	else
	{
		E("[Himax] %s IC Type Error. \n",__func__);
	}
	return ret;

send_i2c_msg_fail:
	E("[Himax]:send_i2c_msg_fail @ line: %d \n",__LINE__);

	return -1;
}

static uint8_t himax_calculateChecksum(char *ImageBuffer, int fullLength)
{
	
	if(IC_CHECKSUM == HX_TP_BIN_CHECKSUM_SW)
	{
		u16 checksum = 0;
		uint8_t cmd[5], last_byte;
		int FileLength, i, readLen, k, lastLength;

		FileLength = fullLength - 2;
		memset(cmd, 0x00, sizeof(cmd));

		himax_FlashMode(1);

		FileLength = (FileLength + 3) / 4;
		for (i = 0; i < FileLength; i++)
		{
			last_byte = 0;
			readLen = 0;

			cmd[0] = i & 0x1F;
			if (cmd[0] == 0x1F || i == FileLength - 1)
			last_byte = 1;
			cmd[1] = (i >> 5) & 0x1F;cmd[2] = (i >> 10) & 0x1F;
			if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0 )
			{
				E("%s: i2c access fail!\n", __func__);
				return -1;
			}

			if( himax_i2c_write_data(i2c_client, 0x46, 0, &(cmd[0])) < 0 )
			{
				E("%s: i2c access fail!\n", __func__);
				return -1;
			}

			if( himax_i2c_read_data(i2c_client, 0x59, 4, &(cmd[0])) < 0 )
			{
				E("%s: i2c access fail!\n", __func__);
				return -1;
			}

			if (i < (FileLength - 1))
			{
				checksum += cmd[0] + cmd[1] + cmd[2] + cmd[3];
				if (i == 0)
				{
					E("%s: himax_marked cmd 0 to 3 (first 4 bytes): %d, %d, %d, %d\n", __func__, cmd[0], cmd[1], cmd[2], cmd[3]);
				}
			}
			else
			{
				E("%s: himax_marked cmd 0 to 3 (last 4 bytes): %d, %d, %d, %d\n", __func__, cmd[0], cmd[1], cmd[2], cmd[3]);
				E("%s: himax_marked, checksum (not last): %d\n", __func__, checksum);

				lastLength = (((fullLength - 2) % 4) > 0)?((fullLength - 2) % 4):4;

				for (k = 0; k < lastLength; k++)
				{
					checksum += cmd[k];
				}
				E("%s: himax_marked, checksum (final): %d\n", __func__, checksum);

				
				if (ImageBuffer[fullLength - 1] == (u8)(0xFF & (checksum >> 8)) && ImageBuffer[fullLength - 2] == (u8)(0xFF & checksum))
				{
					himax_FlashMode(0);
					return 1;
				}
				else 
				{
					himax_FlashMode(0);
					return 0;
				}
			}
		}
	}
	else if(IC_CHECKSUM == HX_TP_BIN_CHECKSUM_HW)
	{
		u32 sw_checksum = 0;
		u32 hw_checksum = 0;
		uint8_t cmd[5], last_byte;
		int FileLength, i, readLen, k, lastLength;

		FileLength = fullLength;
		memset(cmd, 0x00, sizeof(cmd));

		himax_FlashMode(1);

		FileLength = (FileLength + 3) / 4;
		for (i = 0; i < FileLength; i++)
		{
			last_byte = 0;
			readLen = 0;

			cmd[0] = i & 0x1F;
			if (cmd[0] == 0x1F || i == FileLength - 1)
			{
				last_byte = 1;
			}
			cmd[1] = (i >> 5) & 0x1F;cmd[2] = (i >> 10) & 0x1F;
			if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0 )
			{
				E("%s: i2c access fail!\n", __func__);
				return -1;
			}

			if( himax_i2c_write_data(i2c_client, 0x46, 0, &(cmd[0])) < 0 )
			{
				E("%s: i2c access fail!\n", __func__);
				return -1;
			}

			if( himax_i2c_read_data(i2c_client, 0x59, 4, &(cmd[0])) < 0 )
			{
				E("%s: i2c access fail!\n", __func__);
				return -1;
			}

			if (i < (FileLength - 1))
			{
				sw_checksum += cmd[0] + cmd[1] + cmd[2] + cmd[3];
				if (i == 0)
				{
					I("%s: himax_marked cmd 0 to 3 (first 4 bytes): %d, %d, %d, %d\n", __func__, cmd[0], cmd[1], cmd[2], cmd[3]);
				}
			}
			else
			{
				I("%s: himax_marked cmd 0 to 3 (last 4 bytes): %d, %d, %d, %d\n", __func__, cmd[0], cmd[1], cmd[2], cmd[3]);
				I("%s: himax_marked, sw_checksum (not last): %d\n", __func__, sw_checksum);

				lastLength = ((fullLength % 4) > 0)?(fullLength % 4):4;

				for (k = 0; k < lastLength; k++)
				{
					sw_checksum += cmd[k];
				}
				I("%s: himax_marked, sw_checksum (final): %d\n", __func__, sw_checksum);

				
				cmd[0] = 0x01;
				if( himax_i2c_write_data(i2c_client, 0xE5, 1, &(cmd[0])) < 0 )
				{
					E("%s: i2c access fail!\n", __func__);
					return -1;
				}

				
				msleep(30);

				
				if( himax_i2c_read_data(i2c_client, 0xAD, 4, &(cmd[0])) < 0 )
				{
					E("%s: i2c access fail!\n", __func__);
					return -1;
				}

				hw_checksum = cmd[0] + cmd[1]*0x100 + cmd[2]*0x10000 + cmd[3]*1000000;
				I("[Touch_FH] %s: himax_marked, sw_checksum (final): %d\n", __func__, sw_checksum);
				I("[Touch_FH] %s: himax_marked, hw_checkusm (final): %d\n", __func__, hw_checksum);

				
				if( hw_checksum == sw_checksum )
				{
					himax_FlashMode(0);
					return 1;
				}
				else
				{
					himax_FlashMode(0);
					return 0;
				}
			}
		}
	}
	else if(IC_CHECKSUM == HX_TP_BIN_CHECKSUM_CRC)
	{
		uint8_t cmd[5];

		
		if( himax_i2c_read_data(i2c_client, 0x7F, 5, &(cmd[0])) < 0 )
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}
		cmd[3] = 0x02;

		if( himax_i2c_write_data(i2c_client, 0x7F, 5, &(cmd[0])) < 0 )
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

#ifdef HX_FLASH_TEST  
		cmd[0] = 0x01;
		cmd[1] = 0x00;
		cmd[2] = 0x02;
		if (himax_i2c_write_data(i2c_client, 0x43 ,3, &cmd[0]) < 0)
		{
			E("%s: i2c access fail!\n", __func__);
			return 0;
		}
#else
		himax_FlashMode(1);
#endif

		
		cmd[0] = 0x05;
		cmd[1] = 0x00;
		cmd[2] = 0x00;
		if( himax_i2c_write_data(i2c_client, 0xD2, 3, &(cmd[0])) < 0 )
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		
		cmd[0] = 0x01;
		if( himax_i2c_write_data(i2c_client, 0xE5, 1, &(cmd[0])) < 0 )
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		
		msleep(30);

		
		if( himax_i2c_read_data(i2c_client, 0xAD, 4, &(cmd[0])) < 0 )
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		if( cmd[0] == 0 && cmd[1] == 0 && cmd[2] == 0 && cmd[3] == 0 )
		{
			himax_FlashMode(0);
			return 1;
		}
		else
		{
			himax_FlashMode(0);
			return 0;
		}
	}
	return 0;
}

static int himax_read_flash(unsigned char *buf, unsigned int addr_start, unsigned int length)
{
	u16 i = 0;
	u16 j = 0;
	u16 k = 0;
	uint8_t cmd[4];
	u16 local_start_addr = addr_start / 4;
	u16 local_length = length;
	u16 local_end_addr = (addr_start + length ) / 4 + 1;
	u16 local_addr = addr_start % 4;

	I("Himax %s addr_start = %d , local_start_addr = %d , local_length = %d , local_end_addr = %d , local_addr = %d \n",__func__,addr_start,local_start_addr,local_length,local_end_addr,local_addr);
	if( himax_i2c_write_data(i2c_client, 0x81, 0, &(cmd[0])) < 0 )
	{
		E("%s i2c write 81 fail.\n",__func__);
		return -1;
	}
	msleep(120);
	if( himax_i2c_write_data(i2c_client, 0x82, 0, &(cmd[0])) < 0 )
	{
		E("%s i2c write 82 fail.\n",__func__);
		return -1;
	}
	msleep(100);
	cmd[0] = 0x01;
	if( himax_i2c_write_data(i2c_client, 0x43, 1, &(cmd[0])) < 0 )
	{
		E("%s i2c write 43 fail.\n",__func__);
		return -1;
	}
	msleep(100);
	i = local_start_addr;
	do
	{
		cmd[0] = i & 0x1F;
		cmd[1] = (i >> 5) & 0x1F;
		cmd[2] = (i >> 10) & 0x1F;
		if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0 )
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_write_data(i2c_client, 0x46, 0, &(cmd[0])) < 0 )
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_read_data(i2c_client, 0x59, 4, &(cmd[0])) < 0 )
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		I("Himax cmd[0]=%d,cmd[1]=%d,cmd[2]=%d,cmd[3]=%d\n",cmd[0],cmd[1],cmd[2],cmd[3]);
		if(i == local_start_addr) 
		{
			j = 0;
			for(k = local_addr; k < 4 && j < local_length; k++)
			{
				buf[j++] = cmd[k];
			}
		}
		else 
		{
			for(k = 0; k < 4 && j < local_length; k++)
			{
				buf[j++] = cmd[k];
			}
		}
		i++;
	}
	while(i < local_end_addr);
	cmd[0] = 0;
	if( himax_i2c_write_data(i2c_client, 0x43, 1, &(cmd[0])) < 0 )
	{
		return -1;
	}
	return 1;
}

static int himax_read_flash_info(unsigned char *buf, unsigned int addr_start, unsigned int length)
{
	u16 i = 0;
	u16 j = 0;
	u16 k = 0;
	uint8_t cmd[4];
	u16 local_start_addr = addr_start / 4;
	u16 local_length = length;
	u16 local_end_addr = (addr_start + length ) / 4 + 1;
	u16 local_addr = addr_start % 4;

	I("Himax %s addr_start = %d , local_start_addr = %d , local_length = %d , local_end_addr = %d , local_addr = %d \n",__func__,addr_start,local_start_addr,local_length,local_end_addr,local_addr);
	i = local_start_addr;
	do
	{
		cmd[0] = i & 0x1F;
		cmd[1] = (i >> 5) & 0x1F;
		cmd[2] = (i >> 10) & 0x1F;
		if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0 )
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_write_data(i2c_client, 0x46, 0, &(cmd[0])) < 0 )
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_read_data(i2c_client, 0x59, 4, &(cmd[0])) < 0 )
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		I("Himax cmd[0]=%d,cmd[1]=%d,cmd[2]=%d,cmd[3]=%d\n",cmd[0],cmd[1],cmd[2],cmd[3]);
		if(i == local_start_addr) 
		{
			j = 0;
			for(k = local_addr; k < 4 && j < local_length; k++)
			{
				buf[j++] = cmd[k];
			}
		}
		else 
		{
			for(k = 0; k < 4 && j < local_length; k++)
			{
				buf[j++] = cmd[k];
			}
		}
		i++;
	}
	while(i < local_end_addr);
	return 1;
}


void calculate_point_number(void)
{
	I("Himax HX_MAX_PT = %d\n",HX_MAX_PT);
	HX_TOUCH_INFO_POINT_CNT = HX_MAX_PT * 4 ;

	if( (HX_MAX_PT % 4) == 0)
	{
		HX_TOUCH_INFO_POINT_CNT += (HX_MAX_PT / 4) * 4 ;
	}
	else
	{
		HX_TOUCH_INFO_POINT_CNT += ((HX_MAX_PT / 4) +1) * 4 ;
	}
	I("Himax HX_TOUCH_INFO_POINT_CNT = %d\n",HX_TOUCH_INFO_POINT_CNT);
}

void himax_touch_information(void)
{
	static unsigned char temp_buffer[6];
	uint8_t cmd[4];

	TPD_DMESG("start %s, %d\n", __FUNCTION__, __LINE__);

	if(IC_TYPE == HX_85XX_A_SERIES_PWON)
	{
		HX_RX_NUM = 0;
		HX_TX_NUM = 0;
		HX_BT_NUM = 0;
		HX_X_RES = 0;
		HX_Y_RES = 0;
		HX_MAX_PT = 0;
		HX_INT_IS_EDGE = false;
	}
	else if(IC_TYPE == HX_85XX_B_SERIES_PWON)
	{
		HX_RX_NUM = 0;
		HX_TX_NUM = 0;
		HX_BT_NUM = 0;
		HX_X_RES = 0;
		HX_Y_RES = 0;
		HX_MAX_PT = 0;
		HX_INT_IS_EDGE = false;
	}
	else if(IC_TYPE == HX_85XX_C_SERIES_PWON)
	{
		
		himax_read_flash( temp_buffer, 0x3D5, 3);
		HX_RX_NUM = temp_buffer[0];
		HX_TX_NUM = temp_buffer[1];
		HX_BT_NUM = (temp_buffer[2]) & 0x1F;

		
		himax_read_flash( temp_buffer, 0x345, 4);
		HX_X_RES = temp_buffer[0]*256 + temp_buffer[1];
		HX_Y_RES = temp_buffer[2]*256 + temp_buffer[3];

		
		himax_read_flash( temp_buffer, 0x3ED, 1);
		HX_MAX_PT = temp_buffer[0] >> 4;

		
		himax_read_flash( temp_buffer, 0x3EE, 2);
		if( (temp_buffer[1] && 0x01) == 1 )
		{
			HX_INT_IS_EDGE = true;
		}
		else
		{
			HX_INT_IS_EDGE = false;
		}
	}
	else if(IC_TYPE == HX_85XX_D_SERIES_PWON)
	{

		if( himax_i2c_write_data(i2c_client, 0x81, 0, &(cmd[0])) < 0 )
		{
			E("%s i2c write 81 fail.\n",__func__);
			return -1;
		}
		msleep(120);
		if( himax_i2c_write_data(i2c_client, 0x82, 0, &(cmd[0])) < 0 )
		{
			E("%s i2c write 82 fail.\n",__func__);
			return -1;
		}
		msleep(100);
		cmd[0] = 0x01;
		if( himax_i2c_write_data(i2c_client, 0x43, 1, &(cmd[0])) < 0 )
		{
			E("%s i2c write 43 fail.\n",__func__);
			return -1;
		}
		msleep(100);

		
		

		himax_read_flash_info( temp_buffer, 0x26E, 5);
		HX_RX_NUM = temp_buffer[0];
		HX_TX_NUM = temp_buffer[1];
		HX_MAX_PT = (temp_buffer[2] & 0xF0) >> 4;
		
		HX_XY_REV = (temp_buffer[4] & 0x04);

		#ifdef HX_EN_SEL_BUTTON
		HX_BT_NUM = (temp_buffer[2] & 0x0F);
		#endif


		#ifdef HX_EN_MUT_BUTTON
		himax_read_flash_info( temp_buffer, 0x262, 1);
		HX_BT_NUM = (temp_buffer[0] & 0x07);
		#endif

		himax_read_flash_info( temp_buffer, 0x272, 6);
		if(HX_XY_REV==0)
		{
		HX_X_RES = temp_buffer[2]*256 + temp_buffer[3];
		HX_Y_RES = temp_buffer[4]*256 + temp_buffer[5];
        }
		else
		{
		HX_Y_RES = temp_buffer[2]*256 + temp_buffer[3];
		HX_X_RES = temp_buffer[4]*256 + temp_buffer[5];
		}

		himax_read_flash_info( temp_buffer, 0x200, 6);
		if( (temp_buffer[1] && 0x01) == 1 )
		{
			HX_INT_IS_EDGE = true;
		}
		else
		{
			HX_INT_IS_EDGE = false;
		}

		I("Himax HX_RX_NUM = %d \n",HX_RX_NUM);
		I("Himax HX_TX_NUM = %d \n",HX_TX_NUM);
		I("Himax HX_MAX_PT = %d \n",HX_MAX_PT);
		I("Himax HX_BT_NUM = %d \n",HX_BT_NUM);
		I("Himax HX_X_RES = %d \n",HX_X_RES);
		I("Himax HX_Y_RES = %d \n",HX_Y_RES);
		if(HX_INT_IS_EDGE)
		{
			I("Himax HX_INT_IS_EDGE = true \n");
		}
		else
		{
			I("Himax HX_INT_IS_EDGE = false \n");
		}
		
		himax_read_flash_info(temp_buffer,0x85,2); 
		if( (temp_buffer[0] & 0x80) == 0x80 )
		{
			Is_2T3R = true;

			himax_read_flash_info(temp_buffer,0x216,4); 

			HX_RX_NUM_2 = temp_buffer[0];
			HX_TX_NUM_2 = temp_buffer[1];
			I("Himax 2T3R HX_RX_NUM_2 = %d \n",HX_RX_NUM_2);
			I("Himax 2T3R HX_TX_NUM_2 = %d \n",HX_TX_NUM_2);
		}
		else
		{
			Is_2T3R = false;
			I("Himax Is_2T3T is false\n");
		}

		cmd[0] = 0;
		if( himax_i2c_write_data(i2c_client, 0x43, 1, &(cmd[0])) < 0 )
		{
			return -1;
		}


	}
	else
	{
		HX_RX_NUM = 0;
		HX_TX_NUM = 0;
		HX_BT_NUM = 0;
		HX_X_RES = 0;
		HX_Y_RES = 0;
		HX_MAX_PT = 0;
		HX_INT_IS_EDGE = false;
	}

	TPD_DMESG("end %s, %d\n", __FUNCTION__, __LINE__);
}

int himax_hang_shaking(void)    
{

	int ret, result;
	uint8_t hw_reset_check[1];
	uint8_t hw_reset_check_2[1];
	uint8_t buf0[2];

    if(mt_get_gpio_in(GPIO_CTP_EINT_PIN) == 0)
    {
       I("[Himax]%s: IRQ = 0, return form himax_hang_shaking \n", __func__);
       return 0;
    }

	
	buf0[0] = 0x92;
	if(IC_STATUS_CHECK == 0xAA)
	{
		buf0[1] = 0xAA;
		IC_STATUS_CHECK = 0x55;
	}
	else
	{
		buf0[1] = 0x55;
		IC_STATUS_CHECK = 0xAA;
	}

	if( himax_i2c_write_data(i2c_client, buf0[0], 1, &(buf0[1])) < 0 )
	{
		E("[Himax]:write 0x92 failed line: %d \n",__LINE__);
		goto work_func_send_i2c_msg_fail;
	}
	msleep(15); 

	buf0[0] = 0x92;

#ifdef Himax_fcover
	if(sensitivity_state == 1) {
	   buf0[1] = 0x04;
	}
        else
#endif
	{
	   buf0[1] = 0x00;
	}

	if( himax_i2c_write_data(i2c_client, buf0[0], 1, &(buf0[1])) < 0 )
	{
		E("[Himax]:write 0x92 failed line: %d \n",__LINE__);
		goto work_func_send_i2c_msg_fail;
	}
	msleep(2);

	if( himax_i2c_read_data(i2c_client, 0xDA, 1, &(hw_reset_check[0]))<0 )
	{
		E("[Himax]:himax_i2c_read_data 0xDA failed line: %d \n",__LINE__);
		goto work_func_send_i2c_msg_fail;
	}
	

	if((IC_STATUS_CHECK != hw_reset_check[0]))
	{
		msleep(2);
		if( himax_i2c_read_data(i2c_client, 0xDA, 1, &(hw_reset_check_2[0]))<0 )
		{
			E("[Himax]:i2c_himax_read 0xDA failed line: %d \n",__LINE__);
			goto work_func_send_i2c_msg_fail;
		}
		

		if(hw_reset_check[0] == hw_reset_check_2[0])
		{
			result = 1; 
		}
		else
		{
			result = 0; 
		}
	}
	else
	{
		result = 0; 
	}

	return result;

work_func_send_i2c_msg_fail:
	return 2;
}


#ifdef HX_ESD_WORKAROUND
void ESD_HW_REST(void)
{
	ESD_RESET_ACTIVATE = 1;
	

	I("Himax TP: ESD - Reset\n");

	#ifdef HX_RST_BY_POWER
		
		hwPowerDown(MT65XX_POWER_LDO_VGP4,  "TP");
		hwPowerDown(MT65XX_POWER_LDO_VGP5, "TP_EINT");
		msleep(100);

		
		hwPowerOn(MT65XX_POWER_LDO_VGP4, VOL_2800, "TP");
		hwPowerOn(MT65XX_POWER_LDO_VGP5, VOL_1800, "TP_EINT");
		msleep(100);
		I("Himax %s TP rst IN ESD  ESD ESD by HX_RST_BY_POWER %d\n",__func__,__LINE__);

	#else
		mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
		msleep(20);
		mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
		msleep(20);
		I("Himax %s TP rst  in ESD ESD ESD  by RST PIN %d\n",__func__,__LINE__);
	#endif

	himax_ts_poweron();

	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	irq_enable = true;
}
#endif

#ifdef ENABLE_CHIP_STATUS_MONITOR
static int himax_chip_monitor_function(struct work_struct *dat) 
{
	int ret;

	if((running_status == 0)&& (suspend_state == 0))
	{
		if(mt_get_gpio_in(GPIO_CTP_EINT_PIN) == 0)
		{
			I("[Himax]%s: IRQ = 0, Enable IRQ\n", __func__);
			mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
			irq_enable = true;
		}

		ret = himax_hang_shaking(); 
		if(ret == 2)
		{
		
		        ESD_HW_REST();
			I("[Himax] %s: I2C Fail \n", __func__);
		}
		if(ret == 1)
		{
			I("[Himax] %s: MCU Stop \n", __func__);

			#ifdef HX_ESD_WORKAROUND
			ESD_HW_REST();
			#endif
		}
		
		

		queue_delayed_work(himax_wq, &himax_chip_monitor, 3*HZ);	
	}
	return 0;
}
#endif

#ifdef HX_RST_PIN_FUNC
void himax_HW_reset(void)
{
	#ifdef HX_ESD_WORKAROUND
	ESD_RESET_ACTIVATE = 1;
	#endif

	#ifdef HX_RST_BY_POWER
		
		hwPowerDown(MT65XX_POWER_LDO_VGP4,  "TP");
		hwPowerDown(MT65XX_POWER_LDO_VGP5, "TP_EINT");
		msleep(100);

		
		hwPowerOn(MT65XX_POWER_LDO_VGP4, VOL_2800, "TP");
		hwPowerOn(MT65XX_POWER_LDO_VGP5, VOL_1800, "TP_EINT");
		msleep(100);
		I("Himax %s TP rst by HX_RST_BY_POWER %d\n",__func__,__LINE__);
	#else
		mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
		msleep(20);
		mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
		msleep(20);
		I("Himax %s TP rst by RST PIN %d\n",__func__,__LINE__);
	#endif
}
#endif


#ifdef HX_TP_SYS_REGISTER
static ssize_t himax_register_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	int base = 0;
	uint16_t loop_i,loop_j;
	uint8_t data[128];
	uint8_t outData[5];

	memset(outData, 0x00, sizeof(outData));
	memset(data, 0x00, sizeof(data));

	I("Himax multi_register_command = %d \n",multi_register_command);

	if(multi_register_command == 1)
	{
		base = 0;

		for(loop_i = 0; loop_i < 6; loop_i++)
		{
			if(multi_register[loop_i] != 0x00)
			{
				if(multi_cfg_bank[loop_i] == 1) 
				{
					outData[0] = 0x15;
					himax_i2c_write_data(i2c_client, 0xE1, 1, &(outData[0]));
					msleep(10);

					outData[0] = 0x00;
					outData[1] = multi_register[loop_i];
					himax_i2c_write_data(i2c_client, 0xD8, 2, &(outData[0]));
					msleep(10);

					himax_i2c_read_data(i2c_client, 0x5A, 128, &(data[0]));

					outData[0] = 0x00;
					himax_i2c_write_data(i2c_client, 0xE1, 1, &(outData[0]));

					for(loop_j=0; loop_j<128; loop_j++)
					{
						multi_value[base++] = data[loop_j];
					}
				}
				else 
				{
					himax_i2c_read_data(i2c_client, multi_register[loop_i], 128, &(data[0]));

					for(loop_j=0; loop_j<128; loop_j++)
					{
						multi_value[base++] = data[loop_j];
					}
				}
			}
		}

		base = 0;
		for(loop_i = 0; loop_i < 6; loop_i++)
		{
			if(multi_register[loop_i] != 0x00)
			{
				if(multi_cfg_bank[loop_i] == 1)
				{
					ret += sprintf(buf + ret, "Register: FE(%x)\n", multi_register[loop_i]);
				}
				else
				{
					ret += sprintf(buf + ret, "Register: %x\n", multi_register[loop_i]);
				}

				for (loop_j = 0; loop_j < 128; loop_j++)
				{
					ret += sprintf(buf + ret, "0x%2.2X ", multi_value[base++]);
					if ((loop_j % 16) == 15)
					{
						ret += sprintf(buf + ret, "\n");
					}
				}
			}
		}
		return ret;
	}

	if(config_bank_reg)
	{
		I("[TP] %s: register_command = FE(%x)\n", __func__, register_command);

		
		outData[0] = 0x15;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(outData[0]));

		msleep(10);

		outData[0] = 0x00;
		outData[1] = register_command;
		himax_i2c_write_data(i2c_client, 0xD8, 2, &(outData[0]));

		msleep(10);

		himax_i2c_read_data(i2c_client, 0x5A, 128, &(data[0]));

		msleep(10);

		outData[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(outData[0]));
	}
	else
	{
		if ( himax_i2c_read_data(i2c_client, register_command, 128, &(data[0])) < 0 )
		{
			return ret;
		}
	}

	if(config_bank_reg)
	{
		ret += sprintf(buf, "command: FE(%x)\n", register_command);
	}
	else
	{
		ret += sprintf(buf, "command: %x\n", register_command);
	}

	for (loop_i = 0; loop_i < 128; loop_i++)
	{
		ret += sprintf(buf + ret, "0x%2.2X ", data[loop_i]);
		if ((loop_i % 16) == 15)
		{
			ret += sprintf(buf + ret, "\n");
		}
	}

	ret += sprintf(buf + ret, "\n");
	return ret;
}

static ssize_t himax_register_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	char buf_tmp[6], length = 0;
	unsigned long result = 0;
	uint8_t loop_i = 0;
	uint16_t base = 5;
	uint8_t write_da[128];
	uint8_t outData[5];

	memset(buf_tmp, 0x0, sizeof(buf_tmp));
	memset(write_da, 0x0, sizeof(write_da));
	memset(outData, 0x0, sizeof(outData));

	I("himax %s \n",buf);

	if( buf[0] == 'm' && buf[1] == 'r' && buf[2] == ':')
	{
		memset(multi_register, 0x00, sizeof(multi_register));
		memset(multi_cfg_bank, 0x00, sizeof(multi_cfg_bank));
		memset(multi_value, 0x00, sizeof(multi_value));

		I("himax multi register enter\n");

		multi_register_command = 1;

		base 		= 2;
		loop_i 	= 0;

		while(true)
		{
			if(buf[base] == '\n')
			{
				break;
			}

			if(loop_i >= 6 )
			{
				break;
			}

			if(buf[base] == ':' && buf[base+1] == 'x' && buf[base+2] == 'F' && buf[base+3] == 'E' && buf[base+4] != ':')
			{
				memcpy(buf_tmp, buf + base + 4, 2);
				if (!strict_strtoul(buf_tmp, 16, &result))
				{
					multi_register[loop_i] = result;
					multi_cfg_bank[loop_i++] = 1;
				}
				base += 6;
			}
			else
			{
				memcpy(buf_tmp, buf + base + 2, 2);
				if (!strict_strtoul(buf_tmp, 16, &result))
				{
					multi_register[loop_i] = result;
					multi_cfg_bank[loop_i++] = 0;
				}
				base += 4;
			}
		}

		I("========================== \n");
		for(loop_i = 0; loop_i < 6; loop_i++)
		{
			I("%d,%d:",multi_register[loop_i],multi_cfg_bank[loop_i]);
		}
		printk("\n");
	}
	else if ((buf[0] == 'r' || buf[0] == 'w') && buf[1] == ':')
	{
		multi_register_command = 0;

		if (buf[2] == 'x')
		{
			if(buf[3] == 'F' && buf[4] == 'E') 
			{
				config_bank_reg = true;

				memcpy(buf_tmp, buf + 5, 2);
				if (!strict_strtoul(buf_tmp, 16, &result))
				{
					register_command = result;
				}
				base = 7;

				I("CMD: FE(%x)\n", register_command);
			}
			else
			{
				config_bank_reg = false;

				memcpy(buf_tmp, buf + 3, 2);
				if (!strict_strtoul(buf_tmp, 16, &result))
				{
					register_command = result;
				}
				base = 5;
				I("CMD: %x\n", register_command);
			}

			for (loop_i = 0; loop_i < 128; loop_i++)
			{
				if (buf[base] == '\n')
				{
					if (buf[0] == 'w')
					{
						if(config_bank_reg)
						{
							outData[0] = 0x15;
							himax_i2c_write_data(i2c_client, 0xE1, 1, &(outData[0]));

							msleep(10);

							outData[0] = 0x00;
							outData[1] = register_command;
							himax_i2c_write_data(i2c_client, 0xD8, 2, &(outData[0]));

							msleep(10);
							himax_i2c_write_data(i2c_client, 0x40, length, &(write_da[0]));

							msleep(10);

							outData[0] = 0x00;
							himax_i2c_write_data(i2c_client, 0xE1, 1, &(outData[0]));

							I("CMD: FE(%x), %x, %d\n", register_command,write_da[0], length);
						}
						else
						{
							himax_i2c_write_data(i2c_client, register_command, length, &(write_da[0]));
							I("CMD: %x, %x, %d\n", register_command,write_da[0], length);
						}
					}

					printk("\n");
					return count;
				}
				if (buf[base + 1] == 'x')
				{
					buf_tmp[4] = '\n';
					buf_tmp[5] = '\0';
					memcpy(buf_tmp, buf + base + 2, 2);
					if (!strict_strtoul(buf_tmp, 16, &result))
					{
						write_da[loop_i] = result;
					}
					length++;
				}
				base += 4;
			}
		}
	}
	return count;
}

static DEVICE_ATTR(register, 0644, himax_register_show, himax_register_store);
#endif

#ifdef HX_TP_SYS_DEBUG_LEVEL
int fts_ctpm_fw_upgrade_with_sys_fs(unsigned char *fw, int len)
{
	unsigned char* ImageBuffer = fw; 
	int fullFileLength = len; 
	int i, j;
	uint8_t cmd[5], last_byte, prePage;
	int FileLength;
	uint8_t checksumResult = 0;

	
	for (j = 0; j < 3; j++)
	{
		if(IC_CHECKSUM == HX_TP_BIN_CHECKSUM_CRC)
		{
			FileLength = fullFileLength;
		}
		else
		{
			FileLength = fullFileLength - 2;
		}

		#ifdef HX_RST_PIN_FUNC
		himax_HW_reset();
		#endif

		if( himax_i2c_write_data(i2c_client, 0x81, 0, &(cmd[0])) < 0 )
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		mdelay(120);

		himax_unlock_flash();  

		cmd[0] = 0x05;cmd[1] = 0x00;cmd[2] = 0x02;
		if( himax_i2c_write_data(i2c_client, 0x43, 3, &(cmd[0])) < 0 )
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}

		if( himax_i2c_write_data(i2c_client, 0x4F, 0, &(cmd[0])) < 0 )
		{
			E("%s: i2c access fail!\n", __func__);
			return -1;
		}
		mdelay(50);

		himax_ManualMode(1);
		himax_FlashMode(1);

		FileLength = (FileLength + 3) / 4;
		for (i = 0, prePage = 0; i < FileLength; i++)
		{
			last_byte = 0;
			cmd[0] = i & 0x1F;
			if (cmd[0] == 0x1F || i == FileLength - 1)
			{
				last_byte = 1;
			}
			cmd[1] = (i >> 5) & 0x1F;
			cmd[2] = (i >> 10) & 0x1F;
			if( himax_i2c_write_data(i2c_client, 0x44, 3, &(cmd[0])) < 0 )
			{
				E("%s: i2c access fail!\n", __func__);
				return -1;
			}

			if (prePage != cmd[1] || i == 0)
			{
				prePage = cmd[1];
				cmd[0] = 0x01;cmd[1] = 0x09;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					E("%s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;cmd[1] = 0x0D;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					E("%s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;cmd[1] = 0x09;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					E("%s: i2c access fail!\n", __func__);
					return -1;
				}
			}

			memcpy(&cmd[0], &ImageBuffer[4*i], 4);
			if( himax_i2c_write_data(i2c_client, 0x45, 4, &(cmd[0])) < 0 )
			{
				E("%s: i2c access fail!\n", __func__);
				return -1;
			}

			cmd[0] = 0x01;cmd[1] = 0x0D;
			if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
			{
				E("%s: i2c access fail!\n", __func__);
				return -1;
			}

			cmd[0] = 0x01;cmd[1] = 0x09;
			if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
			{
				E("%s: i2c access fail!\n", __func__);
				return -1;
			}

			if (last_byte == 1)
			{
				cmd[0] = 0x01;cmd[1] = 0x01;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					E("%s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;cmd[1] = 0x05;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					E("%s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;cmd[1] = 0x01;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					E("%s: i2c access fail!\n", __func__);
					return -1;
				}

				cmd[0] = 0x01;cmd[1] = 0x00;
				if( himax_i2c_write_data(i2c_client, 0x43, 2, &(cmd[0])) < 0 )
				{
					E("%s: i2c access fail!\n", __func__);
					return -1;
				}

				mdelay(10);
				if (i == (FileLength - 1))
				{
					himax_FlashMode(0);
					himax_ManualMode(0);
#ifdef HX_FLASH_TEST 
					
					himax_changeIref(3);
					checksumResult = himax_calculateChecksum(ImageBuffer, fullFileLength);

					if(checksumResult == 1)
					{
						
						himax_changeIref(5);
						checksumResult = himax_calculateChecksum(ImageBuffer, fullFileLength);

						if(checksumResult == 1)
						{
							
							himax_changeIref(2);
							checksumResult = himax_calculateChecksum(ImageBuffer, fullFileLength);
						}
					}
#else
					checksumResult = himax_calculateChecksum(ImageBuffer, fullFileLength);
#endif
					himax_lock_flash();

					if (checksumResult > 0) 
					{
						return 1;
					}
					else 
					{
#ifdef HX_FLASH_TEST 
						flash_checksum_pass = false;
				
#endif
						E("%s: checksum calculate fail , retry j = %d !\n", __func__, j);
						return 0;
					}
				}
			}
		}
	}
	
}

static ssize_t himax_debug_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	size_t count = 0;
	int i = 0;

	if(debug_level_cmd == 't')
	{
		if(fw_update_complete)
		{
			count += sprintf(buf, "FW Update Complete \n");
		}
		else
		{
			count += sprintf(buf, "FW Update Fail \n");
		}
	}
	else if(debug_level_cmd == 'i')
	{
		if(irq_enable)
		{
			count += sprintf(buf, "IRQ is enable\n");
		}
		else
		{
			count += sprintf(buf, "IRQ is disable\n");
		}
	}
	else if(debug_level_cmd == 'r')
	{
		if(power_ret<0)
		{
			count += sprintf(buf, " himax power on fail \n");
		}
		else
		{
			count += sprintf(buf, "himax power on OK \n");
		}
	}
	else if(debug_level_cmd == 'h')
	{
		if(handshaking_result == 0)
		{
			count += sprintf(buf, "Handshaking Result = %d (MCU Running)\n",handshaking_result);
		}
		else if(handshaking_result == 1)
		{
			count += sprintf(buf, "Handshaking Result = %d (MCU Stop)\n",handshaking_result);
		}
		else if(handshaking_result == 2)
		{
			count += sprintf(buf, "Handshaking Result = %d (I2C Error)\n",handshaking_result);
		}
		else
		{
			count += sprintf(buf, "Handshaking Result = error \n");
		}
	}
	else if(debug_level_cmd == 'v')
	{
		count += sprintf(buf + count, "FW_VER_MAJ_buff = ");
		count += sprintf(buf + count, "0x%2.2X \n",FW_VER_MAJ_buff[0]);

		count += sprintf(buf + count, "FW_VER_MIN_buff = ");
		count += sprintf(buf + count, "0x%2.2X \n",FW_VER_MIN_buff[0]);

		count += sprintf(buf + count, "CFG_VER_MAJ_buff = ");
		for( i=0 ; i<12 ; i++)
		{
			count += sprintf(buf + count, "0x%2.2X ",CFG_VER_MAJ_buff[i]);
		}
		count += sprintf(buf + count, "\n");

		count += sprintf(buf + count, "CFG_VER_MIN_buff = ");
		for( i=0 ; i<12 ; i++)
		{
			count += sprintf(buf + count, "0x%2.2X ",CFG_VER_MIN_buff[i]);
		}
		count += sprintf(buf + count, "\n");

		count += sprintf(buf + count, "i_file_version = ");
#ifdef  HX_FW_UPDATE_BY_I_FILE
		for( i=0 ; i<12 ; i++)
		{
			count += sprintf(buf + count, "0x%2.2X ",i_CTPM_FW[CFG_VER_MIN_FLASH_ADDR + i]);
		}
		count += sprintf(buf + count, "\n");
#endif
	}
	else if(debug_level_cmd == 'd')
	{
		count += sprintf(buf + count, "Himax Touch IC Information :\n");
		if(IC_TYPE == HX_85XX_A_SERIES_PWON)
		{
			count += sprintf(buf + count, "IC Type : A\n");
		}
		else if(IC_TYPE == HX_85XX_B_SERIES_PWON)
		{
			count += sprintf(buf + count, "IC Type : B\n");
		}
		else if(IC_TYPE == HX_85XX_C_SERIES_PWON)
		{
			count += sprintf(buf + count, "IC Type : C\n");
		}
		else if(IC_TYPE == HX_85XX_D_SERIES_PWON)
		{
			count += sprintf(buf + count, "IC Type : D\n");
		}
		else
		{
			count += sprintf(buf + count, "IC Type error.\n");
		}

		if(IC_CHECKSUM == HX_TP_BIN_CHECKSUM_SW)
		{
			count += sprintf(buf + count, "IC Checksum : SW\n");
		}
		else if(IC_CHECKSUM == HX_TP_BIN_CHECKSUM_HW)
		{
			count += sprintf(buf + count, "IC Checksum : HW\n");
		}
		else if(IC_CHECKSUM == HX_TP_BIN_CHECKSUM_CRC)
		{
			count += sprintf(buf + count, "IC Checksum : CRC\n");
		}
		else
		{
			count += sprintf(buf + count, "IC Checksum error.\n");
		}

		if(HX_INT_IS_EDGE)
		{
			count += sprintf(buf + count, "Interrupt : EDGE TIRGGER\n");
		}
		else
		{
			count += sprintf(buf + count, "Interrupt : LEVEL TRIGGER\n");
		}

		count += sprintf(buf + count, "RX Num : %d\n",HX_RX_NUM);
		count += sprintf(buf + count, "TX Num : %d\n",HX_TX_NUM);
		count += sprintf(buf + count, "BT Num : %d\n",HX_BT_NUM);
		count += sprintf(buf + count, "X Resolution : %d\n",HX_X_RES);
		count += sprintf(buf + count, "Y Resolution : %d\n",HX_Y_RES);
		count += sprintf(buf + count, "Max Point : %d\n",HX_MAX_PT);
	}
	else
	{
		count += sprintf(buf, "%d\n", debug_log_level);
	}
	return count;
}

static ssize_t himax_debug_dump(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	char data[3];
	char fileName[128];
	struct file* filp = NULL;
	mm_segment_t oldfs;
	int result = 0;
       int k;

	if (buf[0] >= '0' && buf[0] <= '9' && buf[1] == '\n')
	{
		debug_log_level = buf[0] - '0';
	}

	if (buf[0] == 'i') 
	{
		debug_level_cmd = buf[0];

		if( buf[2] == '1') 
		{
			mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
			irq_enable = true;
		}
		else if(buf[2] == '0') 
		{
			mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
			irq_enable = false;
		}
		else
		{
			E("[Himax] %s: debug_level command = 'i' , parameter error.\n", __func__);
		}
		return count;
	}

	if( buf[0] == 'h') 
	{
		debug_level_cmd = buf[0];

		mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
		irq_enable = false;

		handshaking_result = himax_hang_shaking(); 

		mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
		irq_enable = true;

		return count;
	}

	if( buf[0] == 'v') 
	{
		debug_level_cmd = buf[0];
		himax_read_FW_ver();
		mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
		irq_enable = true;
		return count;
	}

	if( buf[0] == 'd') 
	{
		debug_level_cmd = buf[0];
		return count;
	}

	if( buf[0] == 'r') 
	{
		debug_level_cmd = buf[0];
#ifdef HX_RST_PIN_FUNC
	himax_HW_reset();
#endif
		msleep(50);
		power_ret=himax_ts_poweron();
		return count;
	}
	if( buf[0] == 'F')
	{
		if(buf[1] == 't')
		{
			sscanf(&(buf[3]), "%s",&touchdown_flag);
		}
		if(buf[1] == 'm')
		{
			sscanf(&(buf[3]), "%s",&rec_msg_flag);
		}
		if(buf[1] == 'd')
		{
			sscanf(&(buf[3]), "%s",&rec_data_flag);
		}

	}
#ifdef ENABLE_CHIP_STATUS_MONITOR
	if( buf[0] == 'M')
	{
		if(buf[1] == 'o')
		{
			running_status = 0;
			queue_delayed_work(himax_wq, &himax_chip_monitor, 10*HZ);

		}
		if(buf[1] == 'c')
		{
			running_status = 1;
			cancel_delayed_work_sync(&himax_chip_monitor);
		}
	}
#endif
#if defined(GESTURE_PT_LOG) || defined(Himax_Gesture)
	
	if (buf[0] == 'g')
	{
		if(buf[1]== 'c')
		{
			if(sscanf(&(buf[3]), "%d",&ges_debug_flag) ==0)
			{
				I("+++ ges_debug_flag =0x%x\n",ges_debug_flag);
			}
		}
		if(buf[1]=='k')
		{
			if(sscanf(&(buf[3]), "%d",&FC_debug_flag) ==0)
			{
				I("+++ FC_debug_flag =0x%x\n",FC_debug_flag);
			}
		}
		if(buf[1]=='r')
		{
			if(sscanf(&(buf[3]), "%d",&charge_debug_flag) ==0)
			{
				I("+++ charge_debug_flag =0x%x\n",charge_debug_flag);
			}
		}
	}

#endif
#ifdef BUTTON_CHECK
    if(buf[0] == 'b') 
    {
        if(sscanf(&(buf[2]), "%d", &bt_confirm_cnt) == 0)
        {
            I("++++bt_confirm_cnt=0x%x\n", bt_confirm_cnt);
        }
    }


    if(buf[0] == 'o') 
    {
        if(sscanf(&(buf[2]), "%d", &obs_intvl) == 0)
        {
            I("++++obs_intvl=0x%x\n", obs_intvl);
        }
    }

    if(buf[0] == 'c') 
    {
        if(sscanf(&(buf[2]), "%d", &himax_debug_flag) == 0)
        {
            I("++++himax_debug_flag=0x%x\n", himax_debug_flag);
        }
    }


#endif

#ifdef PT_NUM_LOG
        if(buf[0] == 'z') 
        {
            if(sscanf(&(buf[2]), "%d", &himax_debug_flag) == 0)
            {
                I("++++himax_debug_flag=0x%x\n", himax_debug_flag);
            }
        }

        I("++++curr_ptr=%d\n", curr_ptr);
        for(k = 0 ; k < PT_ARRAY_SZ ; k++)
        {
            I("[%d] ", point_cnt_array[k]);
            if(k%10 == 9)
            {
                printk("\n");
            }
        }
#endif

	if(buf[0] == 't')
	{
		running_status = 1;
		cancel_delayed_work_sync(&himax_chip_monitor);

		debug_level_cmd = buf[0];
		fw_update_complete = false;

		memset(fileName, 0, 128);
		
		snprintf(fileName, count-2, "%s", &buf[2]);
		I("[TP] %s: upgrade from file(%s) start!\n", __func__, fileName);
		
		filp = filp_open(fileName, O_RDONLY, 0);
		if(IS_ERR(filp))
		{
			E("%s: open firmware file failed\n", __func__);
			goto firmware_upgrade_done;
			
		}
		oldfs = get_fs();
		set_fs(get_ds());

		
		result=filp->f_op->read(filp,upgrade_fw,sizeof(upgrade_fw), &filp->f_pos);
		if(result < 0)
		{
			E("%s: read firmware file failed\n", __func__);
			goto firmware_upgrade_done;
			
		}

		set_fs(oldfs);
		filp_close(filp, NULL);

		I("[TP] %s: upgrade start,len %d: %02X, %02X, %02X, %02X\n", __func__, result, upgrade_fw[0], upgrade_fw[1], upgrade_fw[2], upgrade_fw[3]);

		if(result > 0)
		{
			
			mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
			irq_enable = false;
			if(fts_ctpm_fw_upgrade_with_sys_fs(upgrade_fw, result) <= 0)
			{
				I("[TP] %s: TP upgrade error, line: %d\n", __func__, __LINE__);
				fw_update_complete = false;
			}
			else
			{
				I("[TP] %s: TP upgrade OK, line: %d\n", __func__, __LINE__);
				fw_update_complete = true;
			}
			mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
			irq_enable = true;
			goto firmware_upgrade_done;
			
		}
		running_status = 0;
		queue_delayed_work(himax_wq, &himax_chip_monitor, 10*HZ);


	}

	#ifdef HX_FW_UPDATE_BY_I_FILE
	if(buf[0] == 'f')
	{
		I("[TP] %s: upgrade firmware from kernel image start!\n", __func__);
		if (i_isTP_Updated == 0)
		{
			I("himax touch isTP_Updated: %d\n", i_isTP_Updated);
			if(1)
			{
				mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
				irq_enable = false;
				I("himax touch firmware upgrade: %d\n", i_isTP_Updated);
				if(fts_ctpm_fw_upgrade_with_i_file() == 0)
				{
					I("himax_marked TP upgrade error, line: %d\n", __LINE__);
					fw_update_complete = false;
				}
				else
				{
					I("himax_marked TP upgrade OK, line: %d\n", __LINE__);
					fw_update_complete = true;
				}
				mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
				irq_enable = true;
				i_isTP_Updated = 1;
				goto firmware_upgrade_done;
			}
		}
	}
	#endif

HimaxErr:
	TPD_DMESG("Himax TP: I2C transfer error, line: %d\n", __LINE__);
	return count;

firmware_upgrade_done:

	return count;
}

static DEVICE_ATTR(debug, 0644, himax_debug_show, himax_debug_dump);
#endif

#ifdef HX_TP_SYS_DIAG
static uint8_t *getMutualBuffer(void)
{
	return diag_mutual;
}

static uint8_t *getSelfBuffer(void)
{
	return &diag_self[0];
}

static uint8_t getXChannel(void)
{
	return x_channel;
}

static uint8_t getYChannel(void)
{
	return y_channel;
}

static uint8_t getDiagCommand(void)
{
	return diag_command;
}

static uint8_t *getMutualBuffer_2(void)
{
	return diag_mutual_2;
}

static uint8_t getXChannel_2(void)
{
	return x_channel_2;
}

static uint8_t getYChannel_2(void)
{
	return y_channel_2;
}


static void setXChannel(uint8_t x)
{
	x_channel = x;
}

static void setYChannel(uint8_t y)
{
	y_channel = y;
}

static void setMutualBuffer(void)
{
	diag_mutual = kzalloc(x_channel * y_channel * sizeof(uint8_t), GFP_KERNEL);
}

static void setXChannel_2(uint8_t x)
{
	x_channel_2 = x;
	I("[Himax Debug] %s Set x_channel_2 = %d \n",__func__,x_channel_2);
}

static void setYChannel_2(uint8_t y)
{
	y_channel_2 = y;
	I("[Himax Debug] %s Set y_channel_2 = %d \n",__func__,y_channel_2);
}

static void setMutualBuffer_2(void)
{
	diag_mutual_2 = kzalloc(x_channel_2 * y_channel_2 * sizeof(uint8_t), GFP_KERNEL);
}


static ssize_t himax_diag_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	size_t count = 0;
	uint32_t loop_i;
	uint16_t mutual_num, self_num, width;

	if(Is_2T3R && diag_command == 4)
	{
		mutual_num 	= x_channel_2 * y_channel_2;
		self_num 		= x_channel_2 + y_channel_2; 

		width 			= x_channel_2;
		count += sprintf(buf + count, "ChannelStart: %4d, %4d\n\n", x_channel_2, y_channel_2);
	}
	else
	{

		mutual_num = x_channel * y_channel;
		self_num = x_channel + y_channel; 

		width = x_channel;
		count += sprintf(buf + count, "ChannelStart: %4d, %4d\n\n", x_channel, y_channel);
	}
	
	if (diag_command >= 1 && diag_command <= 6)
	{
		if (diag_command <= 3)
		{
			for (loop_i = 0; loop_i < mutual_num; loop_i++)
			{
				count += sprintf(buf + count, "%4d", diag_mutual[loop_i]);
				if ((loop_i % width) == (width - 1))
				{
				count += sprintf(buf + count, " %3d\n", diag_self[width + loop_i/width]);
				}
			}
			count += sprintf(buf + count, "\n");
			for (loop_i = 0; loop_i < width; loop_i++)
			{
				count += sprintf(buf + count, "%4d", diag_self[loop_i]);
				if (((loop_i) % width) == (width - 1))
				{
					count += sprintf(buf + count, "\n");
				}
			}

			#ifdef HX_EN_SEL_BUTTON
			count += sprintf(buf + count, "\n");
			for (loop_i = 0; loop_i < HX_BT_NUM; loop_i++)
			{
				count += sprintf(buf + count, "%4d", diag_self[HX_RX_NUM + HX_TX_NUM + loop_i]);
			}
			#endif
		}
		else if(Is_2T3R && diag_command == 4 )
		{
			for (loop_i = 0; loop_i < mutual_num; loop_i++)
			{
				count += sprintf(buf + count, "%4d", diag_mutual_2[loop_i]);
				if ((loop_i % width) == (width - 1))
					count += sprintf(buf + count, " %3d\n", diag_self[width + loop_i/width]);
			}

			count += sprintf(buf + count, "\n");
			for (loop_i = 0; loop_i < width; loop_i++)
			{
				count += sprintf(buf + count, "%4d", diag_self[loop_i]);
				if (((loop_i) % width) == (width - 1))
					count += sprintf(buf + count, "\n");
			}

#ifdef HX_EN_SEL_BUTTON
			count += sprintf(buf + count, "\n");
			for (loop_i = 0; loop_i < HX_BT_NUM; loop_i++)
				count += sprintf(buf + count, "%4d", diag_self[HX_RX_NUM_2 + HX_TX_NUM_2 + loop_i]);
#endif

		}
		else if (diag_command > 4)
		{
			for (loop_i = 0; loop_i < self_num; loop_i++)
			{
				count += sprintf(buf + count, "%4d", diag_self[loop_i]);
				if (((loop_i - mutual_num) % width) == (width - 1))
				{
					count += sprintf(buf + count, "\n");
				}
			}
		}
		else
		{
			for (loop_i = 0; loop_i < mutual_num; loop_i++)
			{
				count += sprintf(buf + count, "%4d", diag_mutual[loop_i]);
				if ((loop_i % width) == (width - 1))
				{
					count += sprintf(buf + count, "\n");
				}
			}
		}
		count += sprintf(buf + count, "ChannelEnd");
		count += sprintf(buf + count, "\n");
	}
	else if (diag_command == 7)
	{
		for (loop_i = 0; loop_i < 128 ;loop_i++)
		{
			if((loop_i % 16) == 0)
			{
				count += sprintf(buf + count, "LineStart:");
			}

			count += sprintf(buf + count, "%4d", diag_coor[loop_i]);
			if((loop_i % 16) == 15)
			{
				count += sprintf(buf + count, "\n");
			}
		}
	}
	return count;
}

static ssize_t himax_diag_dump(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	const uint8_t command_ec_128_raw_flag = 0x01;
	const uint8_t command_ec_24_normal_flag = 0x00;

	uint8_t command_ec_128_raw_baseline_flag = 0x02;
	uint8_t command_ec_128_raw_bank_flag = 0x03;

	uint8_t command_91h[2] = {0x91, 0x00};
	uint8_t command_82h[1] = {0x82};
	uint8_t command_F3h[2] = {0xF3, 0x00};
	uint8_t command_83h[1] = {0x83};
	uint8_t receive[1];

	if (IC_TYPE != HX_85XX_D_SERIES_PWON)
	{
		command_ec_128_raw_baseline_flag = 0x02 | command_ec_128_raw_flag;
	}
	else
	{
		command_ec_128_raw_baseline_flag = 0x02;
		command_ec_128_raw_bank_flag = 0x03;
	}

	if (buf[0] == '1') 
	{
		command_91h[1] = command_ec_128_raw_baseline_flag; 
		himax_i2c_write_data(i2c_client, command_91h[0], 1, &(command_91h[1]));
		diag_command = buf[0] - '0';
		E("[Himax]diag_command=0x%x\n",diag_command);
	}
	else if (buf[0] == '2')	
	{
		command_91h[1] = command_ec_128_raw_flag;	
		himax_i2c_write_data(i2c_client, command_91h[0], 1, &(command_91h[1]));
		diag_command = buf[0] - '0';
		E("[Himax]diag_command=0x%x\n",diag_command);
	}
	else if (buf[0] == '3')	
	{
		if (IC_TYPE != HX_85XX_D_SERIES_PWON)
		{
			himax_i2c_write_data(i2c_client, command_82h[0], 0, &(command_82h[0]));
			msleep(50);

			himax_i2c_read_data(i2c_client, command_F3h[0], 1, &(receive[0]));
			command_F3h[1] = (receive[0] | 0x80);

			himax_i2c_write_data(i2c_client, command_F3h[0], 1, &(command_F3h[1]));

			command_91h[1] = command_ec_128_raw_baseline_flag;
			himax_i2c_write_data(i2c_client, command_91h[0], 1, &(command_91h[0]));

			himax_i2c_write_data(i2c_client, command_83h[0], 0, &(command_83h[0]));
			msleep(50);
		}
		else
		{
			command_91h[1] = command_ec_128_raw_bank_flag;	
			himax_i2c_write_data(i2c_client, command_91h[0], 1, &(command_91h[1]));
		}
		diag_command = buf[0] - '0';
		E("[Himax]diag_command=0x%x\n",diag_command);
	}
	else if (buf[0] == '4' ) 
	{
		command_91h[1] = 0x04;
		himax_i2c_write_data(i2c_client, command_91h[0], 1, &(command_91h[1]));
		diag_command = buf[0] - '0';
		E("[Himax]diag_command=0x%x\n",diag_command);
	}

	else if (buf[0] == '7')
	{
		diag_command = buf[0] - '0';
	}
	
	else if (buf[0] == '8')
	{
		diag_command = buf[0] - '0';

		coordinate_fn = filp_open(DIAG_COORDINATE_FILE,O_CREAT | O_WRONLY | O_APPEND | O_TRUNC,0666);
		if(IS_ERR(coordinate_fn))
		{
			I("[HIMAX TP ERROR]%s: coordinate_dump_file_create error\n", __func__);
			coordinate_dump_enable = 0;
			filp_close(coordinate_fn,NULL);
		}
		coordinate_dump_enable = 1;
	}
	else if (buf[0] == '9')
	{
		coordinate_dump_enable = 0;
		diag_command = buf[0] - '0';

		if(!IS_ERR(coordinate_fn))
		{
			filp_close(coordinate_fn,NULL);
		}
	}
	
	else
	{
		if (IC_TYPE != HX_85XX_D_SERIES_PWON)
		{
			himax_i2c_write_data(i2c_client, command_82h[0], 0, &(command_82h[0]));
			msleep(50);

			command_91h[1] = command_ec_24_normal_flag;
			himax_i2c_write_data(i2c_client, command_91h[0], 1, &(command_91h[1]));
			himax_i2c_read_data(i2c_client, command_F3h[0], 1, &(receive[0]));
			command_F3h[1] = (receive[0] & 0x7F);
			himax_i2c_write_data(i2c_client, command_F3h[0], 1, &(command_F3h[1]));
			himax_i2c_write_data(i2c_client, command_83h[0], 0, &(command_83h[0]));
		}
		else
		{
			command_91h[1] = command_ec_24_normal_flag;
			himax_i2c_write_data(i2c_client, command_91h[0], 1, &(command_91h[1]));
		}
		diag_command = 0;
		E("[Himax]diag_command=0x%x\n",diag_command);
	}
	return count;
}
static DEVICE_ATTR(diag, 0644,himax_diag_show, himax_diag_dump);
#endif

#ifdef HX_TP_SYS_FLASH_DUMP

static uint8_t getFlashCommand(void)
{
	return flash_command;
}

static uint8_t getFlashDumpProgress(void)
{
	return flash_progress;
}

static uint8_t getFlashDumpComplete(void)
{
	return flash_dump_complete;
}

static uint8_t getFlashDumpFail(void)
{
	return flash_dump_fail;
}

static uint8_t getSysOperation(void)
{
	return sys_operation;
}

static uint8_t getFlashReadStep(void)
{
	return flash_read_step;
}

static uint8_t getFlashDumpSector(void)
{
	return flash_dump_sector;
}

static uint8_t getFlashDumpPage(void)
{
	return flash_dump_page;
}

static bool getFlashDumpGoing(void)
{
	return flash_dump_going;
}

static void setFlashBuffer(void)
{
	int i=0;
	flash_buffer = kzalloc(32768*sizeof(uint8_t), GFP_KERNEL);
	for(i=0; i<32768; i++)
	{
		flash_buffer[i] = 0x00;
	}
}

static void setSysOperation(uint8_t operation)
{
	sys_operation = operation;
}

static void setFlashDumpProgress(uint8_t progress)
{
	flash_progress = progress;
	I("TPPPP setFlashDumpProgress : progress = %d ,flash_progress = %d \n",progress,flash_progress);
}

static void setFlashDumpComplete(uint8_t status)
{
	flash_dump_complete = status;
}

static void setFlashDumpFail(uint8_t fail)
{
	flash_dump_fail = fail;
}

static void setFlashCommand(uint8_t command)
{
	flash_command = command;
}

static void setFlashReadStep(uint8_t step)
{
	flash_read_step = step;
}

static void setFlashDumpSector(uint8_t sector)
{
	flash_dump_sector = sector;
}

static void setFlashDumpPage(uint8_t page)
{
	flash_dump_page = page;
}

static void setFlashDumpGoing(bool going)
{
	flash_dump_going = going;
}

static ssize_t himax_flash_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	int ret = 0;
	int loop_i;
	uint8_t local_flash_read_step =0;
	uint8_t local_flash_complete = 0;
	uint8_t local_flash_progress = 0;
	uint8_t local_flash_command = 0;
	uint8_t local_flash_fail = 0;

	local_flash_complete = getFlashDumpComplete();
	local_flash_progress = getFlashDumpProgress();
	local_flash_command = getFlashCommand();
	local_flash_fail = getFlashDumpFail();

	I("TPPPP flash_progress = %d \n",local_flash_progress);

	if(local_flash_fail)
	{
		ret += sprintf(buf+ret, "FlashStart:Fail \n");
		ret += sprintf(buf + ret, "FlashEnd");
		ret += sprintf(buf + ret, "\n");
		return ret;
	}

	if(!local_flash_complete)
	{
		ret += sprintf(buf+ret, "FlashStart:Ongoing:0x%2.2x \n",flash_progress);
		ret += sprintf(buf + ret, "FlashEnd");
		ret += sprintf(buf + ret, "\n");
		return ret;
	}

	if(local_flash_command == 1 && local_flash_complete)
	{
		ret += sprintf(buf+ret, "FlashStart:Complete \n");
		ret += sprintf(buf + ret, "FlashEnd");
		ret += sprintf(buf + ret, "\n");
		return ret;
	}

	if(local_flash_command == 3 && local_flash_complete)
	{
		ret += sprintf(buf+ret, "FlashStart: \n");
		for(loop_i = 0; loop_i < 128; loop_i++)
		{
			ret += sprintf(buf + ret, "x%2.2x", flash_buffer[loop_i]);
			if((loop_i % 16) == 15)
			{
				ret += sprintf(buf + ret, "\n");
			}
		}
		ret += sprintf(buf + ret, "FlashEnd");
		ret += sprintf(buf + ret, "\n");
		return ret;
	}

	
	local_flash_read_step = getFlashReadStep();

	ret += sprintf(buf+ret, "FlashStart:%2.2x \n",local_flash_read_step);

	for (loop_i = 0; loop_i < 1024; loop_i++)
	{
		ret += sprintf(buf + ret, "x%2.2X", flash_buffer[local_flash_read_step*1024 + loop_i]);

		if ((loop_i % 16) == 15)
		{
			ret += sprintf(buf + ret, "\n");
		}
	}

	ret += sprintf(buf + ret, "FlashEnd");
	ret += sprintf(buf + ret, "\n");
	return ret;
}

static ssize_t himax_flash_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	char buf_tmp[6];
	unsigned long result = 0;
	uint8_t loop_i = 0;
	int base = 0;

	memset(buf_tmp, 0x0, sizeof(buf_tmp));

	I("[TP] %s: buf[0] = %s\n", __func__, buf);

	if(getSysOperation() == 1)
	{
		I("[TP] %s: SYS is busy , return!\n", __func__);
		return count;
	}

	if(buf[0] == '0')
	{
		setFlashCommand(0);
		if(buf[1] == ':' && buf[2] == 'x')
		{
			memcpy(buf_tmp, buf + 3, 2);
			I("[TP] %s: read_Step = %s\n", __func__, buf_tmp);
			if (!strict_strtoul(buf_tmp, 16, &result))
			{
				I("[TP] %s: read_Step = %lu \n", __func__, result);
				setFlashReadStep(result);
			}
		}
	}
	else if(buf[0] == '1')
	{
		setSysOperation(1);
		setFlashCommand(1);
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);
		queue_work(flash_wq, &flash_work);
	}
	else if(buf[0] == '2')
	{
		setSysOperation(1);
		setFlashCommand(2);
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);

		queue_work(flash_wq, &flash_work);
	}
	else if(buf[0] == '3')
	{
		setSysOperation(1);
		setFlashCommand(3);
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);

		memcpy(buf_tmp, buf + 3, 2);
		if (!strict_strtoul(buf_tmp, 16, &result))
		{
			setFlashDumpSector(result);
		}

		memcpy(buf_tmp, buf + 7, 2);
		if (!strict_strtoul(buf_tmp, 16, &result))
		{
			setFlashDumpPage(result);
		}

		queue_work(flash_wq, &flash_work);
	}
	else if(buf[0] == '4')
	{
		I("[TP] %s: command 4 enter.\n", __func__);
		setSysOperation(1);
		setFlashCommand(4);
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);

		memcpy(buf_tmp, buf + 3, 2);
		if (!strict_strtoul(buf_tmp, 16, &result))
		{
			setFlashDumpSector(result);
		}
		else
		{
			I("[TP] %s: command 4 , sector error.\n", __func__);
			return count;
		}

		memcpy(buf_tmp, buf + 7, 2);
		if (!strict_strtoul(buf_tmp, 16, &result))
		{
			setFlashDumpPage(result);
		}
		else
		{
			I("[TP] %s: command 4 , page error.\n", __func__);
			return count;
		}

		base = 11;

		I("=========Himax flash page buffer start=========\n");
		for(loop_i=0;loop_i<128;loop_i++)
		{
			memcpy(buf_tmp, buf + base, 2);
			if (!strict_strtoul(buf_tmp, 16, &result))
			{
				flash_buffer[loop_i] = result;
				I(" %d ",flash_buffer[loop_i]);
				if(loop_i % 16 == 15)
				{
					printk("\n");
				}
			}
			base += 3;
		}
		I("=========Himax flash page buffer end=========\n");

		queue_work(flash_wq, &flash_work);
	}
	return count;
}
static DEVICE_ATTR(flash_dump, 0644,himax_flash_show, himax_flash_store);

static void himax_ts_flash_work_func(struct work_struct *work)
{
	uint8_t page_tmp[128];
	uint8_t x59_tmp[4] = {0,0,0,0};
	int i=0, j=0, k=0, l=0, buffer_ptr = 0, flash_end_count = 0;
	uint8_t local_flash_command = 0;
	uint8_t sector = 0;
	uint8_t page = 0;

	uint8_t x81_command[2] = {0x81,0x00};
	uint8_t x82_command[2] = {0x82,0x00};
	uint8_t x42_command[2] = {0x42,0x00};
	uint8_t x43_command[4] = {0x43,0x00,0x00,0x00};
	uint8_t x44_command[4] = {0x44,0x00,0x00,0x00};
	uint8_t x45_command[5] = {0x45,0x00,0x00,0x00,0x00};
	uint8_t x46_command[2] = {0x46,0x00};
	uint8_t x4A_command[2] = {0x4A,0x00};
	uint8_t x4D_command[2] = {0x4D,0x00};
	

	mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
	irq_enable = false;
	setFlashDumpGoing(true);

	#ifdef HX_RST_PIN_FUNC
	himax_HW_reset();
	#endif

	sector = getFlashDumpSector();
	page   = getFlashDumpPage();

	local_flash_command = getFlashCommand();

	if( himax_i2c_write_data(i2c_client, x81_command[0], 0, &(x81_command[1])) < 0 )
	{
		E("%s i2c write 81 fail.\n",__func__);
		goto Flash_Dump_i2c_transfer_error;
	}
	msleep(120);

	if( himax_i2c_write_data(i2c_client, x82_command[0], 0, &(x82_command[1])) < 0 )
	{
		E("%s i2c write 82 fail.\n",__func__);
		goto Flash_Dump_i2c_transfer_error;
	}
	msleep(100);

	I("[TP] %s: local_flash_command = %d enter.\n", __func__,local_flash_command);
	I("[TP] %s: flash buffer start.\n", __func__);
	for(i=0;i<128;i++)
	{
		I(" %2.2x ",flash_buffer[i]);
		if((i%16) == 15)
		{
			printk("\n");
		}
	}
	I("[TP] %s: flash buffer end.\n", __func__);

	if(local_flash_command == 1 || local_flash_command == 2)
	{
		x43_command[1] = 0x01;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 1, &(x43_command[1])) < 0 )
		{
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(100);

		for( i=0 ; i<8 ;i++)
		{
			for(j=0 ; j<32 ; j++)
			{
				I("TPPPP Step 2 i=%d , j=%d %s\n",i,j,__func__);
				
				for(k=0; k<128; k++)
				{
					page_tmp[k] = 0x00;
				}
				for(k=0; k<32; k++)
				{
					x44_command[1] = k;
					x44_command[2] = j;
					x44_command[3] = i;
					if( himax_i2c_write_data(i2c_client, x44_command[0], 3, &(x44_command[1])) < 0 )
					{
						E("%s i2c write 44 fail.\n",__func__);
						goto Flash_Dump_i2c_transfer_error;
					}

					if( himax_i2c_write_data(i2c_client, x46_command[0], 0, &(x46_command[1])) < 0 )
					{
						E("%s i2c write 46 fail.\n",__func__);
						goto Flash_Dump_i2c_transfer_error;
					}
					
					if( himax_i2c_read_data(i2c_client, 0x59, 4, &(x59_tmp[0])) < 0 )
					{
						E("%s i2c write 59 fail.\n",__func__);
						goto Flash_Dump_i2c_transfer_error;
					}
					
					for(l=0; l<4; l++)
					{
						page_tmp[k*4+l] = x59_tmp[l];
					}
					
				}
				

				for(k=0; k<128; k++)
				{
					flash_buffer[buffer_ptr++] = page_tmp[k];

					if(page_tmp[k] == 0xFF)
					{
						flash_end_count ++;
						if(flash_end_count == 32)
						{
							flash_end_count = 0;
							buffer_ptr = buffer_ptr -32;
							goto FLASH_END;
						}
					}
					else
					{
						flash_end_count = 0;
					}
				}
				setFlashDumpProgress(i*32 + j);
			}
		}
	}
	else if(local_flash_command == 3)
	{
		x43_command[1] = 0x01;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 1, &(x43_command[1])) < 0 )
		{
			E("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(100);

		for(i=0; i<128; i++)
		{
			page_tmp[i] = 0x00;
		}

		for(i=0; i<32; i++)
		{
			x44_command[1] = i;
			x44_command[2] = page;
			x44_command[3] = sector;

			if( himax_i2c_write_data(i2c_client, x44_command[0], 3, &(x44_command[1])) < 0 )
			{
				E("%s i2c write 44 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}

			if( himax_i2c_write_data(i2c_client, x46_command[0], 0, &(x46_command[1])) < 0 )
			{
				E("%s i2c write 46 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			
			if( himax_i2c_read_data(i2c_client, 0x59, 4, &(x59_tmp[0])) < 0 )
			{
				E("%s i2c write 59 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			
			for(j=0; j<4; j++)
			{
				page_tmp[i*4+j] = x59_tmp[j];
			}
			
		}
		
		for(i=0; i<128; i++)
		{
			flash_buffer[buffer_ptr++] = page_tmp[i];
		}
	}
	else if(local_flash_command == 4)
	{
		
		I("[TP] %s: local_flash_command = 4, enter.\n", __func__);

		
		
		
		x43_command[1] = 0x01;
		x43_command[2] = 0x00;
		x43_command[3] = 0x06;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 3, &(x43_command[1])) < 0 )
		{
			E("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x44_command[1] = 0x03;
		x44_command[2] = 0x00;
		x44_command[3] = 0x00;
		if( himax_i2c_write_data(i2c_client, x44_command[0], 3, &(x44_command[1])) < 0 )
		{
			E("%s i2c write 44 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x45_command[1] = 0x00;
		x45_command[2] = 0x00;
		x45_command[3] = 0x3D;
		x45_command[4] = 0x03;
		if( himax_i2c_write_data(i2c_client, x45_command[0], 4, &(x45_command[1])) < 0 )
		{
			E("%s i2c write 45 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		if( himax_i2c_write_data(i2c_client, x4A_command[0], 0, &(x4A_command[1])) < 0 )
		{
			E("%s i2c write 4A fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(50);

		
		
		
		x43_command[1] = 0x01;
		x43_command[2] = 0x00;
		x43_command[3] = 0x02;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 3, &(x43_command[1])) < 0 )
		{
			E("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x44_command[1] = 0x00;
		x44_command[2] = page;
		x44_command[3] = sector;
		if( himax_i2c_write_data(i2c_client, x44_command[0], 3, &(x44_command[1])) < 0 )
		{
			E("%s i2c write 44 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		if( himax_i2c_write_data(i2c_client, x4D_command[0], 0, &(x4D_command[1])) < 0 )
		{
			E("%s i2c write 4D fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(100);

		
		
		
		x42_command[1] = 0x01;
		if( himax_i2c_write_data(i2c_client, x42_command[0], 1, &(x42_command[1])) < 0 )
		{
			E("%s i2c write 42 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(100);

		
		
		
		x43_command[1] = 0x01;
		x43_command[2] = 0x00;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 2, &(x43_command[1])) < 0 )
		{
			E("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		
		
		
		x44_command[1] = 0x00;
		x44_command[2] = page;
		x44_command[3] = sector;
		if( himax_i2c_write_data(i2c_client, x44_command[0], 3, &(x44_command[1])) < 0 )
		{
			E("%s i2c write 44 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		
		
		
		x43_command[1] = 0x01;
		x43_command[2] = 0x09;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 2, &(x43_command[1])) < 0 )
		{
			E("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x43_command[1] = 0x01;
		x43_command[2] = 0x0D;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 2, &(x43_command[1])) < 0 )
		{
			E("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x43_command[1] = 0x01;
		x43_command[2] = 0x09;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 2, &(x43_command[1])) < 0 )
		{
			E("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		for(i=0; i<32; i++)
		{
			I("himax :i=%d \n",i);
			x44_command[1] = i;
			x44_command[2] = page;
			x44_command[3] = sector;
			if( himax_i2c_write_data(i2c_client, x44_command[0], 3, &(x44_command[1])) < 0 )
			{
				E("%s i2c write 44 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			msleep(10);

			x45_command[1] = flash_buffer[i*4 + 0];
			x45_command[2] = flash_buffer[i*4 + 1];
			x45_command[3] = flash_buffer[i*4 + 2];
			x45_command[4] = flash_buffer[i*4 + 3];
			if( himax_i2c_write_data(i2c_client, x45_command[0], 4, &(x45_command[1])) < 0 )
			{
				E("%s i2c write 45 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			msleep(10);

			
			// manual mode command : 48 ,data will be written into flash buffer
			
			x43_command[1] = 0x01;


			x43_command[2] = 0x0D;
			if( himax_i2c_write_data(i2c_client, x43_command[0], 2, &(x43_command[1])) < 0 )
			{
				E("%s i2c write 43 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			msleep(10);

			x43_command[1] = 0x01;
			x43_command[2] = 0x09;
			if( himax_i2c_write_data(i2c_client, x43_command[0], 2, &(x43_command[1])) < 0 )
			{
				E("%s i2c write 43 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			msleep(10);
		}

		
		
		
		x43_command[1] = 0x01;
		x43_command[2] = 0x01;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 2, &(x43_command[1])) < 0 )
		{
			E("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x43_command[1] = 0x01;
		x43_command[2] = 0x05;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 2, &(x43_command[1])) < 0 )
		{
			E("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x43_command[1] = 0x01;
		x43_command[2] = 0x01;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 2, &(x43_command[1])) < 0 )
		{
			E("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x43_command[1] = 0x01;
		x43_command[2] = 0x00;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 2, &(x43_command[1])) < 0 )
		{
			E("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		
		
		
		x43_command[1] = 0x00;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 1, &(x43_command[1])) < 0 )
		{
			E("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		
		
		
		x42_command[1] = 0x00;
		if( himax_i2c_write_data(i2c_client, x42_command[0], 1, &(x42_command[1])) < 0 )
		{
			E("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		
		
		
		x43_command[1] = 0x01;
		x43_command[2] = 0x00;
		x43_command[3] = 0x06;
		if( himax_i2c_write_data(i2c_client, x43_command[0], 3, &(x43_command[1])) < 0 )
		{
			E("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x44_command[1] = 0x03;
		x44_command[2] = 0x00;
		x44_command[3] = 0x00;
		if( himax_i2c_write_data(i2c_client, x44_command[0], 3, &(x44_command[1])) < 0 )
		{
			E("%s i2c write 44 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x45_command[1] = 0x00;
		x45_command[2] = 0x00;
		x45_command[3] = 0x7D;
		x45_command[4] = 0x03;
		if( himax_i2c_write_data(i2c_client, x45_command[0], 4, &(x45_command[1])) < 0 )
		{
			E("%s i2c write 45 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		if( himax_i2c_write_data(i2c_client, x4A_command[0], 0, &(x4A_command[1])) < 0 )
		{
			E("%s i2c write 4D fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		msleep(50);

		buffer_ptr = 128;
		I("Himax: Flash page write Complete~~~~~~~~~~~~~~~~~~~~~~~\n");
	}

FLASH_END:

	I("Complete~~~~~~~~~~~~~~~~~~~~~~~\n");

	I(" buffer_ptr = %d \n",buffer_ptr);

	for (i = 0; i < buffer_ptr; i++)
	{
		I("%2.2X ", flash_buffer[i]);

		if ((i % 16) == 15)
		{
			printk("\n");
		}
	}
	I("End~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

	himax_i2c_write_data(i2c_client, x43_command[0], 0, &(x43_command[1]));
	msleep(50);

	if(local_flash_command == 2)
	{
		struct file *fn;

		fn = filp_open(FLASH_DUMP_FILE,O_CREAT | O_WRONLY ,0);
		if(!IS_ERR(fn))
		{
			fn->f_op->write(fn,flash_buffer,buffer_ptr*sizeof(uint8_t),&fn->f_pos);
			filp_close(fn,NULL);
		}
	}



	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	irq_enable = true;
	setFlashDumpGoing(false);

	setFlashDumpComplete(1);
	setSysOperation(0);
	return;

Flash_Dump_i2c_transfer_error:



	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	irq_enable = true;
	setFlashDumpGoing(false);
	setFlashDumpComplete(0);
	setFlashDumpFail(1);
	setSysOperation(0);
	return;
}
#endif

#ifdef HX_TP_SYS_SELF_TEST
static ssize_t himax_chip_self_test_function(struct device *dev, struct device_attribute *attr, char *buf)
{
	int val=0x00;
	val = himax_chip_self_test();
	return sprintf(buf, "%d\n",val);
}

static int himax_chip_self_test(void)
{
	uint8_t cmdbuf[11];
	int ret = 0;
	uint8_t valuebuf[16];
	int i=0, pf_value=0x00;
#ifdef HX_FLASH_TEST 
	if (flash_checksum_pass == false)
	{
		I("%s, firmware checksum failed!\n", __func__);
		return -1;
	}
#endif

	
	#ifdef HX_RST_PIN_FUNC
	himax_HW_reset();
	himax_ts_poweron();
	#endif
	

	if (IC_TYPE == HX_85XX_C_SERIES_PWON)
	{
		
		cmdbuf[0] = 0x82;
		ret = himax_i2c_write_data(i2c_client, cmdbuf[0], 0, &(cmdbuf[1]));
		if(ret < 0)
		{
			E("[Himax]:write TSSOFF failed line: %d \n",__LINE__);
		}
		mdelay(120); 

		cmdbuf[0] = 0x8D;
		cmdbuf[1] = 0xB4; 
		cmdbuf[2] = 0x64; 
		cmdbuf[3] = 0x36;
		cmdbuf[4] = 0x09;
		cmdbuf[5] = 0x2D;
		cmdbuf[6] = 0x09;
		cmdbuf[7] = 0x32;
		cmdbuf[8] = 0x08; 
		ret = himax_i2c_write_data(i2c_client, cmdbuf[0], 8, &(cmdbuf[1]));
		if(ret < 0)
		{
			E("[Himax]:write HX_CMD_SELFTEST_BUFFER failed line: %d \n",__LINE__);
		}

		udelay(100);

		ret = himax_i2c_read_data(i2c_client, 0x8D, 9, &(valuebuf[0]));
		if(ret < 0)
		{
			E("[Himax]:read HX_CMD_SELFTEST_BUFFER failed line: %d \n",__LINE__);
		}
		I("[Himax]:0x8D[0] = 0x%x\n",valuebuf[0]);
		I("[Himax]:0x8D[1] = 0x%x\n",valuebuf[1]);
		I("[Himax]:0x8D[2] = 0x%x\n",valuebuf[2]);
		I("[Himax]:0x8D[3] = 0x%x\n",valuebuf[3]);
		I("[Himax]:0x8D[4] = 0x%x\n",valuebuf[4]);
		I("[Himax]:0x8D[5] = 0x%x\n",valuebuf[5]);
		I("[Himax]:0x8D[6] = 0x%x\n",valuebuf[6]);
		I("[Himax]:0x8D[7] = 0x%x\n",valuebuf[7]);
		I("[Himax]:0x8D[8] = 0x%x\n",valuebuf[8]);

		cmdbuf[0] = 0xE9;
		cmdbuf[1] = 0x00;
		cmdbuf[2] = 0x06;
		ret = himax_i2c_write_data(i2c_client, cmdbuf[0], 2, &(cmdbuf[1]));
		if(ret < 0)
		{
			E("[Himax]:write sernsor to self-test mode failed line: %d \n",__LINE__);
		}
		udelay(100);

		ret = himax_i2c_read_data(i2c_client, 0xE9, 3, &(valuebuf[0]));
		if(ret < 0)
		{
			E("[Himax]:read 0xE9 failed line: %d \n",__LINE__);
		}
		I("[Himax]:0xE9[0] = 0x%x\n",valuebuf[0]);
		I("[Himax]:0xE9[1] = 0x%x\n",valuebuf[1]);
		I("[Himax]:0xE9[2] = 0x%x\n",valuebuf[2]);

		cmdbuf[0] = 0x83;
		ret = himax_i2c_write_data(i2c_client, cmdbuf[0], 0, &(cmdbuf[1]));
		if(ret < 0)
		{
			E("[Himax]:write HX_CMD_TSSON failed line: %d \n",__LINE__);
		}
		mdelay(1500); 

		cmdbuf[0] = 0x82;
		ret = himax_i2c_write_data(i2c_client, cmdbuf[0], 0, &(cmdbuf[1]));
		if(ret < 0)
		{
			E("[Himax]:write TSSOFF failed line: %d \n",__LINE__);
		}

		mdelay(120); 

		memset(valuebuf, 0x00 , 16);
		ret = himax_i2c_read_data(i2c_client, 0x8D, 16, &(valuebuf[0]));
		if(ret < 0)
		{
			E("[Himax]:read HX_CMD_FW_VERSION_ID failed line: %d \n",__LINE__);
		}
		else
		{
			if(valuebuf[0]==0xAA)
			{
				I("[Himax]: self-test pass\n");
				pf_value = 0x0;
			}
			else
			{
				I("[Himax]: self-test fail\n");
				pf_value = 0x1;
				for(i=0;i<16;i++)
				{
					I("[Himax]:0x8D buff[%d] = 0x%x\n",i,valuebuf[i]);
				}
			}
		}
		mdelay(120); 

		cmdbuf[0] = 0xE9;
		cmdbuf[1] = 0x00;
		cmdbuf[2] = 0x00;
		ret = himax_i2c_write_data(i2c_client, cmdbuf[0], 2, &(cmdbuf[1]));
		if(ret < 0)
		{
			E("[Himax]:write sensor to normal mode failed line: %d \n",__LINE__);
		}
		mdelay(120); 

		cmdbuf[0] = 0x83;
		ret = himax_i2c_write_data(i2c_client, cmdbuf[0], 0, &(cmdbuf[1]));
		if(ret < 0)
		{
			E("[Himax]:write HX_CMD_TSSON failed line: %d \n",__LINE__);
		}
		msleep(120); 
	}
	else if(IC_TYPE == HX_85XX_D_SERIES_PWON)
	{
		
		himax_i2c_write_data(i2c_client, 0x82, 0, &(cmdbuf[0]));
		msleep(120);

		
		
		cmdbuf[0] = 0x15;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));
		msleep(10);

		cmdbuf[0] = 0x00;
		cmdbuf[1] = 0x02; 
		himax_i2c_write_data(i2c_client, 0xD8, 2, &(cmdbuf[0]));
		msleep(10);

		himax_i2c_read_data(i2c_client, 0x5A, 2, &(valuebuf[0]));
		msleep(10);

		cmdbuf[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));

		msleep(30);

		I("[Himax]:0xFE02_0 = 0x%x\n",valuebuf[0]);
		I("[Himax]:0xFE02_1 = 0x%x\n",valuebuf[1]);

		valuebuf[0] = valuebuf[1] & 0xFD; 

		I("[Himax]:0xFE02_valuebuf = 0x%x\n",valuebuf[0]);

		
		cmdbuf[0] = 0x15;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));
		msleep(10);

		cmdbuf[0] = 0x00;
		cmdbuf[1] = 0x02; 
		himax_i2c_write_data(i2c_client, 0xD8, 2, &(cmdbuf[0]));
		msleep(10);

		cmdbuf[0] = valuebuf[0];
		himax_i2c_write_data(i2c_client, 0x40, 1, &(cmdbuf[0]));
		msleep(10);

		cmdbuf[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));

		msleep(30);
		

		
		cmdbuf[0] = 0x15;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));
		msleep(10);

		cmdbuf[0] = 0x00;
		cmdbuf[1] = 0x02; 
		himax_i2c_write_data(i2c_client, 0xD8, 2, &(cmdbuf[0]));
		msleep(10);

		himax_i2c_read_data(i2c_client, 0x5A, 2, &(valuebuf[0]));
		msleep(10);

		cmdbuf[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));
		msleep(30);

		I("[Himax]:0xFE02_0_back = 0x%x\n",valuebuf[0]);
		I("[Himax]:0xFE02_1_back = 0x%x\n",valuebuf[1]);

		
		cmdbuf[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0xE3, 1, &(cmdbuf[0]));

		msleep(30);

		himax_i2c_read_data(i2c_client, 0xE3, 1, &(valuebuf[0]));

		I("[Himax]:0xE3_back = 0x%x\n",valuebuf[0]);

		
		
		cmdbuf[0] = 0x15;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));
		msleep(10);

		cmdbuf[0] = 0x00;
		cmdbuf[1] = 0x96; 
		himax_i2c_write_data(i2c_client, 0xD8, 2, &(cmdbuf[0]));
		msleep(10);

		
		cmdbuf[0] = 0xB4;
		cmdbuf[1] = 0x64;
		cmdbuf[2] = 0x3F;
		cmdbuf[3] = 0x3F;
		cmdbuf[4] = 0x3C;
		cmdbuf[5] = 0x00;
		cmdbuf[6] = 0x3C;
		cmdbuf[7] = 0x00;
		himax_i2c_write_data(i2c_client, 0x40, 8, &(cmdbuf[0]));
		msleep(10);

		cmdbuf[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));

		msleep(30);

		
		cmdbuf[0] = 0x15;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));
		msleep(10);

		cmdbuf[0] = 0x00;
		cmdbuf[1] = 0x96; 
		himax_i2c_write_data(i2c_client, 0xD8, 2, &(cmdbuf[0]));
		msleep(10);

		himax_i2c_read_data(i2c_client, 0x5A, 16, &(valuebuf[0]));
		msleep(10);

		cmdbuf[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));

		for(i=1;i<16;i++)
		{
			I("[Himax]:0xFE96 buff_back[%d] = 0x%x\n",i,valuebuf[i]);
		}

		msleep(30);

		
		cmdbuf[0] = 0x16;
		himax_i2c_write_data(i2c_client, 0x91, 1, &(cmdbuf[0]));

		himax_i2c_read_data(i2c_client, 0x91, 1, &(valuebuf[0]));

		I("[Himax]:0x91_back = 0x%x\n",valuebuf[0]);
		msleep(10);

		
		himax_i2c_write_data(i2c_client, 0x83, 0, &(cmdbuf[0]));

		mdelay(2000);

		
		himax_i2c_write_data(i2c_client, 0x82, 0, &(cmdbuf[0]));

		msleep(30);

		
		cmdbuf[0] = 0x15;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));
		msleep(10);

		cmdbuf[0] = 0x00;
		cmdbuf[1] = 0x96; 
		himax_i2c_write_data(i2c_client, 0xD8, 2, &(cmdbuf[0]));
		msleep(10);

		himax_i2c_read_data(i2c_client, 0x5A, 16, &(valuebuf[0]));
		msleep(10);

		cmdbuf[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0xE1, 1, &(cmdbuf[0]));

		
		cmdbuf[0] = 0x00;
		himax_i2c_write_data(i2c_client, 0x91, 1, &(cmdbuf[0]));

		if(valuebuf[1]==0xAA) 
		{
			I("[Himax]: self-test pass\n");
			pf_value = 0x0;
		}
		else
		{
			I("[Himax]: self-test fail\n");
			pf_value = 0x1;
			for(i=1;i<16;i++)
			{
				I("[Himax]:0xFE96 buff[%d] = 0x%x\n",i,valuebuf[i]);
			}
		}

		
		
			#ifdef HX_RST_PIN_FUNC
			himax_HW_reset();
			#endif
		

		himax_ts_poweron();
	}
	return pf_value;
}

static DEVICE_ATTR(tp_self_test, (S_IWUSR|S_IRUGO), himax_chip_self_test_function, NULL);
#endif

#ifdef HX_TP_SYS_VENDOR
static ssize_t touch_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	uint8_t namebuf[32];
	int sensorid = 0;

	memset(namebuf, 0x0, sizeof(namebuf));

	switch (hx8526_ctp_module_name)
	{
		case 0: sprintf(namebuf, "Truly");
				sensorid = 0x21;
				break;
		case 1: sprintf(namebuf, "Yanghua");
				sensorid = 0x22;
				break;
		case 2: sprintf(namebuf, "Shenyue");
				sensorid = 0x12;
				break;
		default:sprintf(namebuf, "Unknown!");
				break;
	}

	ret += sprintf(buf, "%s_FW:%#x_iFileVer:%#x_SensorId:%s(%#x)\n", HIMAX8528_NAME,
			g_fw_verion, g_ifile_version, namebuf, sensorid);

	return ret;
}

static DEVICE_ATTR(vendor, (S_IRUGO), touch_vendor_show, NULL);
#endif

#ifdef HX_TP_SYS_ATTN
static ssize_t touch_attn_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	sprintf(buf, "attn = %x\n", mt_get_gpio_in(GPIO_CTP_EINT_PIN));
	ret = strlen(buf) + 1;

	return ret;
}

static DEVICE_ATTR(attn, (S_IRUGO), touch_attn_show, NULL);
#endif

#ifdef HX_TP_SYS_INT
static ssize_t himax_int_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	size_t count = 0;

	count += sprintf(buf + count, "%s", irq_enable == true ? "IRQ is enable \n" : "IRQ is disable \n");

	return count;
}

static ssize_t himax_int_status_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	if( buf[0] == '1') 
	{
		mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
		irq_enable = true;
	}
	else if(buf[0] == '0') 
	{
		mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
		irq_enable = false;
	}
	else
	{
		E("[Himax] %s: Parameter error.\n", __func__);
	}
	return count;

}

static DEVICE_ATTR(enabled, (S_IWUSR|S_IRUGO),
	himax_int_status_show, himax_int_status_store);
#endif


#ifdef HX_TP_SYS_RESET
static ssize_t himax_reset_set(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	if (buf[0] == '1')
	{
		mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
		irq_enable = false;
#ifdef HX_RST_PIN_FUNC
		himax_HW_reset();
		msleep(50);
#endif
		himax_ts_poweron();

		mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
		irq_enable = true;
	}

	return count;
}

static DEVICE_ATTR(reset, (S_IWUSR|S_IRUGO), NULL, himax_reset_set);
#endif

static int himax_touch_sysfs_init(void)
{
	int ret;
	android_touch_kobj = kobject_create_and_add("android_touch", NULL);
	if (android_touch_kobj == NULL)
	{
		TPD_DMESG(KERN_ERR "[Himax]: subsystem_register failed\n");
		ret = -ENOMEM;
		return ret;
	}

	
	#ifdef HX_TP_SYS_DEBUG_LEVEL
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_debug.attr);
	if (ret)
	{
		TPD_DMESG(KERN_ERR "[Himax]: create_file debug failed\n");
		return ret;
	}
	#endif
	

	
	#ifdef HX_TP_SYS_REGISTER
	register_command = 0;
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_register.attr);
	if (ret)
	{
		TPD_DMESG(KERN_ERR "[Himax]: create_file register failed\n");
		return ret;
	}
	#endif
	

	
	#ifdef HX_TP_SYS_DIAG
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_diag.attr);
	if (ret)
	{
		TPD_DMESG(KERN_ERR "[Himax]: sysfs_create_file failed\n");
		return ret;
	}
	#endif
	

	
	#ifdef HX_TP_SYS_FLASH_DUMP
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_flash_dump.attr);
	if (ret)
	{
		E("sysfs_create_file failed\n");
		return ret;
	}
	#endif
	

	
	#ifdef HX_TP_SYS_SELF_TEST
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_tp_self_test.attr);
	if (ret)
	{
		E("[Himax]TOUCH_ERR: sysfs_create_file dev_attr_tp_self_test failed\n");
		return ret;
	}
	#endif
	

	
	#ifdef HX_TP_SYS_VENDOR
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_vendor.attr);
	if (ret)
	{
		E("sysfs_create_file vendor failed\n");
		return ret;
	}
	#endif
	

	
	#ifdef HX_TP_SYS_ATTN
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_attn.attr);
	if (ret) {
		E("sysfs_create_file attn failed\n");
		return ret;
	}
	#endif
	

	
	#ifdef HX_TP_SYS_INT
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_enabled.attr);
	if (ret)
	{
		E("sysfs_create_file enables failed\n");
		return ret;
	}
	#endif
	

	
	#ifdef HX_TP_SYS_RESET
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_reset.attr);
	if (ret)
	{
		E("sysfs_create_file reset failed\n");
		return ret;
	}
	#endif
	

	return 0 ;
}

static void himax_touch_sysfs_deinit(void)
{
	
	#ifdef HX_TP_SYS_DEBUG_LEVEL
	sysfs_remove_file(android_touch_kobj, &dev_attr_debug.attr);
	#endif
	

	
	#ifdef HX_TP_SYS_DIAG
	sysfs_remove_file(android_touch_kobj, &dev_attr_diag.attr);
	#endif
	

	
	#ifdef HX_TP_SYS_REGISTER
	sysfs_remove_file(android_touch_kobj, &dev_attr_register.attr);
	#endif
	

	
	#ifdef HX_TP_SYS_SELF_TEST
	sysfs_remove_file(android_touch_kobj, &dev_attr_tp_self_test.attr);
	#endif
	

	
	#ifdef HX_TP_SYS_VENDOR
	sysfs_remove_file(android_touch_kobj, &dev_attr_vendor.attr);
	#endif
	

	
	#ifdef HX_TP_SYS_ATTN
	sysfs_remove_file(android_touch_kobj, &dev_attr_attn.attr);
	#endif
	

	
	#ifdef HX_TP_SYS_INT
	sysfs_remove_file(android_touch_kobj, &dev_attr_enabled.attr);
	#endif
	

	
	#ifdef HX_TP_SYS_RESET
	sysfs_remove_file(android_touch_kobj, &dev_attr_reset.attr);
	#endif
	

	kobject_del(android_touch_kobj);
}


#ifdef CONFIG_TOUCHSCREEN_TOUCH_FW_UPDATE
static struct ts_fwu {
	const struct firmware *fw;
	u8 *fw_data_start;
	uint32_t fw_size;
	atomic_t in_flash;
};
static struct wake_lock flash_wake_lock;
#define FLASH_RETRY_TIMES 3

int himax_touch_fw_update(struct firmware *fw)
{
	struct ts_fwu *ts_fwu;
	uint8_t namebuf[32];
	int i = 0, tagLen = 0, ret = 0;
	char tag[40];
	char version[10] = { 0 };
	int FileLength;
	int FirmwareUpdate_Count=0;

	ts_fwu = kzalloc(sizeof(struct ts_fwu), GFP_KERNEL);
	if (ts_fwu == NULL) {
		E("%s: allocate ts_fwu failed\n", __func__);
		ret = -ENOMEM;
		goto ERR_ALLOC_DATA_FAILED;
	}

	ts_fwu->fw = fw;
	
	if (ts_fwu->fw == NULL) {
		I("[FW] No firmware file, no firmware update\n");
		ret = -1;
		goto HMX_FW_REQUEST_FAILURE;
	}

	switch (hx8526_ctp_module_name)
	{
		case 0: snprintf(namebuf, sizeof(namebuf), "TRULY");
				break;
		case 1: snprintf(namebuf, sizeof(namebuf), "YANGHUA");
				break;
		case 2: snprintf(namebuf, sizeof(namebuf), "SHENYUE");
				break;
		default:snprintf(namebuf, sizeof(namebuf), "UNKNOW");
				break;
	}

	
	snprintf(version, 10, "%02X", g_fw_verion);
	I("[FW] version=%s\n", version);
	if (fw->data[0] == 'T' && fw->data[1] == 'P') {
		while ((tag[i] = fw->data[i]) != '\n')
			i++;
		tag[i] = '\0';
		tagLen = i+1;
		I("[FW] tag=%s\n", tag);
		if (strstr(tag, namebuf) == NULL) {
			I("[FW] Update Bypass,unmatch_sensor=%s,this sensor is %s\n", tag, namebuf);
			ret = -1;
			goto HMX_FW_UNMATCH_SENSOR_FAIL;
		}
		if (strstr(tag, version) != NULL) {
			I("[FW] Update Bypass, fw_ver=%s\n", version);
			goto HMX_FW_SAME_VER_IGNORE;
		}
		I("[FW] Need Update\n");
	} else
		I("[FW] No tag, Force Update\n");
	ts_fwu->fw_data_start = (u8 *)(fw->data+tagLen);
	ts_fwu->fw_size = fw->size-tagLen;

	
	himax_HW_reset();

	atomic_set(&ts_fwu->in_flash, 1);
	mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
	irq_enable = false;

	wake_lock_init(&flash_wake_lock, WAKE_LOCK_SUSPEND, HIMAX8528_NAME);
	wake_lock(&flash_wake_lock);
	if (wake_lock_active(&flash_wake_lock))
		I("[FW] wake lock successfully acquired!\n");
	else {
		E("[FW] failed to hold wake lock, give up.....\n");
		ret = -1;
		goto WAKE_LOCK_ACQUIRE_FAILED;
	}

	FileLength = ts_fwu->fw_size;
doFirmwareUpdate_Retry:
	if (fts_ctpm_fw_upgrade_with_sys_fs(ts_fwu->fw_data_start, FileLength) == 0) {
		if(FirmwareUpdate_Count < FLASH_RETRY_TIMES) {
			FirmwareUpdate_Count++;
			I("%s: TP upgrade Error, Count: %d\n", __func__, FirmwareUpdate_Count);
			goto doFirmwareUpdate_Retry;
		}
		E("%s: TP upgrade error, line: %d\n", __func__, __LINE__);
		ret = -1;
	}
	else {
		I("%s: TP upgrade OK, line: %d\n", __func__, __LINE__);
	}

	himax_HW_reset();

	wake_unlock(&flash_wake_lock);
	wake_lock_destroy(&flash_wake_lock);
	atomic_set(&ts_fwu->in_flash, 0);
	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	irq_enable = true;

	I("[FW]firmware flash process complete!\n");

	return ret;

WAKE_LOCK_ACQUIRE_FAILED:
	wake_lock_destroy(&flash_wake_lock);
	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	irq_enable = true;
	atomic_set(&ts_fwu->in_flash, 0);
HMX_FW_REQUEST_FAILURE:
HMX_FW_UNMATCH_SENSOR_FAIL:
ERR_ALLOC_DATA_FAILED:
	return ret;
HMX_FW_SAME_VER_IGNORE:
	return 1;
}

int register_himax_touch_fw_update(void)
{
	himax_tp_notifier.fwupdate = himax_touch_fw_update;
	himax_tp_notifier.flash_timeout = FW_FLASH_TIMEOUT;
	snprintf(himax_tp_notifier.fw_vendor, sizeof(himax_tp_notifier.fw_vendor), "%s", TOUCH_VENDOR);
	snprintf(himax_tp_notifier.fw_ver, sizeof(himax_tp_notifier.fw_ver), "0x%02X", g_fw_verion);
	return register_fw_update(&himax_tp_notifier);
}
#endif

int himax_charge_switch(s32 dir_update)
{
    u32 chr_status = 0;
	uint8_t charge_buf[2] = {0};
    u8 chr_cmd[3] = {0x80, 0x40};
    static u8 chr_pluggedin = 0;
    int ret = 0;
    chr_status = upmu_is_chr_det();


	if (chr_status)
	{
		if (!chr_pluggedin || dir_update)
       		 {

			I("himax >>>work_incharging\n");
			charge_buf[0] = 0x01;

			himax_i2c_write_data(i2c_client, 0x90, 1, &(charge_buf[0]));
			chr_pluggedin = 1;

			#if 1
			int charge_flag = 0;
			himax_i2c_read_data(i2c_client,0x90,1,&charge_flag);
			I("himax >> charge_flag = %d .\n",charge_flag);
			#endif
		}
	}
    else
    {
		if (chr_pluggedin || dir_update)
        		{
			charge_buf[0] = 0x00;
			himax_i2c_write_data(i2c_client, 0x90, 1, &(charge_buf[0]));
			I("himax >>>work_outcharging\n");
			chr_pluggedin = 0;
			#if 1
			int charge_flag = 0;
			himax_i2c_read_data(i2c_client,0x90,1,&charge_flag);
			I("himax >> charge_flag = %d .\n",charge_flag);
			#endif
		}
	}

	return 0;
}




static  void tpd_down(int x, int y, int p)
{
    input_report_key(tpd->dev, BTN_TOUCH, 1);
    
    input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
    input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);

    
    input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, p);
    input_mt_sync(tpd->dev);
    if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
    {
        tpd_button(x, y, 1);
    }
}

static  int tpd_up(int x, int y, int *count)
{
#ifdef HX_PORTING_DEB_MSG
    TPD_DMESG("[Himax] tpd_up[%4d %4d]\n ", x, y);
#endif

    input_report_key(tpd->dev, BTN_TOUCH, 0);
    input_mt_sync(tpd->dev);
    if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
    {
        tpd_button(x, y, 0);
    }
}


static int tpd_touchinfo(struct touch_info *cinfo, struct touch_info *pinfo)
{
	int i = 0,temp1, temp2,ret = 0,res=0;
	char data[128] = {0};
	char gesture_flag = '0';
	u8 check_sum_cal = 0;
	u16 high_byte,low_byte;
	int err[4] = {0};
	unsigned int x=0, y=0, area=0, press=0;
	const unsigned int x_res = HX_X_RES;
	const unsigned int y_res = HX_Y_RES;
	unsigned int temp_x[HX_MAX_PT], temp_y[HX_MAX_PT];

	int check_FC = 0;

#ifdef TPD_PROXIMITY
	int err_1;
	hwm_sensor_data sensor_data;
	u8 proximity_status;
#endif

#ifdef BUTTON_CHECK
	int button_cancel=0;
	
	int cur_frm_max  = 0;
#endif

#ifdef HX_TP_SYS_DIAG
	uint8_t *mutual_data;
	uint8_t *self_data;
	uint8_t diag_cmd;
	int mul_num;
	int self_num;
	int index = 0;

	
	char coordinate_char[15+(HX_MAX_PT+5)*2*5+2];
	struct timeval t;
	struct tm broken;
	
#endif

	

	int read_len;
	int raw_cnt_max = HX_MAX_PT/4;
	int raw_cnt_rmd = HX_MAX_PT%4;
	int hx_touch_info_size, RawDataLen;
	if(raw_cnt_rmd != 0x00)
	{
		if (IC_TYPE == HX_85XX_D_SERIES_PWON)
		{
			RawDataLen = 128 - ((HX_MAX_PT+raw_cnt_max+3)*4) - 1;
	 	}
		else
		{
			RawDataLen = 128 - ((HX_MAX_PT+raw_cnt_max+3)*4);
		}
		hx_touch_info_size = (HX_MAX_PT+raw_cnt_max+2)*4;
	}
	else
	{
		if (IC_TYPE == HX_85XX_D_SERIES_PWON)
		{
			RawDataLen = 128 - ((HX_MAX_PT+raw_cnt_max+2)*4) - 1;
		}
		else
		{
			RawDataLen = 128 - ((HX_MAX_PT+raw_cnt_max+2)*4);
		}
		hx_touch_info_size = (HX_MAX_PT+raw_cnt_max+1)*4;
	}

#ifdef TPD_PROXIMITY
	point_proximity_position=hx_touch_info_size-2;
#endif

#ifdef ENABLE_CHIP_STATUS_MONITOR
	running_status = 1;
	cancel_delayed_work_sync(&himax_chip_monitor);
#endif

#ifdef HX_TP_SYS_DIAG
	if(diag_command) 
#else
		if(false)
#endif
		{
			read_len = 128; 
		}
		else
		{
			read_len =  hx_touch_info_size;
		}

	
	mutex_lock(&i2c_access);
	
	if (tpd_halt) 
	{
		
		if(HX_Gesture==1)
		{
#ifdef HX_PORTING_DEB_MSG
			TPD_DMESG("[Himax] Himax TP: tpd halt but in gesture mode.\n");
#endif
		}
		else
		{
			
#ifdef HX_PORTING_DEB_MSG
			TPD_DMESG("[Himax] Himax TP: tpd_touchinfo return ..\n");
#endif
			mutex_unlock(&i2c_access);
			return false;
		}
	}

#ifdef HX_PORTING_DEB_MSG
	I("[Himax] %s: read_len = %d \n",__func__,read_len);
#endif

	if(HX_Gesture==1)
	{
		mdelay(50);
	}
#ifdef HX_MTK_DMA
	if(himax_i2c_read_data(i2c_client, 0x86, read_len, &(data[0])) < 0 )
	{
		
		memset(data, 0xff , 128);

		mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
		irq_enable = true;
		goto work_func_send_i2c_msg_fail;
	}

#ifdef HX_PORTING_DEB_MSG
	I("[Himax] %s: ",__func__);
	for ( i = 0; i < read_len; i++)
		I("data[%d] = 0x%x, ",i,data[i]);
	printk("\n");
#endif
#else

	for ( i = 0; i < read_len; i = i+8)
	{
		himax_i2c_read_data(i2c_client, 0x86, 8, &(data[i]));
	}
#endif
	mutex_unlock(&i2c_access);

	#ifdef HX_ESD_WORKAROUND
	for(i = 0; i < hx_touch_info_size; i++)
	{
		if(data[i] == 0x00)
		{
			check_sum_cal = 1;
		}
		else if(data[i] == 0xED)
		{
			check_sum_cal = 2;
		}
		else
		{
			check_sum_cal = 0;
			i = hx_touch_info_size;
		}
	}

	
	
	#ifdef HX_TP_SYS_DIAG
	diag_cmd = getDiagCommand();

		#ifdef HX_ESD_WORKAROUND
		if((check_sum_cal != 0 || TOUCH_UP_COUNTER > 10) && ESD_RESET_ACTIVATE == 0 && diag_cmd == 0)  
		#else
		if(check_sum_cal != 0 && diag_cmd == 0)
		#endif
	#else
		#ifdef HX_ESD_WORKAROUND
		if((check_sum_cal != 0 || TOUCH_UP_COUNTER >10 ) && ESD_RESET_ACTIVATE == 0 )  
		#else
		if(check_sum_cal !=0)
		#endif
	#endif
		{
			ret = himax_hang_shaking(); 
			
			
			if(ret == 2)
			{
				goto work_func_send_i2c_msg_fail;
			}

			if((ret == 1) && (check_sum_cal == 1))
			{
				I("ESD event checked - ALL Zero.\n");
				ESD_HW_REST();
			}
			else if(check_sum_cal == 2)
			{
				I("ESD event checked - ALL 0xED.\n");
				ESD_HW_REST();
			}
			if(TOUCH_UP_COUNTER > 10)
			{
				I("TOUCH UP COUNTER > 10.\n");
				ESD_HW_REST();

			}
			if(TOUCH_UP_COUNTER > 10)
				TOUCH_UP_COUNTER = 0;
#ifdef ENABLE_CHIP_STATUS_MONITOR
			running_status = 0;
			queue_delayed_work(himax_wq, &himax_chip_monitor, 3*HZ);	
#endif
			return;
		}
		else if(ESD_RESET_ACTIVATE)
		{
			ESD_RESET_ACTIVATE = 0;
			I("%s: Back from ESD reset, ready to serve.\n", __func__);
			
			
			
			
			
#ifdef ENABLE_CHIP_STATUS_MONITOR
			running_status = 0;
			queue_delayed_work(himax_wq, &himax_chip_monitor, 3*HZ);	
#endif
			return;
		}
#endif

#ifdef Himax_Gesture

#ifdef HX_PORTING_DEB_MSG
		I("GPG key begin !\n");
#endif
		for(i=0;i<read_len;i++)
		{
#ifdef HX_PORTING_DEB_MSG
			I(" GPG key data[%d] is %x\n",i, data[i]);
#endif
			if (check_FC==0)
			{
				if((data[i]==0xFC)||(data[i]==0xF9)||(data[i]==0xF8)||(data[i]==0xFB)||(data[i]==0xFA))
				{
					check_FC = 1;
					gesture_flag = data[i];
				}
				else
				{
					check_FC = 0;
					break;
				}
			}
			else
			{
				if(data[i]!=gesture_flag)
				{
					check_FC = 0;
					break;
				}
			}
		}

#ifdef HX_PORTING_DEB_MSG
		I("check_FC is 1 input_report_key on%c , gesture_flag= %c\n ",check_FC,gesture_flag );
		I("check_FC is   %d!\n", check_FC);
#endif
		if(check_FC == 1)
		{

			if ((gesture_flag==0xF8)&&(gesture_switch==1))
			{
				input_report_key(tpd->dev, KEY_F14, 1);
				
				input_sync(tpd->dev);

				input_report_key(tpd->dev, KEY_F14, 0);
				
				input_sync(tpd->dev);
				I("check_FC is KEY_F14\n");

			}
			if ((gesture_flag==0xF9)&&(gesture_switch==1))
			{
				input_report_key(tpd->dev, KEY_F13, 1);
				
				input_sync(tpd->dev);

				input_report_key(tpd->dev, KEY_F13, 0);
				
				input_sync(tpd->dev);
				I("check_FC is KEY_F13\n");
			}
			if ((gesture_flag==0xFA)&&(gesture_switch==1))
			{
				input_report_key(tpd->dev, KEY_F16, 1);
				
				input_sync(tpd->dev);

				input_report_key(tpd->dev, KEY_F16, 0);
				
				input_sync(tpd->dev);
				I("check_FC is KEY_F15\n");
			}
			if ((gesture_flag==0xFB)&&(gesture_switch==1))
			{
				input_report_key(tpd->dev, KEY_F15, 1);
				
				input_sync(tpd->kpd);

				input_report_key(tpd->dev, KEY_F15, 0);
				
				input_sync(tpd->dev);
				I("check_FC is KEY_F16\n");
			}
			if ((gesture_flag==0xFC)&&(wake_switch== 1))
			{
				input_report_key(tpd->dev, KEY_POWER, 1);
				
				input_sync(tpd->dev);

				input_report_key(tpd->dev, KEY_POWER, 0);
				
				input_sync(tpd->dev);
				I("check_FC is KEY_F17\n");
			}

#ifdef HX_PORTING_DEB_MSG
			I("check_FC is 1 input_report_key on \n " );
			I("Himax GPG key end.\n");
#endif
			return false;
		}

		else
		{
#endif  
			
			check_sum_cal = 0;
			for(i = 0; i < hx_touch_info_size; i++)
			{
				check_sum_cal += data[i];
			}

			
			if ((check_sum_cal != 0x00) || (data[HX_TOUCH_INFO_POINT_CNT] & 0xF0 )!= 0xF0)
			{
				I("checksum fail : check_sum_cal: 0x%02X\n", check_sum_cal);

				mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
				irq_enable = true;

#ifdef ENABLE_CHIP_STATUS_MONITOR
				running_status = 0;
				queue_delayed_work(himax_wq, &himax_chip_monitor, 3*HZ); 
#endif
				return;
			}

#ifdef TPD_PROXIMITY

			if (tpd_proximity_flag == 1)
			{
				I("data[point_proximity_position]************:%d\n",data[point_proximity_position]);

				for(i = 0; i < hx_touch_info_size-1; i++)
				{
					if(data[i] == 0xFF)
						check_sum_cal = 0;
					else
					{
						check_sum_cal = 1;
						i = hx_touch_info_size-1;
					}
				}


				if(((data[point_proximity_position] & 0x04) == 0) || (check_sum_cal == 0))
					tpd_proximity_detect = 1;	
				else
				{
					tpd_proximity_detect = 0;	
					TPD_PROXIMITY_DEBUG(" ps change***********************:%d\n",  tpd_proximity_detect);
				}

				
				TPD_PROXIMITY_DEBUG(" ps change:%d\n",  tpd_proximity_detect);

				
				sensor_data.values[0] = tpd_get_ps_value();
				sensor_data.value_divide = 1;
				sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;

				
				if((err_1 = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
					TPD_PROXIMITY_DMESG("call hwmsen_get_interrupt_data fail = %d\n", err_1);

				if(!tpd_proximity_detect)
					return 0;
			}
#endif  


#ifdef HX_TP_SYS_DIAG
			diag_cmd = getDiagCommand();
			if (diag_cmd >= 1 && diag_cmd <= 6)
	{
		if (IC_TYPE == HX_85XX_D_SERIES_PWON)
		{
			
			for (i = hx_touch_info_size, check_sum_cal = 0; i < 128; i++)
			{
				check_sum_cal += data[i];
			}

			if (check_sum_cal % 0x100 != 0)
			{
				goto bypass_checksum_failed_packet;
			}
		}
		if(Is_2T3R && diag_cmd == 4)
		{
			mutual_data = getMutualBuffer_2();
			self_data 	= getSelfBuffer();

			
			mul_num = getXChannel_2() * getYChannel_2();

			#ifdef HX_EN_SEL_BUTTON
			self_num = getXChannel_2() + getYChannel_2() + HX_BT_NUM;
			#else
			self_num = getXChannel_2() + getYChannel_2();
			#endif
		}
		else
		{
			mutual_data = getMutualBuffer();
			self_data   = getSelfBuffer();

			
			mul_num = getXChannel() * getYChannel();

			#ifdef HX_EN_SEL_BUTTON
			self_num = getXChannel() + getYChannel() + HX_BT_NUM;
			#else
			self_num = getXChannel() + getYChannel();
			#endif
		}
		
		if(data[hx_touch_info_size] == data[hx_touch_info_size+1] && data[hx_touch_info_size+1] == data[hx_touch_info_size+2]
		&& data[hx_touch_info_size+2] == data[hx_touch_info_size+3] && data[hx_touch_info_size] > 0)
		{
			index = (data[hx_touch_info_size] - 1) * RawDataLen;
			
			for (i = 0; i < RawDataLen; i++)
			{
				if (IC_TYPE == HX_85XX_D_SERIES_PWON)
				{
					temp1 = index + i;
				}
				else
				{
					temp1 = index;
				}

				if(temp1 < mul_num)
				{ 
					mutual_data[index + i] = data[i + hx_touch_info_size+4]; 
				}
				else
				{
					if (IC_TYPE == HX_85XX_D_SERIES_PWON)
					{
						temp1 = i + index;
						temp2 = self_num + mul_num;
					}
					else
					{
						temp1 = i;
						temp2 = self_num;
					}
					if(temp1 >= temp2)
					{
						break;
					}

					if (IC_TYPE == HX_85XX_D_SERIES_PWON)
					{
						self_data[i+index-mul_num] = data[i + hx_touch_info_size+4]; 
					}
					else
					{
						self_data[i] = data[i + hx_touch_info_size+4]; 
					}
				}
			}
		}
		else
		{
			I("%s: header format is wrong!\n", __func__);
		}
	}
	else if(diag_cmd == 7)
	{
		memcpy(&(diag_coor[0]), &data[0], 128);
	}
	
	if(coordinate_dump_enable == 1)
	{
		for(i=0; i<(15 + (HX_MAX_PT+5)*2*5); i++)
		{
			coordinate_char[i] = 0x20;
		}
		coordinate_char[15 + (HX_MAX_PT+5)*2*5] = 0xD;
		coordinate_char[15 + (HX_MAX_PT+5)*2*5 + 1] = 0xA;
	}
	
	#endif
	

	bypass_checksum_failed_packet:

	#if defined(HX_EN_SEL_BUTTON) || defined(HX_EN_MUT_BUTTON)
	tpd_key = (data[HX_TOUCH_INFO_POINT_CNT+2]>>4);

	

	if(tpd_key == 0x0F)
	{
		tpd_key = 0xFF;
	}
	
	#else
	tpd_key = 0xFF;
	#endif

	p_point_num = hx_point_num;

	if(data[HX_TOUCH_INFO_POINT_CNT] == 0xff || data[HX_TOUCH_INFO_POINT_CNT] == 0xf0)
	{
		hx_point_num = 0;
	}
	else
	{
		hx_point_num= data[HX_TOUCH_INFO_POINT_CNT] & 0x0f;
	}

#ifdef HX_PORTING_DEB_MSG
	I("Himax p_point_num = %d \n",p_point_num);
	I("Himax hx_point_num = %d \n",hx_point_num);
	I("Himax tpd_key = %d \n",tpd_key);
#endif

#ifdef BUTTON_CHECK
        cur_frm_max  = 0;
        if(tpd_key!=0xff)
            bt_cnt = 0;
#endif

#ifdef PT_NUM_LOG
       if(hx_point_num)
            point_cnt_array[curr_ptr] =100*hx_point_num;
#endif


       if(hx_point_num !=0  && tpd_key==0xFF)
       {
	       HX_KEY_HIT_FLAG=false;
	       for (i = 0; i < HX_MAX_PT; i++)
	       {
		       if (data[4*i] != 0xFF)
		       {
			       
			       x = data[4 * i + 1] | (data[4 * i] << 8) ;
			       y = data[4 * i + 3] | (data[4 * i + 2] << 8);

			       temp_x[i] = x;
			       temp_y[i] = y;

			       if((x <= x_res) && (y <= y_res))
			       {

				       press = data[4*HX_MAX_PT+i];
				       area = press;
				       if(area > 31)
				       {
					       area = (area >> 3);
				       }

				       cinfo->x[i] = x;
				       cinfo->y[i] = y;
				       cinfo->p[i] = press;
				       cinfo->id[i] = i;

#ifdef BUTTON_CHECK
				       if(y< 1280)
				       {
					       if(y > cur_frm_max )
					       {
						       pos_queue[p_latest].pos = y;
						       pos_queue[p_latest].timestamp = jiffies;
						       cur_frm_max = y;
					       }
				       }
#endif

#ifdef PT_NUM_LOG
				       if(hx_point_num)
					       point_cnt_array[curr_ptr] +=1;
#endif


#ifdef HX_PORTING_DEB_MSG
				       I("[HIMAX PORTING MSG]%s Touch DOWN x = %d, y = %d, area = %d, press = %d.\n",__func__, x, y, area, press);
#endif
#ifdef HX_TP_SYS_DIAG
				       
				       if(coordinate_dump_enable == 1)
				       {
					       do_gettimeofday(&t);
					       time_to_tm(t.tv_sec, 0, &broken);

					       sprintf(&coordinate_char[0], "%2d:%2d:%2d:%3li,", broken.tm_hour, broken.tm_min, broken.tm_sec, t.tv_usec/1000);

					       sprintf(&coordinate_char[15 + (i*2)*5], "%4d,", x);
					       sprintf(&coordinate_char[15 + (i*2)*5 + 5], "%4d,", y);

					       coordinate_fn->f_op->write(coordinate_fn,&coordinate_char[0],15 + (HX_MAX_PT+5)*2*sizeof(char)*5 + 2,&coordinate_fn->f_pos);
				       }
				       
#endif
			       }
			       else
			       {
				       cinfo->x[i] = 0xFFFF;
				       cinfo->y[i] = 0xFFFF;
				       cinfo->id[i] = i;

#ifdef HX_PORTING_DEB_MSG
				       I("[HIMAX PORTING MSG]%s Coor Error : Touch DOWN x = %d, y = %d, area = %d, press = %d.\n",__func__, x, y, area, press);
#endif
				       continue;
			       }

			       cinfo->count++;
		       }
		       else
		       {
			       cinfo->x[i] = 0xFFFF;
			       cinfo->y[i] = 0xFFFF;
		       }
	       }

#ifdef HX_ESD_WORKAROUND
	      TOUCH_UP_COUNTER = 0;
#endif
       }
       else if(hx_point_num !=0 && tpd_key !=0xFF)
       {
	       point_key_flag=true;

       }
       else if(hx_point_num==0 && tpd_key !=0xFF)
       {
#ifdef BUTTON_CHECK
	       cur_frm_max = 1;
	       if(pointFromAA())
	       {
		       if(bt_cnt < 0xffff)
			       bt_cnt+=1;

		       if(bt_cnt< bt_confirm_cnt)
		       {
			       cur_frm_max = 0;
		       }
	       }

	       if(cur_frm_max)
	       {
		       if(point_key_flag==false)
		       {
			       tpd_down(tpd_keys_dim_local[tpd_key-1][0],tpd_keys_dim_local[tpd_key-1][1], 0);
#ifdef HX_ESD_WORKAROUND
			       TOUCH_UP_COUNTER = 0;
#endif
		       }
		       HX_KEY_HIT_FLAG=true;
#ifdef HX_PORTING_DEB_MSG
		       I("Press BTN*** \r\n");
#endif
		       point_key_flag=false;
	       }
#else
	       tpd_down(tpd_keys_dim_local[tpd_key-1][0],tpd_keys_dim_local[tpd_key-1][1], 0);
	       HX_KEY_HIT_FLAG=true;
#ifdef HX_ESD_WORKAROUND
	       TOUCH_UP_COUNTER = 0;
#endif
#ifdef HX_PORTING_DEB_MSG
	       I("Press BTN*** \r\n");
#endif
#endif

       }
       else if(hx_point_num==0 && tpd_key ==0xFF)
       {


	       for(i=0;i<HX_MAX_PT;i++)
	       {
		       cinfo->x[i] = 0xFFFF;
		       cinfo->y[i] = 0xFFFF;
	       }
	       if (tpd_key_old != 0xFF)
	       {


		       tpd_up(tpd_keys_dim_local[tpd_key_old-1][0],tpd_keys_dim_local[tpd_key_old-1][1], 0);
#ifdef HX_PORTING_DEB_MSG
		       I("Himax Press BTN up*** tpd_key=%d\r\n");
#endif
		       HX_KEY_HIT_FLAG=true;

	       }
	       else
	       {
		       HX_KEY_HIT_FLAG=false;


#ifdef HX_ESD_WORKAROUND
		       TOUCH_UP_COUNTER++ ;
#endif
#ifdef HX_TP_SYS_DIAG
		       
		       if(coordinate_dump_enable == 1)
		       {
			       do_gettimeofday(&t);
			       time_to_tm(t.tv_sec, 0, &broken);

			       sprintf(&coordinate_char[0], "%2d:%2d:%2d:%lu,", broken.tm_hour, broken.tm_min, broken.tm_sec, t.tv_usec/1000);
			       sprintf(&coordinate_char[15], "Touch up!");
			       coordinate_fn->f_op->write(coordinate_fn,&coordinate_char[0],15 + (HX_MAX_PT+5)*2*sizeof(char)*5 + 2,&coordinate_fn->f_pos);
		       }
		       
#endif
	       }
       }
       tpd_key_old = tpd_key;

#ifdef BUTTON_CHECK
       p_prev = p_latest++;
       if(p_latest>= POS_QUEUE_LEN)
	       p_latest = 0;
#endif

#ifdef PT_NUM_LOG
       if(hx_point_num)
       {
	       curr_ptr+=1;
	       if(curr_ptr>=PT_ARRAY_SZ)
		       curr_ptr = 0;
       }
#endif


#ifdef ENABLE_CHIP_STATUS_MONITOR
       running_status = 0;
       queue_delayed_work(himax_wq, &himax_chip_monitor, 3*HZ);	
#endif

       TPD_DMESG("end %s, %d\n", __FUNCTION__, __LINE__);

#ifdef Himax_Gesture
		}
#endif
		return true;

work_func_send_i2c_msg_fail:

		mutex_unlock(&i2c_access);

		E("[HIMAX TP ERROR]:work_func_send_i2c_msg_fail: %d \n",__LINE__);

#ifdef ENABLE_CHIP_STATUS_MONITOR
		running_status = 0;
		queue_delayed_work(himax_wq, &himax_chip_monitor, 3*HZ);	
#endif
}



static void tpd_eint_interrupt_handler(void)
{
    if(tpd_load_status==0)
        return;
    tpd_flag = 1;
    wake_up_interruptible(&waiter);

}

static int touch_event_handler(void *unused)
{
	struct touch_info cinfo, pinfo;
	const u8 PT_LEAVE = 1;
	u8 i;
	static u8 last_hx_point_num = 0;
	struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
	sched_setscheduler(current, SCHED_RR, &param);

	do
	{
		
		set_current_state(TASK_INTERRUPTIBLE);
		wait_event_interruptible(waiter,tpd_flag!=0);

		tpd_flag = 0;

		set_current_state(TASK_RUNNING);
		himax_charge_switch(0);
		
		if(tpd_hs_enable != 0xFF)
		{
			tpd_enable_high_Sensitivity_exe(tpd_hs_enable);
			tpd_hs_enable = 0xFF;
		}
		else if  (tpd_touchinfo(&cinfo, &pinfo))
		{
#ifdef Himax_fcover
			if(fcover_close_flag == 1)
			{
#endif
				for(i = 0; i < HX_MAX_PT; i++)
				{
					if (cinfo.x[i] != 0xFFFF)
					{
						if(HX_KEY_HIT_FLAG == false)

							tpd_down(cinfo.x[i], cinfo.y[i], cinfo.id[i]);

#ifdef HX_PORTING_DEB_MSG
						if(unlikely(touchdown_flag))
						{
							I("Coordinate cinfo.x[i]=%d,cinfo.y[i]=%d i=%d \n",cinfo.x[i],cinfo.y[i],i);
						}
#endif
					}
				}
				if (hx_point_num == 0&&last_hx_point_num !=0)
				{
					
					tpd_up(cinfo.x[0], cinfo.y[0], i + 1);
				}
				last_hx_point_num=hx_point_num;
				
				input_sync(tpd->dev);
#ifdef Himax_fcover
			}
			else
			{
				for(i = 0; i < HX_MAX_PT; i++)
				{
					if (cinfo.x[i] != 0xFFFF)
					{
						
						if(HX_KEY_HIT_FLAG == false)
						{
							if(((cinfo.x[i]> fcover_x_min)&&(cinfo.x[i]< fcover_x_max))&&((cinfo.y[i]> fcover_y_min)&&(cinfo.y[i]< fcover_y_max)))

							{
								tpd_down(cinfo.x[i], cinfo.y[i], cinfo.id[i]);
							}
						}
					}
				}
				if (hx_point_num == 0&&last_hx_point_num !=0)
				{
					
					tpd_up(cinfo.x[0], cinfo.y[0], i + 1);
				}
				last_hx_point_num=hx_point_num;
				
				input_sync(tpd->dev);

			}
#endif


		}
	}
	while(!kthread_should_stop());

	return 0;
}


#ifdef Android4_0
	static const struct i2c_device_id tpd_i2c_id[] = {{"mtk-tpd",0},{}};
	static struct i2c_board_info __initdata himax_i2c_tpd={ I2C_BOARD_INFO("mtk-tpd", (0x90>>1))};

	static struct i2c_driver tpd_i2c_driver =
	{
		.driver =
		{
			.name = "mtk-tpd",
		},
		.probe 	      = tpd_probe,
		.remove       = tpd_remove,

		.id_table     = tpd_i2c_id,
		.detect       = tpd_detect,
	};
#else
	static const struct i2c_device_id tpd_id[] = {{TPD_DEVICE,0},{}};
	static  unsigned short force[] = {0,0x90,I2C_CLIENT_END,I2C_CLIENT_END};
	static const unsigned short * const forces[] = { force, NULL };
	static struct i2c_client_address_data addr_data = { .forces = forces, };

	static struct i2c_driver tpd_i2c_driver =
	{
		.driver =
		{
			.name  = TPD_DEVICE,
			.owner = THIS_MODULE,
		},
		.probe         = tpd_probe,
		.remove        = tpd_remove,
		.id_table      = tpd_id,
		.detect        = tpd_detect,
		.address_data  = &addr_data,
	};
#endif

static int tpd_detect (struct i2c_client *client, int kind, struct i2c_board_info *info)
{
	strcpy(info->type, TPD_DEVICE);
	return 0;
}

#if defined(COMPARE_CTP_MODULE)
static char himax_get_tp_id(void)
{
	char data[12] = {0};
	char valuebuf[2]={0};
	himax_ts_poweron();

	if( himax_i2c_write_data(i2c_client, 0x82, 0, &(data[0])) < 0)
	{
		E("%s: i2c access fail!\n", __func__);
		return -1;
	}
	mdelay(50);

	data[0] = 0x15;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0)
	{
		E("%s: i2c access fail!\n", __func__);
		return -1;
	}
	msleep(10);

	data[0] = 0x00;
	data[1] = 0x01; 
	if( himax_i2c_write_data(i2c_client, 0xD8, 2, &(data[0])) < 0)
	{
		E("%s: i2c access fail!\n", __func__);
		return -1;
	}
	msleep(10);

	if( himax_i2c_read_data(i2c_client, 0x5A, 2, &(valuebuf[0])) < 0)
	{
		E("%s: i2c access fail!\n", __func__);
		return -1;
	}
	msleep(10);

	data[0] = 0x00;
	if( himax_i2c_write_data(i2c_client, 0xE1, 1, &(data[0])) < 0)
	{
		E("%s: i2c access fail!\n", __func__);
		return -1;
	}
	msleep(10);

	I("[Himax]:0xFE01_0 = 0x%x\n",valuebuf[0]);
	I("[Himax]:0xFE01_1 = 0x%x\n",valuebuf[1]);

	if (0x21 == valuebuf[1])	
	{
		hx8526_ctp_module_name = HX8526_CTP_TRULY;
		I("himax, tpd_probe, hx8526_ctp_module_name = HX8526_CTP_TRULY\n");
	}
	else if(0x22 == valuebuf[1])
	{
		hx8526_ctp_module_name = HX8526_CTP_YANGHUA;
		I("himax, tpd_probe, hx8526_ctp_module_name = HX8526_CTP_YANGHUA\n");
	}
	else if(0x12 == valuebuf[1])
	{
		hx8526_ctp_module_name = HX8526_CTP_SHENYUE;
		I("himax, tpd_probe, hx8526_ctp_module_name = HX8526_CTP_SHENYUE\n");
	}
	else
	{
		hx8526_ctp_module_name = HX8526_CTP_TRULY;
		I("himax, tpd_probe, default hx8526_ctp_module_name = HX8526_CTP_TRULY\n");
	}
	return 0;
}
#endif

u8 isTP_Updated = 0;
static int  tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int retval = 0;
	char data[5];
	int fw_ret;
	int k;
#ifdef TPD_PROXIMITY
	struct hwmsen_object obj_ps;
	int err;
#endif

#ifdef BUTTON_CHECK
	
	for(k = 0 ; k < POS_QUEUE_LEN ; k++)
	{
		pos_queue[k].pos = INVALID_POS;
		pos_queue[k].timestamp = 0;
	}
	p_latest = 1;
	p_prev = 0 ;
	bt_cnt = 0;
#endif

#ifdef PT_NUM_LOG
	for(k = 0 ; k < PT_ARRAY_SZ ; k++)
		point_cnt_array[k] = 0;

	curr_ptr = 0;
#endif

	client->addr |= I2C_ENEXT_FLAG;
	i2c_client = client;
	i2c_client->timing = 400;

#if 0
	mt_set_gpio_mode(GPIO_CTP_EN_PIN, GPIO_CTP_EN_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_EN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ONE);
	mdelay(100);
#endif

	
	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);

	
	mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
	mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
	mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_DISABLE);
	

	
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
	msleep(100);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
	msleep(100);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
	msleep(100);

	
#ifdef HX_MTK_DMA
	gpDMABuf_va = (u8 *)dma_alloc_coherent(&client->dev, 4096, &gpDMABuf_pa, GFP_KERNEL);
	if(!gpDMABuf_va)
	{
		I("[Himax] Allocate DMA I2C Buffer failed\n");
	}
#endif

	
	if (!himax_ic_package_check())
	{
		I("[HIMAX TP ERROR] %s: himax_ic_package_check failed\n", __func__);
		return -1;
	}

#if defined(COMPARE_CTP_MODULE)
	himax_get_tp_id();
#endif

#ifdef Himax_Gesture
	input_set_capability(tpd->dev, EV_KEY, KEY_POWER);
	input_set_capability(tpd->dev, EV_KEY, KEY_F13);
	input_set_capability(tpd->dev, EV_KEY, KEY_F14);
	input_set_capability(tpd->dev, EV_KEY, KEY_F15);
	input_set_capability(tpd->dev, EV_KEY, KEY_F16);
	input_set_capability(tpd->dev, EV_KEY, KEY_F17);
#endif

	mt_eint_set_sens(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_TYPE);
	mt_eint_set_hw_debounce(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN);
	mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINTF_TRIGGER_FALLING, tpd_eint_interrupt_handler, 1);

	msleep(100);

	himax_touch_sysfs_init();

#ifdef TPD_PROXIMITY
	hwmsen_detach(ID_PROXIMITY);
	
	obj_ps.polling = 0;
	obj_ps.sensor_operate = tpd_ps_operate;
	if((err = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
		TPD_PROXIMITY_DMESG("attach fail = %d\n", err);
	else
		TPD_PROXIMITY_DMESG("attach OK = %d\n", err);
#endif

#ifdef  HX_TP_SYS_FLASH_DUMP
	flash_wq = create_singlethread_workqueue("himax_flash_wq");
	if (!flash_wq)
	{
		E("[HIMAX TP ERROR] %s: create flash workqueue failed\n", __func__);
		
		
	}

	INIT_WORK(&flash_work, himax_ts_flash_work_func);

	setSysOperation(0);
	setFlashBuffer();
#endif
	himax_wq = create_singlethread_workqueue("himax_wq");
	if (!himax_wq)
	{
		E("[HIMAX TP ERROR] %s: create workqueue failed\n", __func__);
	}

#ifdef ENABLE_CHIP_STATUS_MONITOR
	INIT_DELAYED_WORK(&himax_chip_monitor, himax_chip_monitor_function); 
	running_status = 0;
#endif

#ifdef HX_FW_UPDATE_BY_I_FILE
#ifdef HX_FLASH_TEST
		I("HIMAX********************Enter CTP Auto Upgrade********************\n");
		i_update_func();
#else

	if (isTP_Updated == 0)
	{
		E("Himax TP: probe, isTP_Updated = 0 :%d\n", __LINE__);
		fw_ret = Check_FW_Version();
		if ((fw_ret == 1)|| (fw_ret == 0 && himax_read_FW_checksum() == 0))
		{
			if (fts_ctpm_fw_upgrade_with_i_file() <= 0)
			{
				isTP_Updated = 0;
				E("Himax TP: Upgrade Error, line:%d\n", __LINE__);
			}
			else
			{
				isTP_Updated = 1;
				E("Himax TP: Upgrade OK, line:%d\n", __LINE__);
			}
			msleep(10);
		}
	}
#endif
#endif

#if 1	
	himax_HW_reset();
	Hx_Read_FW_Version();
	himax_HW_reset();
#endif

#if LCT_ADD_TP_VERSION
	if(proc_create(CTP_PROC_FILE, 0444, NULL, &g_ctp_proc)== NULL)
	{
		I("create_proc_entry Himax_8527 failed\n");
	}
#endif

	himax_touch_information();

#ifdef HX_RST_PIN_FUNC
	himax_HW_reset();
#endif
	calculate_point_number();
	msleep(10);
	setXChannel(HX_RX_NUM); 
	setYChannel(HX_TX_NUM); 
	if(Is_2T3R)
	{
		setXChannel_2(HX_RX_NUM_2); 
		setYChannel_2(HX_TX_NUM_2); 
	}
#ifdef HX_TP_SYS_DIAG
	setMutualBuffer();
	if (getMutualBuffer() == NULL)
	{
		E("[HIMAX TP ERROR] %s: mutual buffer allocate fail failed\n", __func__);
		return -1;
	}
	if(Is_2T3R)
	{
		setMutualBuffer_2();

		if (getMutualBuffer_2() == NULL)
		{
			E("[HIMAX TP ERROR] %s: mutual buffer allocate fail failed\n", __func__);
			return -1;
		}
	}
#endif

	Check_FW_Version();

	himax_ts_poweron();
	I("[Himax] %s End power on \n",__func__);

	tpd_halt = 0;
	touch_thread = kthread_run(touch_event_handler, 0, TPD_DEVICE);
	if (IS_ERR(touch_thread))
	{
		retval = PTR_ERR(touch_thread);
		TPD_DMESG(TPD_DEVICE "[Himax] Himax TP: Failed to create kernel thread: %d\n", retval);
	}

#ifdef ENABLE_CHIP_STATUS_MONITOR
	queue_delayed_work(himax_wq, &himax_chip_monitor, 60*HZ);   
#endif

#ifdef CONFIG_TOUCHSCREEN_TOUCH_FW_UPDATE
	register_himax_touch_fw_update();
#endif

	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	irq_enable = true;

	TPD_DMESG("[Himax] Himax TP: Touch Panel Device Probe %s\n", (retval < 0) ? "FAIL" : "PASS");

#ifdef SLT_DEVINFO_CTP
	switch(hx8526_ctp_module_name)
	{
		case 0:
			devinfo_ctp_regchar("Truly", "Truly", CFG_VER_buff, DEVINFO_USED);
		break;

		case 1:
			devinfo_ctp_regchar("Yanghua", "Yanghua", CFG_VER_buff, DEVINFO_USED);
		break;

		case 2:
			devinfo_ctp_regchar("Shenyue", "Shenyue", CFG_VER_buff, DEVINFO_USED);
		break;

		default:
			devinfo_ctp_regchar("unknown", "unknown", "unknown", DEVINFO_USED);
		break;
	}
#endif
#if 0 
	unsigned char eint_bit;
	eint_bit = mt_get_gpio_in(GPIO_MHALL_EINT_PIN);
        if (eint_bit == 0)
        {
           tpd_enable_high_Sensitivity(1);
        }
#endif
	return 0;

HimaxErr:

#ifdef ENABLE_CHIP_STATUS_MONITOR
	cancel_delayed_work(&himax_chip_monitor);
#endif
	TPD_DMESG("[Himax] Himax TP: I2C transfer error, line: %d\n", __LINE__);
	return -1;
}

static int tpd_remove(struct i2c_client *client)
{
#ifdef CONFIG_TOUCHSCREEN_TOUCH_FW_UPDATE
	unregister_fw_update();
#endif

	himax_touch_sysfs_deinit();

	#ifdef HX_MTK_DMA
	if(gpDMABuf_va)
	{
		dma_free_coherent(&client->dev, 4096, gpDMABuf_va, gpDMABuf_pa);
		gpDMABuf_va = NULL;
		gpDMABuf_pa = NULL;
	}
	#endif

	TPD_DMESG("[Himax] Himax TP: TPD removed\n");
	return 0;
}



static int tpd_local_init(void)
{
	TPD_DMESG("[Himax] HIMAX_TS I2C Touchscreen Driver (Built %s @ %s)\n", __DATE__, __TIME__);

	if(i2c_add_driver(&tpd_i2c_driver)!=0)
	{
		TPD_DMESG("[Himax] unable to add i2c driver.\n");
		return -1;
	}

	#if defined(HX_EN_SEL_BUTTON) || defined(HX_EN_MUT_BUTTON)
	tpd_button_setting(TPD_KEY_COUNT, tpd_keys_local, tpd_keys_dim_local);
	#endif
	I("ningyd : tpd_halt = %d in local_init\n",tpd_halt);
	TPD_DMESG("[Himax] end %s, %d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int tpd_resume(struct i2c_client *client)
{
	int retval = 0;
	int ret = 0;
  	char data[2];
#ifndef Himax_fcover
	himax_charge_switch(1);
#endif
	#ifdef Himax_Gesture
	if ((wake_switch== 1)||(gesture_switch==1))
	{
	    I("[Himax]%s enter,do nothing\n",__func__);
	    I("[Himax]%s enter , write 0x90:0x00 \n",__func__);

	    data[0] = 0x00;
            himax_i2c_write_data(i2c_client, 0x90, 1, &data[0]);
	    
	    mdelay(10);
            himax_i2c_write_data(i2c_client, 0x82, 0, &data[0]);
	    mdelay(50);
            himax_i2c_write_data(i2c_client, 0x83, 0, &data[0]);
	    mdelay(120);
	    
	    HX_Gesture=0;
        	tpd_halt = 0;
	}
    else
	#endif
	{
	    #ifdef HX_CLOSE_POWER_IN_SLEEP
	    hwPowerOn(MT65XX_POWER_LDO_VGP4, VOL_2800, "TP");
	    hwPowerOn(MT65XX_POWER_LDO_VGP5, VOL_1800, "TP_EINT");
	    I("Himax rst in resume should not show up.\n ");
#ifdef HX_RST_PIN_FUNC
	himax_HW_reset();
#endif
	    #endif
	    mutex_lock(&i2c_access);
	    data[0] =0x00;
	    if( himax_i2c_write_data(i2c_client, 0xD7, 1, &(data[0])) < 0 )
	    {
			I("[Himax] tpd_resume send comand D7 failed\n");
			mutex_unlock(&i2c_access);
	        return -1;
	    }
	    msleep(1);

	    if( himax_ts_poweron() < 0)
	    {
			I("[Himax] tpd_resumehimax_ts_poweron failed\n");
			mutex_unlock(&i2c_access);
			return -1;
	    }

	    I("Himax suspend cmd in resume should not show up.\n ");

	    mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
		irq_enable = true;
	    tpd_halt = 0;
	#ifdef ENABLE_CHIP_STATUS_MONITOR
		suspend_state = 0;
	#endif
	mutex_unlock(&i2c_access);
	}
#ifdef Himax_fcover
		I("[Himax] %s in, sensitivity_state = %d\n",__func__, sensitivity_state);
		int state = 0;
		int state_compare  = 0;
		int i = 0;

		mutex_lock(&i2c_access);
		if(1 == sensitivity_state)
		{
			state = 0x04;

		}else if(0 == sensitivity_state)
		{
			state = 0x00;
		}

		if ((ret = himax_i2c_write_data(i2c_client, 0x92, 1, &state)) < 0)
			TPD_DEBUG("[Himax] %s i2c write error: %d\n", __func__, ret);
		msleep(2);

		if ((ret = himax_i2c_write_data(i2c_client, 0x92, 1, &state)) < 0)
			TPD_DEBUG("[Himax] %s i2c write error: %d\n", __func__, ret);
		msleep(2);
		mutex_unlock(&i2c_access);
		msleep(2);
#endif

	return retval;
}

static int tpd_suspend(struct i2c_client *client, pm_message_t message)
{
	int retval = 0;
	int i;
	static char data[2];

#ifdef Himax_Gesture
	if ((wake_switch== 1)||(gesture_switch==1))
	{
		tpd_halt = 1;
		mdelay(50);
		data[0] = 0x10;
    	himax_i2c_write_data(i2c_client, 0x90, 1, &data[0]);
		I("[Himax]%s enter , write 0x90:0x10 \n",__func__);
		HX_Gesture=1;
	}
    else
#endif
	{
    	#ifdef HX_TP_SYS_FLASH_DUMP
		if(getFlashDumpGoing())
		{
			I("[himax] %s: Flash dump is going, reject suspend\n",__func__);
			return 0;
		}
		#endif

		tpd_halt = 1;
	#ifdef ENABLE_CHIP_STATUS_MONITOR
		suspend_state = 1;
	#endif
		TPD_DMESG("[Himax] Himax TP: Suspend\n");
		mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
		irq_enable = false;
		mutex_lock(&i2c_access);

		#ifdef HX_CLOSE_POWER_IN_SLEEP
		mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
		I("Himax rst in suspend should not show up .\n");
		hwPowerDown(MT65XX_POWER_LDO_VGP4,  "TP");
		hwPowerDown(MT65XX_POWER_LDO_VGP5, "TP_EINT");
		#else
		
		himax_i2c_write_data(i2c_client, 0x82, 0, &(data[0]));
		msleep(120);

		
		himax_i2c_write_data(i2c_client, 0x80, 0, &(data[0]));
		msleep(120);

		
		data[0] =0x01;
		himax_i2c_write_data(i2c_client, 0xD7, 1, &(data[0]));
		msleep(100);
		I("Himax suspend cmd in suspend should not show up .\n");
		#endif

		mutex_unlock(&i2c_access);
	}
	return retval;
}

static struct tpd_driver_t tpd_device_driver =
{
	.tpd_device_name = "HIMAX_TS",
	.tpd_local_init = tpd_local_init,
	.suspend = tpd_suspend,
	.resume = tpd_resume,
	#if defined(HX_EN_SEL_BUTTON) || defined(HX_EN_MUT_BUTTON)
	.tpd_have_button = 1,
	#else
	.tpd_have_button = 0,
	#endif
};

static int __init tpd_driver_init(void)
{
	
	hwPowerOn(MT6325_POWER_LDO_VGP1, VOL_3300, "TP");
	hwPowerOn(MT6325_POWER_LDO_VGP2, VOL_1800, "TP");
	msleep(100);

	i2c_register_board_info(0, &himax_i2c_tpd, 1);
	TPD_DMESG("[Himax] MediaTek HIMAX_TS touch panel driver init\n");

	if(tpd_driver_add(&tpd_device_driver) < 0)
		TPD_DMESG("[Himax] add HIMAX_TS driver failed\n");

	return 0;
}

static void __exit tpd_driver_exit(void)
{
	TPD_DMESG("[Himax] MediaTek HIMAX_TS touch panel driver exit\n");
	tpd_driver_remove(&tpd_device_driver);
}

module_init(tpd_driver_init);
module_exit(tpd_driver_exit);


int himax_cable_status(int status)
{
    uint8_t buf0[2] = {0};

    return 0;
}
EXPORT_SYMBOL(himax_cable_status);
