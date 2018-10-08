#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"
#include <cust_gpio_usage.h>
#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <string.h>
	#include <platform/mt_i2c.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
    
	#include <mach/mt_gpio.h>
	#include <linux/i2c.h>
#endif


#define FRAME_WIDTH  (1080)
#define FRAME_HEIGHT (1920)

#define REGFLAG_DELAY             								0xFFFC
#define REGFLAG_UDELAY             								0xFFFB
#define REGFLAG_END_OF_TABLE      							    0xFFFD   

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

#ifndef BUILD_LK
static unsigned int lcm_esd_test = FALSE;      
#endif

static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)    do{lcm_util.set_gpio_mode((GPIO28 | 0x80000000),0);\
							   lcm_util.set_gpio_dir((GPIO28 | 0x80000000),GPIO_DIR_OUT);\
							   lcm_util.set_gpio_out((GPIO28 | 0x80000000),v);}\
							while(0)


#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


#define dsi_set_cmdq_V3(para_tbl,size,force_update)        lcm_util.dsi_set_cmdq_V3(para_tbl,size,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	        lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define   LCM_DSI_CMD_MODE							0

#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#define TPS_I2C_BUSNUM  1
#define I2C_ID_NAME "ktd3121"
#define TPS_ADDR 0x3E

static struct i2c_board_info __initdata ktd3121_board_info = {I2C_BOARD_INFO(I2C_ID_NAME, TPS_ADDR)};
static struct i2c_client *ktd3121_i2c_client = NULL;


static int ktd3121_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int ktd3121_remove(struct i2c_client *client);


 struct ktd3121_dev	{
	struct i2c_client	*client;

};

static const struct i2c_device_id ktd3121_id[] = {
	{ I2C_ID_NAME, 0 },
	{ }
};

static struct i2c_driver ktd3121_iic_driver = {
	.id_table	= ktd3121_id,
	.probe		= ktd3121_probe,
	.remove		= ktd3121_remove,
	
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "ktd3121",
	},
};

static int ktd3121_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	printk( "ktd3121_iic_probe\n");
	printk("TPS: info==>name=%s addr=0x%x\n",client->name,client->addr);
	ktd3121_i2c_client  = client;
	return 0;
}


static int ktd3121_remove(struct i2c_client *client)
{
  printk( "ktd3121_remove\n");
  ktd3121_i2c_client = NULL;
  i2c_unregister_device(client);
  return 0;
}

static int ktd3121_write_bytes(unsigned char addr, unsigned char value)
{
	int ret = 0;
	struct i2c_client *client = ktd3121_i2c_client;
	char write_data[2]={0};
	write_data[0]= addr;
	write_data[1] = value;
    ret=i2c_master_send(client, write_data, 2);
	if(ret<0)
	printk("ktd3121 write data fail !!\n");
	return ret ;
}
static int __init ktd3121_iic_init(void)
{

   printk( "ktd3121_iic_init\n");
   i2c_register_board_info(TPS_I2C_BUSNUM, &ktd3121_board_info, 1);
   printk( "ktd3121_iic_init2\n");
   i2c_add_driver(&ktd3121_iic_driver);
   printk( "ktd3121_iic_init success\n");
   return 0;
}

static void __exit ktd3121_iic_exit(void)
{
  printk( "ktd3121_iic_exit\n");
  i2c_del_driver(&ktd3121_iic_driver);
}


module_init(ktd3121_iic_init);
module_exit(ktd3121_iic_exit);

MODULE_AUTHOR("Xiaokuan Shi");
MODULE_DESCRIPTION("MTK ktd3121 I2C Driver");
MODULE_LICENSE("GPL");
#endif

