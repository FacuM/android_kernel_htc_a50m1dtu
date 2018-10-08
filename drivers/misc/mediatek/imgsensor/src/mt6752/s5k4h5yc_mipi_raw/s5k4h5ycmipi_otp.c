/*************************************************************************************************
4h5_otp.c
---------------------------------------------------------
OTP Application file From Truly for S5K4H5YC
2014.08.13
---------------------------------------------------------
NOTE:
The modification is appended to initialization of image sensor. 
After sensor initialization, use the function , and get the id value.
bool otp_wb_update(BYTE zone)
and
bool otp_lenc_update(BYTE zone), 
then the calibration of AWB and LSC will be applied. 
After finishing the OTP written, we will provide you the golden_rg and golden_bg settings.
**************************************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>  
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/slab.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "s5k4h5ycmipiraw_Sensor.h"

#define S5K4H5YCMIPI_WRITE_ID   (0x30)

extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
#define s5k4h5yc_write_cmos_sensor(addr, para)  iWriteReg((u16) addr , (u32) para , 1, S5K4H5YCMIPI_WRITE_ID)


#define USHORT             unsigned short
#define BYTE               unsigned char
#define Sleep(ms) mdelay(ms)

#define TRULY_ID           0x02
#define LARGAN_LENS        0x01
#define DONGWOON           0x01
#define TDK_VCM			       0x01
#define VALID_OTP          0x01
#define WB_OFFSET          0x17

#define GAIN_DEFAULT       0x0100

#define Golden_RG   0x312
#define Golden_BG   0x268

USHORT Current_RG;
USHORT Current_BG;


kal_uint32 s5k4h5yc_r_ratio;
kal_uint32 s5k4h5yc_b_ratio;

static kal_uint16 s5k4h5yc_read_cmos_sensor(kal_uint32 addr)
{
	  kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,S5K4H5YCMIPI_WRITE_ID);
    return get_byte;
}
static void start_read_otp()
{

	s5k4h5yc_write_cmos_sensor(0x3A02, 0x00);   
	s5k4h5yc_write_cmos_sensor(0x3A00, 0x01);   
	Sleep(20);
}

static void stop_read_otp()
{
	s5k4h5yc_write_cmos_sensor(0x3A00, 0x00);   
}


/*************************************************************************************************
* Function    :  get_otp_flag
* Description :  get otp WRITTEN_FLAG  
* Parameters  :  [BYTE] zone : OTP PAGE index , 0x00~0x0B
* Return      :  [BYTE], if 1 , this type has valid otp data, otherwise, invalid otp data
**************************************************************************************************/
static BYTE get_otp_flag(BYTE zone)
{
	BYTE flag = 0;
	start_read_otp();
	if(zone==0)
	{
		flag = s5k4h5yc_read_cmos_sensor(0x3A1A);
		
	}
	if(zone==1)
	{
		flag = s5k4h5yc_read_cmos_sensor(0x3A31);
		
	}
	stop_read_otp();
	printk("Flag:0x%02x",flag );
	if((flag&0x81)==0x00)
	{
		return 0;
		
	}
	if((flag&0x81)==0x01)
	{
		return 1;
		
	}
	if((flag&0x80)==0x80)
	{
		return 2;
		
	}
}

static bool get_otp_date(BYTE zone) 
{
	BYTE year  = 0;
	BYTE month = 0;
	BYTE day   = 0;
  start_read_otp();
  year  = s5k4h5yc_read_cmos_sensor(0x3A05+zone*WB_OFFSET);
  month = s5k4h5yc_read_cmos_sensor(0x3A06+zone*WB_OFFSET);
  day   = s5k4h5yc_read_cmos_sensor(0x3A07+zone*WB_OFFSET);
	stop_read_otp();

  printk("OTP date=%02d.%02d.%02d", year,month,day);

	return 1;
}


static BYTE get_otp_module_id(BYTE zone)
{
	BYTE module_id = 0;
    start_read_otp();

    module_id  = s5k4h5yc_read_cmos_sensor(0x3A04+zone*WB_OFFSET);
	 
	stop_read_otp();

	printk("OTP_Module ID: 0x%02x.\n",module_id);

	return module_id;
}


