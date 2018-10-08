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
    #include <linux/delay.h>
	#include <mach/mt_gpio.h>
	#include <linux/i2c.h>
#endif


#define FRAME_WIDTH  (720)
#define FRAME_HEIGHT (1280)

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


static LCM_setting_table_V3 lcm_initialization_setting[] = {

#if 1
	
	
	{0x39, 0xB9, 3, {0xFF,0x83,0x94}},

	
	{0x39, 0xBA, 2, {0x73,0x83}},

	
	{0x39, 0xB1, 15, {0x6C,0x55,0x15,0x11,0x04,0x11,0xF1,0x80,0xE8,0x95,0x23,0x80,0xC0,0xD2,0x58}},

	
	{0x39, 0xB2,11, {0x00,0x64,0x10,0x07,0x32,0x1C,0x08,0x08,0x1C,0x4D,0x00}},

	
	{0x15, 0xBC, 1, {0x07}},

	
	{0x39, 0xB4, 12, {0x00,0xFF,0x03,0x5c,0x03,0x5c,0x03,0x5c,0x01,0x70,0x01,0x70}},

     
	{0x15, 0xD2, 1, {0x55}},

	
	{0x39, 0xD3, 30, {0x00,0x06,0x00,0x01,0x1A,0x08,0x00,0x32,0x10,0x07,
                         0x00,0x07,0x54,0x15,0x0F,0x05,0x04,0x02,0x12,0x10,
                         0x05,0x07,0x33,0x33,0x0B,0x0B,0x37,0x10,0x07,0x07}},



	
	{0x39, 0xD5, 44, {0x19,0x19,0x18,0x18,0x1B,0x1B,0x1A,0x1A,0x04,0x05,
                         0x06,0x07,0x00,0x01,0x02,0x03,0x20,0x21,0x18,0x18,
                         0x22,0x23,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
                         0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
                         0x18,0x18,0x18,0x18}},

	
	{0x39, 0xD6, 44, {0x18,0x18,0x19,0x19,0x1B,0x1B,0x1A,0x1A,0x03,0x02,
                         0x01,0x00,0x07,0x06,0x05,0x04,0x23,0x22,0x18,0x18,
                         0x21,0x20,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
                         0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
                         0x18,0x18,0x18,0x18}},

	
	{0x39, 0xE0, 42, {0x00,0x03,0x09,0x23,0x27,0x3F,0x1C,0x3A,0x06,0x0A,0x0B,0x17,0x0D,0x10,0x12,0x11,0x13,0x07,0x11,0x14,0x18,
                         0x00,0x03,0x09,0x23,0x29,0x3F,0x1C,0x3A,0x06,0x0A,0x0B,0x17,0x0E,0x11,0x12,0x11,0x13,0x07,0x12,0x13,0x18}},

	
	{0x39, 0xC0, 2, {0x30,0x14}},

	
	{0x39, 0xC7, 4, {0x00,0xC0,0x00,0xC0}},


	
	{0x15, 0xCC, 1, {0x09}},

	
	{0x15, 0xDF, 1, {0x87}},

	{0x15, 0x51, 1, {0xFF}},

	{0x39, 0xC9, 1, {0x1F,0x00,0x14,0x1E,0x81,0x1E,0x00}},

	{0x39, 0x55, 1, {0x01}},
	{0x39, 0x5E, 1, {0x00}},

	
    {0x39, 0xCA, 9, {0x40,0x30,0x2F,0x2E,0x2D,0x26,0x23,0x21,0x20}},

    
    {0x39, 0xCE, 9, {0x00,0x00,0x00,0x10,0x10,0x10,0x10,0x20,0x20,0x20,
						0x20,0x20,0x20,0x20,0x30,0x30,0x30,0x30,0x30,0x30,
						0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
						0x30,0x30,0x30,0x00}},

    {0x39, 0x53, 1, {0x24}},

	{0x05, 0x11, 0, {}},
	{0x05, 0x29, 0, {}},

#endif
};


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
		params->ic="hx8394d";
		params->info="720*1280";
		
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
        params->dsi.word_count=720*3;

		params->dsi.vertical_sync_active				= 2;
		params->dsi.vertical_backporch					= 6;
		params->dsi.vertical_frontporch					= 16;
		params->dsi.vertical_active_line				= FRAME_HEIGHT;

		params->dsi.horizontal_sync_active				= 30;
		params->dsi.horizontal_backporch				= 70;
		params->dsi.horizontal_frontporch				= 100;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	    

		
		params->dsi.PLL_CLOCK = 198;
		
		
		
		

		params->pwm_min = 6;
		params->pwm_default = 88;
		params->pwm_max = 255;
		params->camera_blk = 192;
		params->camera_dua_blk = 192;
		params->camera_rec_blk = 168;

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

	mt_set_gpio_mode(GPIO_LCD_ENP_PIN, GPIO_LCD_BIAS_ENP_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_LCD_ENP_PIN, GPIO_DIR_OUT);

	mt_set_gpio_mode(GPIO_LCD_ENN_PIN, GPIO_LCD_BIAS_ENP_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_LCD_ENN_PIN, GPIO_DIR_OUT);

	if (en)
	{

		mt_set_gpio_out(GPIO_LCD_ENP_PIN, GPIO_OUT_ONE);
		mdelay(10);
		mt_set_gpio_out(GPIO_LCD_ENN_PIN, GPIO_OUT_ONE);

		#ifdef BUILD_LK
		hx8394d_write_byte(0x00, 0x0f);
		hx8394d_write_byte(0x01, 0x0f);
		#endif
	}
	else
	{
		mt_set_gpio_out(GPIO_LCD_ENN_PIN, GPIO_OUT_ZERO);
		mdelay(10);
		mt_set_gpio_out(GPIO_LCD_ENP_PIN, GPIO_OUT_ZERO);
	}

}
static void lcm_init(void)
{

    unsigned char cmd = 0x0;
	unsigned char data = 0xFF;
	int ret=0;

	mt_set_gpio_mode(GPIO_LCM_BL_EN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCM_BL_EN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCM_BL_EN, GPIO_OUT_ONE);

	cmd=0x02;
	data=0x6D;
#ifdef BUILD_LK
	ret=KTD3121_write_byte(cmd,data);
    if(ret)
    dprintf(0, "[LK]hx8394d----ktd3121----cmd=%0x--i2c write error----\n",cmd);
	else
	dprintf(0, "[LK]hx8394d----ktd3121----cmd=%0x--i2c write success----\n",cmd);
#else
	ret=ktd3121_write_bytes(cmd,data);
	if(ret<0)
	printk("[KERNEL]hx8394d----ktd3121---cmd=%0x-- i2c write error-----\n",cmd);
	else
	printk("[KERNEL]hx8394d----ktd3121---cmd=%0x-- i2c write success-----\n",cmd);
#endif

	cmd=0x03;
	data=0xFF;
#ifdef BUILD_LK
	ret=KTD3121_write_byte(cmd,data);
    if(ret)
	    dprintf(0, "[LK]hx8394d----ktd3121----cmd=%0x--i2c write error----\n",cmd);
	else
		dprintf(0, "[LK]hx8394d----ktd3121----cmd=%0x--i2c write success----\n",cmd);
#else
	ret=ktd3121_write_bytes(cmd,data);
	if(ret<0)
	printk("[KERNEL]hx8394d----ktd3121---cmd=%0x-- i2c write error-----\n",cmd);
	else
	printk("[KERNEL]hx8394d----ktd3121---cmd=%0x-- i2c write success-----\n",cmd);
#endif

    cmd=0x04;
	data=0x00;
#ifdef BUILD_LK
	ret=KTD3121_write_byte(cmd,data);
    if(ret)
	    dprintf(0, "[LK]hx8394d----ktd3121----cmd=%0x--i2c write error----\n",cmd);
	else
		dprintf(0, "[LK]hx8394d----ktd3121----cmd=%0x--i2c write success----\n",cmd);
#else
	ret=ktd3121_write_bytes(cmd,data);
	if(ret<0)
	printk("[KERNEL]hx8394d----ktd3121---cmd=%0x-- i2c write error-----\n",cmd);
	else
	printk("[KERNEL]hx8394d----ktd3121---cmd=%0x-- i2c write success-----\n",cmd);
#endif

    cmd=0x05;
	data=0x07;
#ifdef BUILD_LK
	ret=KTD3121_write_byte(cmd,data);
    if(ret)
	    dprintf(0, "[LK]hx8394d----ktd3121----cmd=%0x--i2c write error----\n",cmd);
	else
		dprintf(0, "[LK]hx8394d----ktd3121----cmd=%0x--i2c write success----\n",cmd);
#else
	ret=ktd3121_write_bytes(cmd,data);
	if(ret<0)
	printk("[KERNEL]hx8394d----ktd3121---cmd=%0x-- i2c write error-----\n",cmd);
	else
	printk("[KERNEL]hx8394d----ktd3121---cmd=%0x-- i2c write success-----\n",cmd);
#endif

    cmd=0x06;
	data=0x00;
#ifdef BUILD_LK
	ret=KTD3121_write_byte(cmd,data);
    if(ret)
	    dprintf(0, "[LK]hx8394d----ktd3121----cmd=%0x--i2c write error----\n",cmd);
	else
		dprintf(0, "[LK]hx8394d----ktd3121----cmd=%0x--i2c write success----\n",cmd);
#else
	ret=ktd3121_write_bytes(cmd,data);
	if(ret<0)
	printk("[KERNEL]hx8394d----ktd3121---cmd=%0x-- i2c write error-----\n",cmd);
	else
	printk("[KERNEL]hx8394d----ktd3121---cmd=%0x-- i2c write success-----\n",cmd);
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
		  printf("[LK]---cmd---hx8394d----%s------\n",__func__);
    #else
		  printk("[KERNEL]---cmd---hx8394d----%s------\n",__func__);
    #endif
		KTD2125_enable(0);
		 SET_RESET_PIN(1);
		mdelay(20);
		SET_RESET_PIN(0);
		mdelay(10);
		SET_RESET_PIN(1);
		mdelay(10);
		KTD2125_enable(1);
		mdelay(200);

	
		data_array[0] = 0x00043902;
		data_array[1] = 0x9483FFB9;
	    dsi_set_cmdq(&data_array, 2, 1);

	
		data_array[0] = 0x00033902;
		data_array[1] = 0x008373BA;
	    dsi_set_cmdq(&data_array, 2, 1);

	
		 data_array[0] = 0x00103902;
		 data_array[1] = 0x15556CB1;
		 data_array[2] = 0xF1110411;
		 data_array[3] = 0x2395E880;
		 data_array[4] = 0x58D2C080;
		 dsi_set_cmdq(&data_array, 5, 1);

	
        data_array[0] = 0x000C3902;
		data_array[1] = 0x106400B2;
		data_array[2] = 0x081C3207;
        data_array[3] = 0x004D1C08;
		dsi_set_cmdq(&data_array, 4, 1);

	
        data_array[0] = 0x00023902;
		data_array[1] = 0x000007BC;
		dsi_set_cmdq(&data_array, 2, 1);

	
		 data_array[0] = 0x000D3902;
		 data_array[1] = 0x03FF00B4;
		 data_array[2] = 0x035C035C;
		 data_array[3] = 0x0170015C;
		 data_array[4] = 0x00000070;
		 dsi_set_cmdq(&data_array, 5, 1);

	
		 data_array[0] = 0x00023902;
	     data_array[1] = 0x000055D2;
		 dsi_set_cmdq(&data_array, 2, 1);

	
		 data_array[0] = 0x001F3902;
		 data_array[1] = 0x000600D3;
		 data_array[2] = 0x00081A01;
		 data_array[3] = 0x00071032;
		 data_array[4] = 0x0F155407;
		 data_array[5] = 0x12020405;
		 data_array[6] = 0x33070510;
		 data_array[7] = 0x370B0B33;
		 data_array[8] = 0x00070710;
		 dsi_set_cmdq(&data_array,9, 1);

	
		 data_array[0] = 0x002D3902;
		 data_array[1] = 0x181919D5;
		 data_array[2] = 0x1A1B1B18;
		 data_array[3] = 0x0605041A;
		 data_array[4] = 0x02010007;
		 data_array[5] = 0x18212003;
		 data_array[6] = 0x18232218;
		 data_array[7] = 0x18181818;
		 data_array[8] = 0x18181818;
	     data_array[9] = 0x18181818;
		 data_array[10] = 0x18181818;
		 data_array[11] = 0x18181819;
		 data_array[12] = 0x00000018;
		 dsi_set_cmdq(&data_array,13, 1);

	
		 data_array[0] = 0x002D3902;
		 data_array[1] = 0x191818D6;
		 data_array[2] = 0x1A1B1B19;
		 data_array[3] = 0x0102031A;
		 data_array[4] = 0x05060700;
		 data_array[5] = 0x18222304;
		 data_array[6] = 0x18202118;
		 data_array[7] = 0x18181818;
		 data_array[8] = 0x18181818;
	     data_array[9] = 0x18181818;
		 data_array[10] = 0x18181818;
		 data_array[11] = 0x18181818;
		 data_array[12] = 0x00000018;
		 dsi_set_cmdq(&data_array,13, 1);


         
		 data_array[0] = 0x002B3902;
		 data_array[1] = 0x090300E0;
		 data_array[2] = 0x1C3F2723;
		 data_array[3] = 0x0B0A063A;
		 data_array[4] = 0x12100D17;
		 data_array[5] = 0x11071311;
		 data_array[6] = 0x03001814;
		 data_array[7] = 0x3F292309;
		 data_array[8] = 0x0A063A1C;
	     data_array[9] = 0x110E170B;
		 data_array[10] = 0x07131112;
		 data_array[11] = 0x00181312;
		 dsi_set_cmdq(&data_array,12, 1);

	
        data_array[0] = 0x00033902;
		data_array[1] = 0x001430C0;
		dsi_set_cmdq(&data_array, 2, 1);

	
		data_array[0] = 0x00053902;
		data_array[1] = 0x00C000C7;
		data_array[2] = 0x000000C0;
		dsi_set_cmdq(&data_array, 3, 1);

	
        data_array[0] = 0x00023902;
		data_array[1] = 0x000009CC;
		dsi_set_cmdq(&data_array, 2, 1);

	 
        data_array[0] = 0x00023902;
		data_array[1] = 0x000087DF;
		dsi_set_cmdq(&data_array, 2, 1);


    
        data_array[0] = 0x00023902;
		data_array[1] = 0x0000FF51;
		dsi_set_cmdq(&data_array, 2, 1);

    
        data_array[0] = 0x00083902;
		data_array[1] = 0x14001FC9;
		data_array[2] = 0x001E811E;
		dsi_set_cmdq(&data_array, 3, 1);

    
        data_array[0] = 0x00023902;
		data_array[1] = 0x00000155;
		dsi_set_cmdq(&data_array, 2, 1);

		mdelay(5);

	 
        data_array[0] = 0x00023902;
		data_array[1] = 0x0000005E;
		dsi_set_cmdq(&data_array, 2, 1);

	  
        data_array[0] = 0x000A3902;
		data_array[1] = 0x2F3040CA;
		data_array[2] = 0x23262D2E;
		data_array[3] = 0x00002021;
		dsi_set_cmdq(&data_array, 4, 1);


     
        data_array[0] = 0x00243902;
		data_array[1] = 0x000000CE;
		data_array[2] = 0x10101010;
		data_array[3] = 0x20202020;
		data_array[4] = 0x30202020;
		data_array[5] = 0x30303030;
		data_array[6] = 0x30303030;
		data_array[7] = 0x30303030;
		data_array[8] = 0x30303030;
		data_array[9] = 0x00303030;
		dsi_set_cmdq(&data_array, 10, 1);

     
        data_array[0] = 0x00023902;
		data_array[1] = 0x00002453;
		dsi_set_cmdq(&data_array, 2, 1);

	
		data_array[0] = 0x00110500;
		dsi_set_cmdq(&data_array, 1, 1);
		mdelay(120);

	
        data_array[0] = 0x00290500;
		dsi_set_cmdq(&data_array, 1, 1);
		mdelay(10);
	 
		data_array[0] = 0x00023902;
		data_array[1] = 0x00002453;
		dsi_set_cmdq(&data_array, 2, 1);

#endif
}

