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
#define dsi_swap_port(swap) lcm_util.dsi_swap_port(swap)

#define   LCM_DSI_CMD_MODE							0

#define REGFLAG_PORT_SWAP 0xFFFA
#define REGFLAG_DELAY 0xFFFC
#define REGFLAG_END_OF_TABLE 0xFFFD 

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {
	
#if 1
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x00}}, 
	{0xB0,4,{0x0F,0x0F,0x1E,0x14}}, 
	{0xB2,1,{0x00}}, 
	{0xB6,1,{0x03}}, 
	
	{0xC0,20,{0x03,0x00,0x06,0x07,0x08,0x09,0x00,0x00,0x00,0x00,
		  0x02,0x00,0x0A,0x0B,0x0C,0x0D,0x00,0x00,0x00,0x00}}, 
 	
	{0xC1,16,{0x08,0x24,0x24,0x01,0x18,0x24,0x9F,0x85,0x08,0x24,
		  0x24,0x01,0x18,0x24,0x9F,0x85}},
	
	{0xC2,24,{0x03,0x05,0x1B,0x24,0x13,0x31,0x01,0x05,0x1B,0x24,
		  0x13,0x31,0x03,0x05,0x1B,0x38,0x00,0x11,0x02,0x05,
		  0x1B,0x38,0x00,0x11}}, 
	
	{0xC3,24,{0x02,0x05,0x1B,0x24,0x13,0x11,0x03,0x05,0x1B,0x24,
		  0x13,0x11,0x03,0x05,0x1B,0x38,0x00,0x11,0x02,0x05,
		  0x1B,0x38,0x00,0x11}},
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x01}}, 
	
	
	{0xB7,1,{0x04}}, 
	{0xB8,1,{0x05}}, 
	{0xB9,1,{0x04}}, 
	{0xBA,1,{0x14}},
	
	
	{0xC2,3,{0x00,0x35,0x07}}, 
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x02}}, 
	
	
	{0x8F,6,{0x5A,0x96,0x3C,0xC3,0xA5,0x69}},
	{0x89,1,{0x00}},
	{0x8C,3,{0x55,0x41,0x53}},
	{0x9A,1,{0x5A}}, 
	{0x35,1,{0x00}}, 
	{0x11,1,{0x00}}, 
	{REGFLAG_DELAY,140,{}},
	{0x29,1,{0x00}}, 
	{REGFLAG_DELAY,140,{}},
	
	{REGFLAG_END_OF_TABLE,0x00,{}},
#endif
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28,1,{0x00}},
	{REGFLAG_DELAY,100,{}},
	{0x10,1,{0x00}},
	{REGFLAG_END_OF_TABLE,0x00,{}},
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

		case REGFLAG_END_OF_TABLE :
			break;

		case REGFLAG_PORT_SWAP:
			dsi_swap_port(1);
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

static const LCM_UTIL_FUNCS* lcm_get_util_funcs(void)
{
	return &lcm_util;
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
		params->module="edo";
		params->vendor="edo";
		params->ic="sh1368";
		params->info="720*1280";
		
#endif

        #if (LCM_DSI_CMD_MODE)
		params->dsi.mode   = CMD_MODE;
        #else
		params->dsi.mode   = BURST_VDO_MODE; 
        #endif
	
		
		
		
		params->dsi.LANE_NUM		    = LCM_FOUR_LANE;
		
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

        	
		
		params->dsi.packet_size=512;

        	
		params->dsi.intermediat_buffer_num = 0;

		
		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
        	params->dsi.word_count=720*3;	
		
		params->dsi.vertical_sync_active				= 4;
		params->dsi.vertical_backporch					= 15;
		params->dsi.vertical_frontporch					= 15;
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 4;
		params->dsi.horizontal_backporch				= 30;
		params->dsi.horizontal_frontporch				= 20;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	   	

		
		params->dsi.PLL_CLOCK = 203;
		
		
		
		

		params->pwm_min = 132;
		params->pwm_default = 197;
		params->pwm_max = 255;
		params->camera_blk = 255;
		params->camera_dua_blk = 255;
		params->camera_rec_blk = 255;

}
#if 0
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

	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_LCD_BIAS_ENP_PIN_M_GPIO);
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_LCD_BIAS_ENN_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);

	if (en)
	{

		mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
		mdelay(12);
		mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);
		#ifdef BUILD_LK
		hx8394d_write_byte(0x00, 0x0f);
		hx8394d_write_byte(0x01, 0x0f);
		#endif
	}
	else
	{

		mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ZERO);
		mdelay(12);
		mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);
	}

}
#endif