static BYTE get_otp_lens_id(BYTE zone)
{
	BYTE lens_id = 0;

    start_read_otp();

    lens_id  = s5k4h5yc_read_cmos_sensor(0x3A08+zone*WB_OFFSET);
  
	stop_read_otp();

	printk("OTP_Lens ID: 0x%02x.\n",lens_id);

	return lens_id;
}


static BYTE get_otp_vcm_id(BYTE zone)
{
	BYTE vcm_id = 0;

    start_read_otp();

	vcm_id = s5k4h5yc_read_cmos_sensor(0x3A09+zone*WB_OFFSET);

	stop_read_otp();

	printk("OTP_VCM ID: 0x%02x.\n",vcm_id);

	return vcm_id;	
}


static BYTE get_otp_driver_id(BYTE zone)
{
	BYTE driver_id = 0;

    start_read_otp();

	driver_id = s5k4h5yc_read_cmos_sensor(0x3A0A+zone*WB_OFFSET);

	stop_read_otp();

	printk("OTP_Driver ID: 0x%02x.\n",driver_id);

	return driver_id;
}

static BYTE get_light_id(BYTE zone)
{
	BYTE light_id = 0;

    start_read_otp();
	light_id = s5k4h5yc_read_cmos_sensor(0x0A0D);

	stop_read_otp();

	printk("OTP_Light ID: 0x%02x.\n",light_id);

	return light_id;
}


static bool otp_lenc_update(USHORT zone)
{
	s5k4h5yc_write_cmos_sensor(0x0100, 0x01);
	Sleep(10);
	USHORT temp;
	temp = s5k4h5yc_read_cmos_sensor(0x3300);
	temp = temp | 0x01;
	s5k4h5yc_write_cmos_sensor(0x3300, temp);
	Sleep(50);
	
	temp = s5k4h5yc_read_cmos_sensor(0x3300);
	temp = temp & 0xfe;
	s5k4h5yc_write_cmos_sensor(0x3300, temp);
	return 1;
}




static bool wb_gain_set()
{
	USHORT R_GAIN;
	USHORT B_GAIN;
	USHORT Gr_GAIN;
	USHORT Gb_GAIN;
	USHORT G_GAIN;
		
	if(!s5k4h5yc_r_ratio || !s5k4h5yc_b_ratio)
	{
		printk("OTP WB ratio Data Err!");
			return 0;
	}



	if(s5k4h5yc_r_ratio >= 512 )
	{
		if(s5k4h5yc_b_ratio>=512) 
		{
			R_GAIN = (USHORT)(GAIN_DEFAULT * s5k4h5yc_r_ratio / 512);						
			G_GAIN = GAIN_DEFAULT;
			B_GAIN = (USHORT)(GAIN_DEFAULT * s5k4h5yc_b_ratio / 512);
		}
	    else
		{
			R_GAIN =  (USHORT)(GAIN_DEFAULT*s5k4h5yc_r_ratio / s5k4h5yc_b_ratio );
			G_GAIN = (USHORT)(GAIN_DEFAULT*512 / s5k4h5yc_b_ratio );
			B_GAIN = GAIN_DEFAULT;    
		 }
	}
    else
    {
	   if(s5k4h5yc_b_ratio >= 512)
		{
			R_GAIN = GAIN_DEFAULT;
			G_GAIN = (USHORT)(GAIN_DEFAULT*512 /s5k4h5yc_r_ratio);		
			B_GAIN =  (USHORT)(GAIN_DEFAULT*s5k4h5yc_b_ratio / s5k4h5yc_r_ratio );

		} 
		else
		{			
			 Gr_GAIN = (USHORT)(GAIN_DEFAULT*512/ s5k4h5yc_r_ratio );						
			 Gb_GAIN = (USHORT)(GAIN_DEFAULT*512/s5k4h5yc_b_ratio );						
			 if(Gr_GAIN >= Gb_GAIN)						
			 {						
				R_GAIN = GAIN_DEFAULT;						
				G_GAIN = (USHORT)(GAIN_DEFAULT *512/ s5k4h5yc_r_ratio );						
				B_GAIN =  (USHORT)(GAIN_DEFAULT*s5k4h5yc_b_ratio / s5k4h5yc_r_ratio );						
			 } 
			 else
			 {						
				R_GAIN =  (USHORT)(GAIN_DEFAULT*s5k4h5yc_r_ratio  / s5k4h5yc_b_ratio);						
				G_GAIN = (USHORT)(GAIN_DEFAULT*512 / s5k4h5yc_b_ratio );						
				B_GAIN = GAIN_DEFAULT;	
			 }
						
		}        
		
    }
    kal_uint16 R_GainH, B_GainH, G_GainH;
    kal_uint16 R_GainL, B_GainL, G_GainL;

    R_GainH = (R_GAIN & 0xff00)>>8;
    R_GainL = (R_GAIN & 0x00ff);

    B_GainH = (B_GAIN & 0xff00)>>8;
    B_GainL = (B_GAIN & 0x00ff);

    G_GainH = (G_GAIN & 0xff00)>>8;
    G_GainL = (G_GAIN & 0x00ff);


    printk("OTP_golden_rg=0x%x,golden_bg=0x%x.\n",Golden_RG,Golden_BG);
    printk("OTP_current_rg=0x%x,current_bg=0x%x.\n",Current_RG,Current_BG);
    printk("OTP_s5k4h5yc_r_ratio=0x%x,s5k4h5yc_b_ratio=0x%x. \n",s5k4h5yc_r_ratio,s5k4h5yc_b_ratio);

    s5k4h5yc_write_cmos_sensor(0x020e, G_GainH);
    s5k4h5yc_write_cmos_sensor(0x020f, G_GainL);
    s5k4h5yc_write_cmos_sensor(0x0210, R_GainH);
    s5k4h5yc_write_cmos_sensor(0x0211, R_GainL);
    s5k4h5yc_write_cmos_sensor(0x0212, B_GainH);
    s5k4h5yc_write_cmos_sensor(0x0213, B_GainL);
    s5k4h5yc_write_cmos_sensor(0x0214, G_GainH);
    s5k4h5yc_write_cmos_sensor(0x0215, G_GainL);
    printk("OTP WB Update Finished! \n");
    return 1;
	
}

