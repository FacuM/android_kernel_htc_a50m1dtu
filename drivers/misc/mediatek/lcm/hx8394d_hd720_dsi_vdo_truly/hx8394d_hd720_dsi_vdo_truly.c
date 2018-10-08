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

#define   LCM_DSI_CMD_MODE							0



static LCM_setting_table_V3 lcm_initialization_setting[] = {
	
#if 1
	
	
	{0x39, 0xB9, 3, {0xFF,0x83,0x94}},

	
	{0x39, 0xBA, 2, {0x32,0x83}},

	
	{0x39, 0xB1, 15, {0x6C,0x11,0x11,0x24,0x04,0x11,0xF1,0x80,0xEA,0x96,0x23,0x80,0xC0,0xD2,0x58}},

	
	{0x39, 0xB2,11, {0x00,0x64,0x10,0x07,0x32,0x1C,0x08,0x08,0x1C,0x4D,0x00}},

	
	{0x15, 0xBC, 1, {0x07}},

	
	{0x39, 0xBF, 3, {0x41,0x0E,0x01}},


	
	{0x39, 0xB4, 12, {0x00,0xFF,0x40,0x50,0x40,0x50,0x40,0x50,0x01,0x6A,0x01,0x6A}},

	
	{0x39, 0xD3, 30, {0x00,0x06,0x00,0x40,0x07,0x00,0x00,0x32,0x10,0x08,
                         0x00,0x08,0x52,0x15,0x0F,0x05,0x0F,0x32,0x10,0x00,
                         0x00,0x00,0x47,0x44,0x0C,0x0C,0x47,0x0C,0x0C,0x47}},

	
	{0x39, 0xD5, 44, {0x20,0x21,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                         0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
                         0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
                         0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x19,0x19,
                         0x18,0x18,0x24,0x25}},

	
	{0x39, 0xD6, 44, {0x24,0x25,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00,
                         0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x58,0x58,
                         0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
                         0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
                         0x19,0x19,0x20,0x21}},

	
	{0x39, 0xE0, 42, {0x00,0x01,0x05,0x31,0x38,0x3F,0x15,0x3D,0x06,0x09,0x0B,0x16,0x0E,0x12,0x15,0x13,0x14,0x07,0x11,0x13,0x18,
                         0x00,0x01,0x05,0x31,0x37,0x3F,0x14,0x3E,0x06,0x09,0x0B,0x17,0x0F,0x12,0x15,0x13,0x14,0x07,0x11,0x12,0x16}},
	
	{0x15, 0xBD, 1, {0x00}},
	{0x39, 0xC1, 43, {0x01,0x00,0x08,0x10,0x17,0x1F,0x26,0x2E,0x35,0x3D,
                         0x44,0x4B,0x53,0x5B,0x62,0x6A,0x72,0x79,0x81,0x89,
                         0x91,0x99,0xA1,0xA9,0xB1,0xB9,0xC1,0xCA,0xD2,0xDA,
                         0xE3,0xEC,0xF5,0xFF,0x16,0x27,0xFB,0x29,0xD5,0x45,
                         0x22,0xFF,0xC0}},
	 
	{0x15, 0xBD, 1, {0x01}},
	{0x39, 0xC1, 42, {0x00,0x08,0x10,0x17,0x1F,0x26,0x2E,0x35,0x3D,0x44,
                         0x4B,0x53,0x5B,0x62,0x6A,0x72,0x79,0x81,0x89,0x91,
                         0x99,0xA1,0xA9,0xB1,0xB9,0xC1,0xCA,0xD2,0xDA,0xE3,
                         0xEC,0xF5,0xFF,0x16,0x27,0xFB,0x29,0xD5,0x45,0x22,
                         0xFF,0xC0}},
	
	{0x15, 0xBD, 1, {0x02}},
	{0x39, 0xC1, 42, {0x00,0x08,0x10,0x17,0x1F,0x26,0x2E,0x35,0x3D,0x44,
                         0x4B,0x53,0x5B,0x62,0x6A,0x72,0x79,0x81,0x89,0x91,
                         0x99,0xA1,0xA9,0xB1,0xB9,0xC1,0xCA,0xD2,0xDA,0xE3,
                         0xEC,0xF5,0xFF,0x16,0x27,0xFB,0x29,0xD5,0x45,0x22,
                         0xFF,0xC0}},

	
	{0x39, 0xB6, 2, {0x70,0x70}},

	
	{0x15, 0xCC, 1, {0x09}},

	
	{0x39, 0xC0, 2, {0x30,0x14}},

	
	{0x39, 0xC7, 4, {0x00,0xC0,0x40,0xC0}},


	{0x05, 0x11, 0, {}},
	{0x05, 0x29, 0, {}},




	
	{0x15, 0x51, 1, {0xFF}},
	{0x39, 0x53, 1, {0x24}},
	{0x39, 0x55, 1, {0x00}},
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
		params->module="K70506";
		params->vendor="truly";
		params->ic="hx8394d";
		params->info="720*1280";
		
#endif

        #if (LCM_DSI_CMD_MODE)
		params->dsi.mode   = CMD_MODE;
        #else
		params->dsi.mode   = SYNC_PULSE_VDO_MODE; 
        #endif
	
		
		
		
		params->dsi.LANE_NUM				= LCM_THREE_LANE;
		
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

        
		
		params->dsi.packet_size=512;

        
		params->dsi.intermediat_buffer_num = 0;

		
		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
        params->dsi.word_count=720*3;	
		
		params->dsi.vertical_sync_active				= 4;
		params->dsi.vertical_backporch					= 12;
		params->dsi.vertical_frontporch					= 15;
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 20;
		params->dsi.horizontal_backporch				= 80;
		params->dsi.horizontal_frontporch				= 80;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	    

		
		params->dsi.PLL_CLOCK = 306;
		
		
		
		

		params->pwm_min = 7;
		params->pwm_default = 94;
		params->pwm_max = 255;
		params->camera_blk = 194;
		params->camera_dua_blk = 194;
		params->camera_rec_blk = 194;

}
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
		MDELAY(10);
		mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);
		#ifdef BUILD_LK
		hx8394d_write_byte(0x00, 0x0f);
		hx8394d_write_byte(0x01, 0x0f);
		hx8394d_write_byte(0x02, 0x03);
		#endif
	}
	else
	{

		mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ZERO);
		MDELAY(10);
		mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);
		MDELAY(10);
	}

}
static void lcm_init(void)
{
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

		lcm_util.set_gpio_mode((GPIO128 | 0x80000000),0);
		lcm_util.set_gpio_dir((GPIO128 | 0x80000000),GPIO_DIR_OUT);
		lcm_util.set_gpio_out((GPIO128 | 0x80000000),0);


			unsigned int data_array[35];
	#ifdef BUILD_LK
		  printf("[LK]---cmd---hx8394d----%s------\n",__func__);
    #else
		  printk("[KERNEL]---cmd---hx8394d----%s------\n",__func__);
    #endif
		
		SET_RESET_PIN(1);
		MDELAY(2);
		SET_RESET_PIN(0);
		MDELAY(1);
		SET_RESET_PIN(1);
		MDELAY(5);
		KTD2125_enable(1);
		MDELAY(150);

	
		data_array[0] = 0x00043902; 						 
		data_array[1] = 0x9483FFB9; 				
	        dsi_set_cmdq(&data_array, 2, 1); 

	
		data_array[0] = 0x000c3902;
		data_array[1] = 0xa04372BA;
		data_array[2] = 0x0909b265;
		data_array[3] = 0x00001040;
		dsi_set_cmdq(&data_array, 4, 1);

	
		 data_array[0] = 0x00103902;			   
		 data_array[1] = 0x11116CB1;   
		 data_array[2] = 0xF1110424; 
		 data_array[3] = 0x2396EA80; 
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

	
                data_array[0] = 0x00043902;					   
		data_array[1] = 0x010E41BF; 			
		dsi_set_cmdq(&data_array, 2, 1);
	
	
		 data_array[0] = 0x000D3902;			   
		 data_array[1] = 0x40FF00B4;   
		 data_array[2] = 0x40504050; 
		 data_array[3] = 0x016A0150; 
		 data_array[4] = 0x0000006A; 
		 dsi_set_cmdq(&data_array, 5, 1);
	
	
		 data_array[0] = 0x001F3902;		   
		 data_array[1] = 0x000600D3;   
		 data_array[2] = 0x00000740; 
		 data_array[3] = 0x00081032; 
		 data_array[4] = 0x0F155208; 
		 data_array[5] = 0x10320F05; 
		 data_array[6] = 0x47000000; 
		 data_array[7] = 0x470C0C44; 
		 data_array[8] = 0x00470C0C; 
		 dsi_set_cmdq(&data_array,9, 1);

	
		 data_array[0] = 0x002D3902;		   
		 data_array[1] = 0x002120D5;   
		 data_array[2] = 0x04030201; 
		 data_array[3] = 0x18070605; 
		 data_array[4] = 0x18181818; 
		 data_array[5] = 0x18181818; 
		 data_array[6] = 0x18181818; 
		 data_array[7] = 0x18181818; 
		 data_array[8] = 0x18181818; 
	         data_array[9] = 0x18181818; 
		 data_array[10] = 0x19181818; 
		 data_array[11] = 0x24181819; 
		 data_array[12] = 0x00000025; 
		 dsi_set_cmdq(&data_array,13, 1);

	
		 data_array[0] = 0x002D3902;		   
		 data_array[1] = 0x072524D6;   
		 data_array[2] = 0x03040506; 
		 data_array[3] = 0x18000102; 
		 data_array[4] = 0x18181818; 
		 data_array[5] = 0x58181818; 
		 data_array[6] = 0x18181858; 
		 data_array[7] = 0x18181818; 
		 data_array[8] = 0x18181818; 
	         data_array[9] = 0x18181818; 
		 data_array[10] = 0x18181818; 
		 data_array[11] = 0x20191918; 
		 data_array[12] = 0x00000021; 
		 dsi_set_cmdq(&data_array,13, 1);
	

         
		 data_array[0] = 0x002B3902;		   
		 data_array[1] = 0x050100E0;   
		 data_array[2] = 0x153F3831; 
		 data_array[3] = 0x0B09063D; 
		 data_array[4] = 0x15120E16; 
		 data_array[5] = 0x11071413; 
		 data_array[6] = 0x01001813; 
		 data_array[7] = 0x3F373105; 
		 data_array[8] = 0x09063E14; 
	         data_array[9] = 0x120F170B; 
		 data_array[10] = 0x07141315; 
		 data_array[11] = 0x00161211; 
		 dsi_set_cmdq(&data_array,12, 1);

         
              data_array[0] = 0x00023902;					   
		data_array[1] = 0x000000BD; 			
		dsi_set_cmdq(&data_array, 2, 1);

               data_array[0] = 0x002C3902;		   
		 data_array[1] = 0x080001C1;   
		 data_array[2] = 0x261F1710; 
		 data_array[3] = 0x443D352E; 
		 data_array[4] = 0x625B534B; 
		 data_array[5] = 0x8179726A; 
		 data_array[6] = 0xA1999189; 
		 data_array[7] = 0xC1B9B1A9; 
		 data_array[8] = 0xE3DAD2CA; 
	        data_array[9] = 0x16FFF5EC; 
		 data_array[10] = 0xD529FB27; 
		 data_array[11] = 0xC0FF2245; 
		 dsi_set_cmdq(&data_array,12, 1);

               data_array[0] = 0x00023902;					   
		 data_array[1] = 0x000001BD; 			
		 dsi_set_cmdq(&data_array, 2, 1);

               data_array[0] = 0x002B3902;		   
		 data_array[1] = 0x100800C1;   
		 data_array[2] = 0x2E261F17; 
		 data_array[3] = 0x4B443D35; 
		 data_array[4] = 0x6A625B53; 
		 data_array[5] = 0x89817972; 
		 data_array[6] = 0xA9A19991; 
		 data_array[7] = 0xCAC1B9B1; 
		 data_array[8] = 0xECE3DAD2; 
	        data_array[9] = 0x2716FFF5; 
		 data_array[10] = 0x45D529FB; 
		 data_array[11] = 0x00C0FF22; 
		 dsi_set_cmdq(&data_array,12, 1);

               data_array[0] = 0x00023902;					   
		 data_array[1] = 0x000002BD; 			
		 dsi_set_cmdq(&data_array, 2, 1);

               data_array[0] = 0x002B3902;		   
		 data_array[1] = 0x100800C1;   
		 data_array[2] = 0x2E261F17; 
		 data_array[3] = 0x4B443D35; 
		 data_array[4] = 0x6A625B53; 
		 data_array[5] = 0x89817972; 
		 data_array[6] = 0xA9A19991; 
		 data_array[7] = 0xCAC1B9B1; 
		 data_array[8] = 0xECE3DAD2; 
	        data_array[9] = 0x2716FFF5; 
		 data_array[10] = 0x45D529FB; 
		 data_array[11] = 0x00C0FF22; 
		 dsi_set_cmdq(&data_array,12, 1);



	
                data_array[0] = 0x00033902;					   
		data_array[1] = 0x007070B6; 			
		dsi_set_cmdq(&data_array, 2, 1);	


	
                data_array[0] = 0x00023902;					   
		data_array[1] = 0x000009CC; 			
		dsi_set_cmdq(&data_array, 2, 1);	

	
                data_array[0] = 0x00033902;					   
		data_array[1] = 0x001430C0; 			
		dsi_set_cmdq(&data_array, 2, 1);	

	
		data_array[0] = 0x00053902;					   
		data_array[1] = 0x40C000C7; 
		data_array[2] = 0x000000C0; 				
		dsi_set_cmdq(&data_array, 3, 1);

	
		data_array[0] = 0x00110500;			   
		dsi_set_cmdq(&data_array, 1, 1); 
		MDELAY(120);
		 
 	
                data_array[0] = 0x00290500;			   
		dsi_set_cmdq(&data_array, 1, 1);
		MDELAY(10);
#endif

}



static void lcm_suspend(void)
{
	unsigned int data_array[16];

	
	

	data_array[0] = 0x00100500; 
	dsi_set_cmdq(data_array, 1, 1);


	
	
	SET_RESET_PIN(0);
	MDELAY(10); 
	
	
	
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

static unsigned int lcm_check_id(void)
{
	unsigned int retval = (which_lcd_module_triple() == 2) ? 1 : 0;
	return retval;

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

LCM_DRIVER hx8394d_hd720_dsi_vdo_truly_lcm_drv = 
{
    .name			= "hx8394d_hd720_dsi_vdo_truly",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
	.check_id	= lcm_check_id,
	.esd_check = lcm_esd_check,
	.esd_recover = lcm_esd_recover,
	.set_backlight	= lcm_setbacklight,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
    };