struct LCM_setting_table {
    unsigned int cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {
	
	{0xFF, 1, {0xEE}},
	{0xFB, 1, {0x01}},
	{0x18, 1, {0x40}},
	
	{0x18, 1, {0x00}},
	
	
	{0xFF, 1, {0xEE}},
	
	{0x7C, 1, {0x31}},
	
	{0xFB, 1, {0x01}},
	
	{0xFF, 1, {0x01}},
	{0x00, 1, {0x01}},
	{0x01, 1, {0x55}},
	{0x02, 1, {0x40}},
	{0x05, 1, {0x50}},
	{0x06, 1, {0x4A}},
	{0x07, 1, {0x29}},
	{0x08, 1, {0x0C}},
	{0x0B, 1, {0x87}},
	{0x0C, 1, {0x87}},
	{0x0E, 1, {0xB0}},
	{0x0F, 1, {0xB3}},
	{0x14, 1, {0x4A}},
	{0x15, 1, {0x12}},
	{0x16, 1, {0x12}},
	{0x18, 1, {0x00}},
	{0x19, 1, {0x77}},
	{0x1A, 1, {0x55}},
	{0x1B, 1, {0x13}},
	{0x1C, 1, {0x00}},
	{0x1D, 1, {0x00}},
	{0x1E, 1, {0x00}},
	{0x1F, 1, {0x00}},
	
	{0x58, 1, {0x82}},
	{0x59, 1, {0x02}},
	{0x5A, 1, {0x02}},
	{0x5B, 1, {0x02}},
	{0x5C, 1, {0x82}},
	{0x5D, 1, {0x82}},
	{0x5E, 1, {0x02}},
	{0x5F, 1, {0x02}},
	
	{0x72, 1, {0x31}},
	
	{0xFB, 1, {0x01}},

	
	{0xFF, 1, {0x05}},
	{0x00, 1, {0x01}},
	{0x01, 1, {0x0B}},
	{0x02, 1, {0x0C}},
	{0x03, 1, {0x09}},
	{0x04, 1, {0x0A}},
	{0x05, 1, {0x00}},
	{0x06, 1, {0x0F}},
	{0x07, 1, {0x10}},
	{0x08, 1, {0x1A}},
	{0x09, 1, {0x00}},
	{0x0A, 1, {0x00}},
	{0x0B, 1, {0x00}},
	{0x0C, 1, {0x00}},
	{0x0D, 1, {0x13}},
	{0x0E, 1, {0x15}},
	{0x0F, 1, {0x17}},
	{0x10, 1, {0x01}},
	{0x11, 1, {0x0B}},
	{0x12, 1, {0x0C}},
	{0x13, 1, {0x09}},
	{0x14, 1, {0x0A}},
	{0x15, 1, {0x00}},
	{0x16, 1, {0x0F}},
	{0x17, 1, {0x10}},
	{0x18, 1, {0x1A}},
	{0x19, 1, {0x00}},
	{0x1A, 1, {0x00}},
	{0x1B, 1, {0x00}},
	{0x1C, 1, {0x00}},
	{0x1D, 1, {0x13}},
	{0x1E, 1, {0x15}},
	{0x1F, 1, {0x17}},

	
	{0x20, 1, {0x00}},
	{0x21, 1, {0x01}},
	{0x22, 1, {0x00}},
	{0x23, 1, {0x40}},
	{0x24, 1, {0x40}},
	{0x25, 1, {0x6D}},

	
	{0x29, 1, {0xD8}},
	{0x2A, 1, {0x2A}},
	{0x2B, 1, {0x00}},
	
	{0xB6, 1, {0x89}},
	{0xB7, 1, {0x14}},
	{0xB8, 1, {0x05}},
	
	{0x4B, 1, {0x04}},
	{0x4C, 1, {0x11}},
	{0x4D, 1, {0x10}},
	{0x4E, 1, {0x01}},
	{0x4F, 1, {0x01}},
	{0x50, 1, {0x10}},
	{0x51, 1, {0x00}},
	{0x52, 1, {0x00}},
	{0x53, 1, {0x08}},
	{0x54, 1, {0x01}},
	{0x55, 1, {0x6D}},
	
	{0x5B, 1, {0x44}},
	{0x5C, 1, {0x00}},
	{0x5F, 1, {0x74}},
	{0x60, 1, {0x75}},
	{0x63, 1, {0xFF}},
	{0x64, 1, {0x00}},
	{0x67, 1, {0x04}},
	{0x68, 1, {0x04}},
	{0x6C, 1, {0x00}},
	
	{0x7A, 1, {0x80}},
	{0x7B, 1, {0x91}},
	{0x7C, 1, {0xD8}},
	{0x7D, 1, {0x60}},
	{0x7F, 1, {0x15}},
	
	{0x80, 1, {0xD4}},
	{0x81, 1, {0x00}},
	{0x82, 1, {0xDF}},
	{0x83, 1, {0x00}},
	{0x84, 1, {0xE9}},
	{0x85, 1, {0x00}},
	{0x86, 1, {0xF2}},
	{0x87, 1, {0x01}},
	{0x88, 1, {0x17}},
	{0x89, 1, {0x01}},
	{0x8A, 1, {0x36}},
	{0x8B, 1, {0x01}},
	{0x8C, 1, {0x6B}},
	{0x8D, 1, {0x01}},
	{0x8E, 1, {0x98}},
	{0x8F, 1, {0x01}},
	{0x90, 1, {0xE0}},
	{0x91, 1, {0x02}},
	{0x92, 1, {0x19}},
	
	{0x93, 1, {0x02}},
	{0x94, 1, {0x1A}},
	{0x95, 1, {0x02}},
	{0x96, 1, {0x4E}},
	{0x97, 1, {0x02}},
	{0x98, 1, {0x85}},
	{0x99, 1, {0x02}},
	{0x9A, 1, {0xA9}},
	{0x9B, 1, {0x02}},
	{0x9C, 1, {0xD9}},
	{0x9D, 1, {0x02}},
	{0x9E, 1, {0xFB}},
	{0x9F, 1, {0x03}},

    {0xA0, 1, {0x28}},
	{0xA2, 1, {0x03}},
	{0xA3, 1, {0x35}},
	{0xA4, 1, {0x03}},
	{0xA5, 1, {0x44}},
	{0xA6, 1, {0x03}},
	{0xA7, 1, {0x54}},
	{0xA9, 1, {0x03}},
	{0xAA, 1, {0x67}},
	{0xAB, 1, {0x03}},
	{0xAC, 1, {0x7D}},
	{0xAD, 1, {0x03}},
	{0xAE, 1, {0x91}},
	{0xAF, 1, {0x03}},

	{0xB0, 1, {0xA7}},
	{0xB1, 1, {0x03}},
	{0xB2, 1, {0xCB}},
	{0xB3, 1, {0x00}},
	{0xB4, 1, {0x9D}},
	{0xB5, 1, {0x00}},
	{0xB6, 1, {0xA2}},
	{0xB7, 1, {0x00}},
	{0xB8, 1, {0xB0}},
	{0xB9, 1, {0x00}},
	{0xBA, 1, {0xBD}},
	{0xBB, 1, {0x00}},
	{0xBC, 1, {0xC9}},
	{0xBD, 1, {0x00}},
	{0xBE, 1, {0xD4}},
	{0xBF, 1, {0x00}},

	{0xC0, 1, {0xDF}},
	{0xC1, 1, {0x00}},
	{0xC2, 1, {0xE9}},
	{0xC3, 1, {0x00}},
	{0xC4, 1, {0xF2}},
	{0xC5, 1, {0x01}},
	{0xC6, 1, {0x17}},
	{0xC7, 1, {0x01}},
	{0xC8, 1, {0x36}},
	{0xC9, 1, {0x01}},
	{0xCA, 1, {0x6B}},
	{0xCB, 1, {0x01}},
	{0xCC, 1, {0x98}},
	{0xCD, 1, {0x01}},
	{0xCE, 1, {0xE0}},
	{0xCF, 1, {0x02}},

	{0xD0, 1, {0x19}},
	{0xD1, 1, {0x02}},
	{0xD2, 1, {0x1A}},
	{0xD3, 1, {0x02}},
	{0xD4, 1, {0x4E}},
	{0xD5, 1, {0x02}},
	{0xD6, 1, {0x85}},
	{0xD7, 1, {0x02}},
	{0xD8, 1, {0xA9}},
	{0xD9, 1, {0x02}},
	{0xDA, 1, {0xD9}},
	{0xDB, 1, {0x02}},
	{0xDC, 1, {0xFB}},
	{0xDD, 1, {0x03}},
	{0xDE, 1, {0x28}},
	{0xDF, 1, {0x03}},
	{0xE0, 1, {0x35}},
	{0xE1, 1, {0x03}},
	{0xE2, 1, {0x44}},
	{0xE3, 1, {0x03}},
	{0xE4, 1, {0x54}},
	{0xE5, 1, {0x03}},
	{0xE6, 1, {0x67}},
	{0xE7, 1, {0x03}},
	{0xE8, 1, {0x7D}},
	{0xE9, 1, {0x03}},
	{0xEA, 1, {0x91}},
	{0xEB, 1, {0x03}},
	{0xEC, 1, {0xA7}},
	{0xED, 1, {0x03}},
	{0xEE, 1, {0xCB}},
	{0xEF, 1, {0x00}},
	{0xF0, 1, {0x7D}},
	{0xF1, 1, {0x00}},
	{0xF2, 1, {0x81}},
	{0xF3, 1, {0x00}},
	{0xF4, 1, {0x94}},
	{0xF5, 1, {0x00}},
	{0xF6, 1, {0xA2}},
	{0xF7, 1, {0x00}},
	{0xF8, 1, {0xAF}},
	{0xF9, 1, {0x00}},
	{0xFA, 1, {0xBC}},
	{0xFF, 1, {0x02}},
	{0xFB, 1, {0x01}},
	{0x00, 1, {0x00}},
	{0x01, 1, {0xC8}},
	{0x02, 1, {0x00}},
	{0x03, 1, {0xD4}},
	{0x04, 1, {0x00}},
	{0x05, 1, {0xDF}},
	{0x06, 1, {0x01}},
	{0x07, 1, {0x06}},
	{0x08, 1, {0x01}},
	{0x09, 1, {0x28}},
	{0x0A, 1, {0x01}},
	{0x0B, 1, {0x61}},
	{0x0C, 1, {0x01}},
	{0x0D, 1, {0x8F}},
	{0x0E, 1, {0x01}},
	{0x0F, 1, {0xDA}},
	{0x10, 1, {0x02}},
	{0x11, 1, {0x15}},
	{0x12, 1, {0x02}},
	{0x13, 1, {0x17}},
	{0x14, 1, {0x02}},
	{0x15, 1, {0x4C}},
	{0x16, 1, {0x02}},
	{0x17, 1, {0x83}},
	{0x18, 1, {0x02}},
	{0x19, 1, {0xA7}},
	{0x1A, 1, {0x02}},
	{0x1B, 1, {0xD8}},
	{0x1C, 1, {0x02}},
	{0x1D, 1, {0xFA}},
	{0x1E, 1, {0x03}},
	{0x1F, 1, {0x27}},
	{0x20, 1, {0x03}},
	{0x21, 1, {0x35}},
	{0x22, 1, {0x03}},
	{0x23, 1, {0x44}},
	{0x24, 1, {0x03}},
	{0x25, 1, {0x55}},
	{0x26, 1, {0x03}},
	{0x27, 1, {0x67}},
	{0x28, 1, {0x03}},
	{0x29, 1, {0x7F}},
	{0x2A, 1, {0x03}},
	{0x2B, 1, {0x92}},
	{0x2D, 1, {0x03}},
	{0x2F, 1, {0xAA}},
	{0x30, 1, {0x03}},
	{0x31, 1, {0xCB}},
	{0x32, 1, {0x00}},
	{0x33, 1, {0x7D}},
	{0x34, 1, {0x00}},
	{0x35, 1, {0x81}},
	{0x36, 1, {0x00}},
	{0x37, 1, {0x94}},
	{0x38, 1, {0x00}},
	{0x39, 1, {0xA2}},
	{0x3A, 1, {0x00}},
	{0x3B, 1, {0xAF}},
	{0x3D, 1, {0x00}},
	{0x3F, 1, {0xBC}},
	{0x40, 1, {0x00}},
	{0x41, 1, {0xC8}},
	{0x42, 1, {0x00}},
	{0x43, 1, {0xD4}},
	{0x44, 1, {0x00}},
	{0x45, 1, {0xDF}},
	{0x46, 1, {0x01}},
	{0x47, 1, {0x06}},
	{0x48, 1, {0x01}},
	{0x49, 1, {0x28}},
	{0x4A, 1, {0x01}},
	{0x4B, 1, {0x61}},
	{0x4C, 1, {0x01}},
	{0x4D, 1, {0x8F}},
	{0x4E, 1, {0x01}},
	{0x4F, 1, {0xDA}},
	{0x50, 1, {0x02}},
	{0x51, 1, {0x15}},
	{0x52, 1, {0x02}},
	{0x53, 1, {0x17}},
	{0x54, 1, {0x02}},
	{0x55, 1, {0x4C}},
	{0x56, 1, {0x02}},
	{0x58, 1, {0x83}},
	{0x59, 1, {0x02}},
	{0x5A, 1, {0xA7}},
	{0x5B, 1, {0x02}},
	{0x5C, 1, {0xD8}},
	{0x5D, 1, {0x02}},
	{0x5E, 1, {0xFA}},
	{0x5F, 1, {0x03}},
	{0x60, 1, {0x27}},
	{0x61, 1, {0x03}},
	{0x62, 1, {0x35}},
	{0x63, 1, {0x03}},
	{0x64, 1, {0x44}},
	{0x65, 1, {0x03}},
	{0x66, 1, {0x55}},
	{0x67, 1, {0x03}},
	{0x68, 1, {0x67}},
	{0x69, 1, {0x03}},
	{0x6A, 1, {0x7F}},
	{0x6B, 1, {0x03}},
	{0x6C, 1, {0x92}},
	{0x6D, 1, {0x03}},
	{0x6E, 1, {0xAA}},
	{0x6F, 1, {0x03}},
	{0x70, 1, {0xCB}},
	{0x71, 1, {0x00}},
	{0x72, 1, {0x00}},
	{0x73, 1, {0x00}},
	{0x74, 1, {0x21}},
	{0x75, 1, {0x00}},
	{0x76, 1, {0x4C}},
	{0x77, 1, {0x00}},
	{0x78, 1, {0x6B}},
	{0x79, 1, {0x00}},
	{0x7A, 1, {0x85}},
	{0x7B, 1, {0x00}},
	{0x7C, 1, {0x9A}},
	{0x7D, 1, {0x00}},
	{0x7E, 1, {0xAD}},
	{0x7F, 1, {0x00}},
	{0x80, 1, {0xBE}},
	{0x81, 1, {0x00}},
	{0x82, 1, {0xCD}},
	{0x83, 1, {0x01}},
	{0x84, 1, {0x01}},
	{0x85, 1, {0x01}},
	{0x86, 1, {0x29}},
	{0x87, 1, {0x01}},
	{0x88, 1, {0x68}},
	{0x89, 1, {0x01}},
	{0x8A, 1, {0x98}},
	{0x8B, 1, {0x01}},
	{0x8C, 1, {0xE5}},
	{0x8D, 1, {0x02}},
	{0x8E, 1, {0x1E}},
	{0x8F, 1, {0x02}},
	{0x90, 1, {0x20}},
	{0x91, 1, {0x02}},
	{0x92, 1, {0x52}},
	{0x93, 1, {0x02}},
	{0x94, 1, {0x88}},
	{0x95, 1, {0x02}},
	{0x96, 1, {0xAA}},
	{0x97, 1, {0x02}},
	{0x98, 1, {0xD7}},
	{0x99, 1, {0x02}},
	{0x9A, 1, {0xF7}},
	{0x9B, 1, {0x03}},
	{0x9C, 1, {0x21}},
	{0x9D, 1, {0x03}},
	{0x9E, 1, {0x2E}},
	{0x9F, 1, {0x03}},
	{0xA0, 1, {0x3D}},
	{0xA2, 1, {0x03}},
	{0xA3, 1, {0x4C}},
	{0xA4, 1, {0x03}},
	{0xA5, 1, {0x5E}},
	{0xA6, 1, {0x03}},
	{0xA7, 1, {0x71}},
	{0xA9, 1, {0x03}},
	{0xAA, 1, {0x86}},
	{0xAB, 1, {0x03}},
	{0xAC, 1, {0x94}},
	{0xAD, 1, {0x03}},
	{0xAE, 1, {0xFA}},
	{0xAF, 1, {0x00}},
	{0xB0, 1, {0x00}},
	{0xB1, 1, {0x00}},
	{0xB2, 1, {0x21}},
	{0xB3, 1, {0x00}},
	{0xB4, 1, {0x4C}},
	{0xB5, 1, {0x00}},
	{0xB6, 1, {0x6B}},
	{0xB7, 1, {0x00}},
	{0xB8, 1, {0x85}},
	{0xB9, 1, {0x00}},
	{0xBA, 1, {0x9A}},
	{0xBB, 1, {0x00}},
	{0xBC, 1, {0xAD}},
	{0xBD, 1, {0x00}},
	{0xBE, 1, {0xBE}},
	{0xBF, 1, {0x00}},
	{0xC0, 1, {0xCD}},
	{0xC1, 1, {0x01}},
	{0xC2, 1, {0x01}},
	{0xC3, 1, {0x01}},
	{0xC4, 1, {0x29}},
	{0xC5, 1, {0x01}},
	{0xC6, 1, {0x68}},
	{0xC7, 1, {0x01}},
	{0xC8, 1, {0x98}},
	{0xC9, 1, {0x01}},
	{0xCA, 1, {0xE5}},
	{0xCB, 1, {0x02}},
	{0xCC, 1, {0x1E}},
	{0xCD, 1, {0x02}},
	{0xCE, 1, {0x20}},
	{0xCF, 1, {0x02}},
	{0xD0, 1, {0x52}},
	{0xD1, 1, {0x02}},
	{0xD2, 1, {0x88}},
	{0xD3, 1, {0x02}},
	{0xD4, 1, {0xAA}},
	{0xD5, 1, {0x02}},
	{0xD6, 1, {0xD7}},
	{0xD7, 1, {0x02}},
	{0xD8, 1, {0xF7}},
	{0xD9, 1, {0x03}},
	{0xDA, 1, {0x21}},
	{0xDB, 1, {0x03}},
	{0xDC, 1, {0x2E}},
	{0xDD, 1, {0x03}},
	{0xDE, 1, {0x3D}},
	{0xDF, 1, {0x03}},
	{0xE0, 1, {0x4C}},
	{0xE1, 1, {0x03}},
	{0xE2, 1, {0x5E}},
	{0xE3, 1, {0x03}},
	{0xE4, 1, {0x71}},
	{0xE5, 1, {0x03}},
	{0xE6, 1, {0x86}},
	{0xE7, 1, {0x03}},
	{0xE8, 1, {0x94}},
	{0xE9, 1, {0x03}},
	{0xEA, 1, {0xFA}},
	{0xFF, 1, {0x05}},
	{0xFB, 1, {0x01}},
	{0xE7, 1, {0x80}},
	{0xFB, 1, {0x01}},

	
	{0xEC, 1, {0x00}},
	
	{0xFF, 1, {0x00}},
	{0xD3, 1, {0x06}},
	{0xD4, 1, {0x04}},
	{0x35, 1, {0x00}},

	{0x11, 0, {}},
	{REGFLAG_DELAY, 140, {}},
	{0x29, 0, {}},
	{0x53, 1, {0x24}},
	{REGFLAG_END_OF_TABLE, 0x00, {}},
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;
    for(i = 0; i < count; i++)
    {
        unsigned cmd;
        cmd = table[i].cmd;
        switch (cmd) {

            case REGFLAG_DELAY :
                if(table[i].count <= 10)
                    MDELAY(table[i].count);
                else
                    MDELAY(table[i].count);
                break;

			case REGFLAG_UDELAY :
				UDELAY(table[i].count);
				break;

            case REGFLAG_END_OF_TABLE :
                break;

            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
        }
    }
}



static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
		memset(params, 0, sizeof(LCM_PARAMS));