static void lcm_power_on(void) {

	
    
	SET_RESET_PIN(0);

	
	mt_set_gpio_mode(GPIO_LCD_VDDIO_EN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_VDDIO_EN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_VDDIO_EN_PIN, GPIO_OUT_ONE);
	
	mt_set_gpio_mode(GPIO_DSI_TE_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_DSI_TE_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_DSI_TE_PIN, GPIO_OUT_ONE);
	MDELAY(1);
	
	mt_set_gpio_mode(GPIO_LCD_VCI_EN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_VCI_EN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_VCI_EN_PIN, GPIO_OUT_ONE);
	MDELAY(10);

	
	SET_RESET_PIN(1);
	MDELAY(15);
	
	SET_RESET_PIN(0);
	MDELAY(5);
	
	SET_RESET_PIN(1);
	MDELAY(10);

}

static void lcm_power_off(void) {
	
	mt_set_gpio_mode(GPIO_DSI_TE_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_DSI_TE_PIN, GPIO_DIR_IN);
	mt_set_gpio_out(GPIO_DSI_TE_PIN, GPIO_OUT_ZERO);
	MDELAY(30);
	
	SET_RESET_PIN(0);
	MDELAY(5);
	
	mt_set_gpio_mode(GPIO_LCD_VCI_EN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_VCI_EN_PIN, GPIO_DIR_IN);
	mt_set_gpio_out(GPIO_LCD_VCI_EN_PIN, GPIO_OUT_ZERO);
	MDELAY(1);
	
	mt_set_gpio_mode(GPIO_LCD_VDDIO_EN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_VDDIO_EN_PIN, GPIO_DIR_IN);
	mt_set_gpio_out(GPIO_LCD_VDDIO_EN_PIN, GPIO_OUT_ZERO);
	MDELAY(1);

}


static void lcm_init_power(void)
{
	lcm_power_on();
}

static void lcm_resume_power(void)
{
	lcm_power_on();
}

static void lcm_suspend_power(void)
{
	lcm_power_off();
}

static void lcm_init(void)
{
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
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

static unsigned int lcm_check_id(void)
{
	unsigned char tmp = which_lcd_module_triple();
	unsigned int retval = (tmp == 10) ? 1 : 0;
	
	
	
	return retval;
}

static struct LCM_setting_table lcm_backlight_level_setting_C1[1] ={0xC1,2,{0x00,0x00}};
static void lcm_setbacklight_cmdq(void* handle,unsigned int level)
{
	unsigned int data_array[16];
	unsigned char als_radio_h,als_radio_l;

	LCD_LOG("%s, sh1368-backlight: level = %d\n", __func__, level);

	if(level>255) 
		level = 255;
	
	level = level*(1023-50)/255+50;
	als_radio_h = level>>8;
	als_radio_l = level;
	lcm_backlight_level_setting_C1[0].para_list[0]=als_radio_h;
	lcm_backlight_level_setting_C1[0].para_list[1]=als_radio_l;
	dsi_set_cmdq_V2(0xC1,2,lcm_backlight_level_setting_C1[0].para_list,1);
}

static void lcm_setbacklight(unsigned int level)
{
	lcm_setbacklight_cmdq(NULL,level);
}

static void lcm_set_lcm_cmd(void* handle,unsigned int *lcm_cmd,unsigned char *lcm_count,unsigned char *lcm_value)
{
	LCD_LOG("%s, lcm\n", __func__);

	unsigned int cmd = lcm_cmd[0];
	unsigned char count = lcm_count[0];
	unsigned char *ppara =  lcm_value;

	dsi_set_cmdq_V2(cmd, count, ppara, 1);
}


LCM_DRIVER sh1368_hd720_dsi_vdo_edo_lcm_drv =
{
	.name		= "sh1368_hd720_dsi_vdo_edo",
	.set_util_funcs	= lcm_set_util_funcs,
	.get_util_funcs	= lcm_get_util_funcs,
	.get_params	= lcm_get_params,
	.init		= lcm_init,
	.suspend	= lcm_suspend,
	.resume		= lcm_resume,
	.compare_id	= lcm_compare_id,
	.check_id	= lcm_check_id,
	.init_power	= lcm_init_power,
	.resume_power	= lcm_resume_power,
	.suspend_power	= lcm_suspend_power,
	.set_backlight_cmdq  = lcm_setbacklight_cmdq,
	.set_backlight = lcm_setbacklight,
	.set_lcm_cmd    = lcm_set_lcm_cmd,
#if (LCM_DSI_CMD_MODE)
	.update		= lcm_update,
#endif
};