static void lcm_suspend(void)
{
	unsigned int data_array[16];

	data_array[0]=0x00280500; 
	dsi_set_cmdq(data_array, 1, 1);

	data_array[0] = 0x00100500; 
	dsi_set_cmdq(data_array, 1, 1);

	mdelay(40);
	SET_RESET_PIN(0);
	mdelay(5); 

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
	int   array[4];
	char  buffer[3];
	char  id0=0;
	char  id1=0;
	char  id2=0;
			int  id=0;
		SET_RESET_PIN(1);
		mdelay(20);
		SET_RESET_PIN(0);
		mdelay(10);
		SET_RESET_PIN(1);
		mdelay(50);

			array[0] = 0x00013700;
		dsi_set_cmdq(array, 1, 1);

		read_reg_v2(0xda, buffer, 1);
			id0 = buffer[0]; 


			array[0] = 0x00013700;
		dsi_set_cmdq(array, 1, 1);

		read_reg_v2(0xdb, buffer, 1);
			id1 = buffer[0]; 

				
				

		   id = (id0<<8)|id1; 
#ifdef BUILD_LK
	printf("%s, LK hx8394d id0 = 0x%08x\n", __func__, id0);
	printf("%s, LK hx8394d id1 = 0x%08x\n", __func__, id1);
	printf("%s, lk hx8394d id = 0x%08x\n", __func__, id);
#else
	printk("%s, Kernel hx8394d id0 = 0x%08x\n", __func__, id0);
	printk("%s, Kernel hx8394d id1 = 0x%08x\n", __func__, id1);
	printk("%s, Kernel hx8394d id = 0x%08x\n", __func__, id);
#endif

	   if(id == 0X3094)
			return 1;
		else
		   return 0;

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
	LCD_LOG("%s, hx8394d backlight: level = %d\n", __func__, level);
	

	unsigned int cmd = 0x51;
	unsigned int count = 1;
	unsigned int value = level;
	dsi_set_cmdq_V2(cmd,count,&value,1);
}

LCM_DRIVER hx8394d_hd720_dsi_vdo_truly_ktd3121_lcm_drv =
{
    .name			= "hx8394d_hd720_dsi_vdo_truly_ktd3121",
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