		params->type   = LCM_TYPE_DSI;

		params->width  = FRAME_WIDTH;
		params->height = FRAME_HEIGHT;
#if defined(CONFIG_CUSTOM_KERNEL_LCM_PHY_WIDTH) && defined(CONFIG_CUSTOM_KERNEL_LCM_PHY_HEIGHT)
		params->physical_width = CONFIG_CUSTOM_KERNEL_LCM_PHY_WIDTH;
		params->physical_height = CONFIG_CUSTOM_KERNEL_LCM_PHY_HEIGHT;
#endif

#ifdef SLT_DEVINFO_LCM
		params->module="truly";
		params->vendor="truly";
		params->ic="nt35596";
		params->info="1080*1920";
		
#endif

        #if (LCM_DSI_CMD_MODE)
		params->dsi.mode   = CMD_MODE;
        #else
		params->dsi.mode   = SYNC_PULSE_VDO_MODE; 
        #endif

		
		
		
		params->dsi.LANE_NUM				= LCM_FOUR_LANE;
		
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

        
		
		params->dsi.packet_size=512;

        
		params->dsi.intermediat_buffer_num = 0;

		
		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
        params->dsi.word_count=1080*3;

		params->dsi.vertical_sync_active				= 2;
		params->dsi.vertical_backporch					= 4;
		params->dsi.vertical_frontporch					= 4;
		params->dsi.vertical_active_line				= FRAME_HEIGHT;