static bool get_otp_wb(BYTE zone)
{

	BYTE temph = 0;
	BYTE templ = 0;
	
	start_read_otp();

	temph = s5k4h5yc_read_cmos_sensor(0x3A0B+zone*WB_OFFSET);
	templ = s5k4h5yc_read_cmos_sensor(0x3A0C+zone*WB_OFFSET);

	Current_RG = (USHORT)((temph<<8)| templ);

	temph = s5k4h5yc_read_cmos_sensor(0x3A0D + zone*WB_OFFSET);
	
	templ = s5k4h5yc_read_cmos_sensor(0x3A0E + zone*WB_OFFSET);

	Current_BG = (USHORT)((temph<<8)| templ);

	stop_read_otp();

	printk("%s,Current_RG = 0x%x,Current_BG = 0x%x.\n",__func__,Current_RG,Current_BG);
	return 1;
}


static bool otp_wb_update(BYTE zone)
{
	USHORT golden_g, current_g;


	if(!get_otp_wb(zone))  
		return 0;
		
	s5k4h5yc_r_ratio = 512 * Golden_RG / Current_RG;
	s5k4h5yc_b_ratio = 512 * Golden_BG / Current_BG;

	wb_gain_set();

	printk("WB update finished! \n");

	return 1;
}

bool s5k4h5yc_otp_update()
{
	BYTE zone = 0x00;
	BYTE FLG = 0x00;
	BYTE MID = 0x00,LENS_ID= 0x00,VCM_ID= 0x00;
	int i;
	
	for(i=0;i<2;i++)
	{
		FLG = get_otp_flag(zone);
		if(FLG == VALID_OTP)
			break;
		else
			zone++;
	}
	if(i==2)
	{
		printk("YSZ_Warning: No OTP Data or OTP data is invalid!!");
		return 0;
	}
	
	MID = get_otp_module_id(zone);
	LENS_ID = get_otp_lens_id(zone);
	VCM_ID = get_otp_vcm_id(zone);
	get_otp_date(zone);
	get_otp_driver_id(zone);
	if(MID != TRULY_ID)
	{
		printk("YSZ_Warning: No Truly Module / LARGAN LENS / TDK VCM !!!!");
		return 0;
	}
	otp_wb_update(zone);	
	otp_lenc_update(zone);

	return 1;
	
}