		params->dsi.horizontal_sync_active				= 20;
		params->dsi.horizontal_backporch				= 40;
		params->dsi.horizontal_frontporch				= 100;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	    

		
		params->dsi.PLL_CLOCK = 286;
		
		
		
		

#if defined(CONFIG_CUSTOM_KERNEL_PWM_MIN) && defined(CONFIG_CUSTOM_KERNEL_PWM_DEF) && defined(CONFIG_CUSTOM_KERNEL_PWM_MAX) && defined(CONFIG_CUSTOM_KERNEL_CAM_AP_LEVEL)
		params->pwm_min = CONFIG_CUSTOM_KERNEL_PWM_MIN;
		params->pwm_default = CONFIG_CUSTOM_KERNEL_PWM_DEF;
		params->pwm_max = CONFIG_CUSTOM_KERNEL_PWM_MAX;
		params->camera_blk = CONFIG_CUSTOM_KERNEL_CAM_AP_LEVEL;
		params->camera_dua_blk = CONFIG_CUSTOM_KERNEL_CAM_AP_LEVEL;
#endif

}

#ifdef BUILD_LK
#define KTD3121_SLAVE_ADDR_WRITE  0x36
static struct mt_i2c_t KTD3121_i2c;

static int KTD3121_write_byte(kal_uint8 addr, kal_uint8 value)
{
    kal_uint32 ret_code = I2C_OK;
    kal_uint8 write_data[2];
    kal_uint16 len;

    write_data[0]= addr;
    write_data[1] = value;

    KTD3121_i2c.id = 1;   
    
    KTD3121_i2c.addr = (KTD3121_SLAVE_ADDR_WRITE >> 1);
    KTD3121_i2c.mode = ST_MODE;
    KTD3121_i2c.speed = 100;
    len = 2;

    ret_code = i2c_write(&KTD3121_i2c, write_data, len);
    

    return ret_code;
}
#endif


#ifdef BUILD_LK
static struct mt_i2c_t hx8394d_i2c;
#define HX8394D_SLAVE_ADDR 0x3e
static kal_uint32 hx8394d_write_byte(kal_uint8 addr, kal_uint8 value)
{
    kal_uint32 ret_code = I2C_OK;
    kal_uint8 write_data[2];
    kal_uint16 len;

    write_data[0]= addr;
    write_data[1] = value;

    hx8394d_i2c.id = 1;
    
    hx8394d_i2c.addr = (HX8394D_SLAVE_ADDR);
    hx8394d_i2c.mode = ST_MODE;
    hx8394d_i2c.speed = 100;
    len = 2;

    ret_code = i2c_write(&hx8394d_i2c, write_data, len);
    

    return ret_code;
}

#endif
static void KTD2125_enable(char en)
{

	mt_set_gpio_mode(GPIO_LCM_PWR_EN, GPIO_LCD_BIAS_ENP_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_LCM_PWR_EN, GPIO_DIR_OUT);

	if (en)
	{

		mt_set_gpio_out(GPIO_LCM_PWR_EN, GPIO_OUT_ONE);
		#ifdef BUILD_LK
		hx8394d_write_byte(0x00, 0x0f);
		hx8394d_write_byte(0x01, 0x0f);
		#endif
	}
	else
	{
		mt_set_gpio_out(GPIO_LCM_PWR_EN, GPIO_OUT_ZERO);
	}

}
static void lcm_init(void)
{

    unsigned char cmd = 0x0;
	unsigned char data = 0xFF;
	int ret=0;

	mt_set_gpio_mode(GPIO_LCM_LED_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCM_LED_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCM_LED_EN, GPIO_OUT_ONE);

	
	cmd=0x06;
	data=0x00;
#ifdef BUILD_LK
	ret=KTD3121_write_byte(cmd,data);
	if(ret)
		dprintf(0, "[LK]nt35596----ktd3121----cmd=%0x--i2c write error----\n",cmd);
	else
		dprintf(0, "[LK]nt35596----ktd3121----cmd=%0x--i2c write success----\n",cmd);
#else
	ret=ktd3121_write_bytes(cmd,data);
	if(ret<0)
		printk("[KERNEL]nt35596----ktd3121---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printk("[KERNEL]nt35596----ktd3121---cmd=%0x-- i2c write success-----\n",cmd);
#endif


	cmd=0x02;
	data=0x6D;
#ifdef BUILD_LK
	ret=KTD3121_write_byte(cmd,data);
    if(ret)
    dprintf(0, "[LK]nt35596----ktd3121----cmd=%0x--i2c write error----\n",cmd);
	else
	dprintf(0, "[LK]nt35596----ktd3121----cmd=%0x--i2c write success----\n",cmd);
#else
	ret=ktd3121_write_bytes(cmd,data);
	if(ret<0)
		printk("[KERNEL]nt35596----ktd3121---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printk("[KERNEL]nt35596----ktd3121---cmd=%0x-- i2c write success-----\n",cmd);
#endif

	cmd=0x03;
	data=0xFF;
#ifdef BUILD_LK
	ret=KTD3121_write_byte(cmd,data);
    if(ret)
	    dprintf(0, "[LK]nt35596----ktd3121----cmd=%0x--i2c write error----\n",cmd);
	else
		dprintf(0, "[LK]nt35596----ktd3121----cmd=%0x--i2c write success----\n",cmd);
#else
	ret=ktd3121_write_bytes(cmd,data);
	if(ret<0)
		printk("[KERNEL]nt35596----ktd3121---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printk("[KERNEL]nt35596----ktd3121---cmd=%0x-- i2c write success-----\n",cmd);
#endif

    cmd=0x04;
	data=0x00;
#ifdef BUILD_LK
	ret=KTD3121_write_byte(cmd,data);
    if(ret)
	    dprintf(0, "[LK]nt35596----ktd3121----cmd=%0x--i2c write error----\n",cmd);
	else
		dprintf(0, "[LK]nt35596----ktd3121----cmd=%0x--i2c write success----\n",cmd);
#else
	ret=ktd3121_write_bytes(cmd,data);
	if(ret<0)
		printk("[KERNEL]nt35596----ktd3121---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printk("[KERNEL]nt35596----ktd3121---cmd=%0x-- i2c write success-----\n",cmd);
#endif

    cmd=0x05;
	data=0x07;
#ifdef BUILD_LK
	ret=KTD3121_write_byte(cmd,data);
    if(ret)
	    dprintf(0, "[LK]nt35596----ktd3121----cmd=%0x--i2c write error----\n",cmd);
	else
		dprintf(0, "[LK]nt35596----ktd3121----cmd=%0x--i2c write success----\n",cmd);
#else
	ret=ktd3121_write_bytes(cmd,data);
	if(ret<0)
		printk("[KERNEL]nt35596----ktd3121---cmd=%0x-- i2c write error-----\n",cmd);
	else
		printk("[KERNEL]nt35596----ktd3121---cmd=%0x-- i2c write success-----\n",cmd);
#endif


#if 0
		KTD2125_enable(0);
		SET_RESET_PIN(1);
		mdelay(20);
		SET_RESET_PIN(0);
		mdelay(10);
		SET_RESET_PIN(1);
		mdelay(10);
		KTD2125_enable(1);
		mdelay(200);
		dsi_set_cmdq_V3(lcm_initialization_setting,sizeof(lcm_initialization_setting)/sizeof(lcm_initialization_setting[0]),1);
#else
		unsigned int data_array[35];
	#ifdef BUILD_LK
		  printf("[LK]---cmd---nt35596----%s------\n",__func__);
    #else
		  printk("[KERNEL]---cmd---nt35596----%s------\n",__func__);
    #endif
		KTD2125_enable(0);
		SET_RESET_PIN(1);
		MDELAY(20);
		SET_RESET_PIN(0);
		MDELAY(10);
		SET_RESET_PIN(1);
		MDELAY(10);
		KTD2125_enable(1);
		MDELAY(200);

	 
	 push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
#endif

}



static void lcm_suspend(void)
{
	unsigned int data_array[16];

	data_array[0]=0x00280500; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00100500; 
	dsi_set_cmdq(data_array, 1, 1);


	SET_RESET_PIN(1);
	MDELAY(20);
	SET_RESET_PIN(0);
	MDELAY(10); 

	SET_RESET_PIN(1);
	MDELAY(120);
	KTD2125_enable(0);
	
	
}

static void lcm_resume(void)
{
	
	
	lcm_init();
}

#if (LCM_DSI_CMD_MODE)
static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);

}
#endif

static unsigned int lcm_compare_id(void)
{
	return 1;
}


static unsigned int lcm_esd_check(void)
{
  #ifndef BUILD_LK
	char  buffer[4];
	int   array[4];

	if(lcm_esd_test)
	{
		lcm_esd_test = FALSE;
		return TRUE;
	}
	array[0] = 0x00043700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x09, buffer, 4);
	
	

	
	printk("<%s:%d>buffer[%x][%x][%x][%x]\n", __func__, __LINE__, buffer[0], buffer[1], buffer[2], buffer[3]);
	if((buffer[0]==0x80) && (buffer[1]==0x73) && (buffer[2]==0x04) && (buffer[3]==0x00))
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
#else
	return FALSE;
#endif

}

static unsigned int lcm_esd_recover(void)
{
	
	lcm_resume();

	return TRUE;
}

static void lcm_setbacklight(unsigned int level)
{
	LCD_LOG("%s, nt35596 backlight: level = %d\n", __func__, level);
	

	unsigned int cmd = 0x51;
	unsigned int count = 1;
	unsigned int value = level;
	dsi_set_cmdq_V2(cmd,count,&value,1);
}


LCM_DRIVER nt35596_fhd_dsi_vdo_truly_ktd3121_lcm_drv =
{
    .name			= "nt35596_fhd_dsi_vdo_truly_ktd3121",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
	.esd_check = lcm_esd_check,
	.esd_recover = lcm_esd_recover,
	.set_backlight	= lcm_setbacklight,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
    };
