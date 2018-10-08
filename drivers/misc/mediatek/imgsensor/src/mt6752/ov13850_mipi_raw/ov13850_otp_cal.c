#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/xlog.h>

#include <linux/proc_fs.h> 


#include <linux/dma-mapping.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "ov13850mipiraw_Sensor.h"
#include "ov13850_otp.h"

#define OV13850_DEBUG  
#ifdef OV13850_DEBUG
	#define OV13850DB(fmt, arg...) xlog_printk(ANDROID_LOG_DEBUG, "[OV13850Raw] ",  fmt, ##arg)
#else
	#define OV13850DB(fmt, arg...)
#endif

extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);

static unsigned char OV13850MIPI_WRITE_ID = 0x6c;

#define OV13850_write_i2c(addr, para) iWriteReg((u16) addr , (u32) para , 1, OV13850MIPI_WRITE_ID)


#define Delay(ms)  mdelay(ms)

static kal_uint16 OV13850_read_i2c(kal_uint32 addr)
{
    kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,OV13850MIPI_WRITE_ID);
    return get_byte;
}
int check_otp_wb(int index)
{
	int flag;
	
	int temp1;
	temp1 = OV13850_read_i2c(0x5002);
	OV13850_write_i2c(0x5002, (0x00 & 0x02) | (temp1 & (~0x02)));
	OV13850_write_i2c(0x3d84, 0xC0);
	
	OV13850_write_i2c(0x3d88, 0x72);
	OV13850_write_i2c(0x3d89, 0x30);
	
	OV13850_write_i2c(0x3d8A, 0x72);
	OV13850_write_i2c(0x3d8B, 0x30);
	
	OV13850_write_i2c(0x3d81, 0x01);
	Delay(5);
	
	flag = OV13850_read_i2c(0x7230);
	if (index == 1)
	{
		flag = (flag>>6) & 0x03;
	}
	else if (index == 2)
	{
		flag = (flag>>4) & 0x03;
	}
	else if (index == 3)
	{
		flag = (flag>>2) & 0x03;
	}
	
	OV13850_write_i2c(0x7230, 0x00);
	
	temp1 = OV13850_read_i2c(0x5002);
	OV13850_write_i2c(0x5002, (0x02 & 0x02) | (temp1 & (~0x02)));
	if (flag == 0x00) {
		return 0;
	}
	else if (flag & 0x02) {
		return 1;
	}
	else {
		return 2;
	}
}
int check_otp_lenc(int index)
{
	int flag;
	
	int temp1;
	temp1 = OV13850_read_i2c(0x5002);
	OV13850_write_i2c(0x5002, (0x00 & 0x02) | (temp1 & (~0x02)));
	OV13850_write_i2c(0x3d84, 0xC0);
	
	OV13850_write_i2c(0x3d88, 0x72);
	OV13850_write_i2c(0x3d89, 0x4a);
	
	OV13850_write_i2c(0x3d8A, 0x72);
	OV13850_write_i2c(0x3d8B, 0x4a);
	
	OV13850_write_i2c(0x3d81, 0x01);
	Delay(5);
	flag = OV13850_read_i2c(0x724a);
	if (index == 1)
	{
		flag = (flag>>6) & 0x03;
	}
	else if (index == 2)
	{
		flag = (flag>>4) & 0x03;
	}
	else if (index == 3)
	{
		flag = (flag>> 2)& 0x03;
	}
	
	OV13850_write_i2c(0x724a, 0x00);
	
	temp1 = OV13850_read_i2c(0x5002);
	OV13850_write_i2c(0x5002, (0x02 & 0x02) | (temp1 & (~0x02)));
	if (flag == 0x00) {
		return 0;
	}
	else if (flag & 0x02) {
		return 1;
	}
	else {
		return 2;
	}
}
int read_otp_wb(int index, struct otp_struct *otp_ptr)
{
	int i;
	int temp;
	int start_addr, end_addr;
	int rg_typical,bg_typical;

	if (index == 1) {
		start_addr = 0x7231;
		end_addr = 0x7235;
	}
	else if (index == 2) {
		start_addr = 0x7236;
		end_addr = 0x723A;
	}
	else if (index == 3) {
		start_addr = 0x723B;
		end_addr = 0x723F;
	}
	
	int temp1;
	temp1 = OV13850_read_i2c(0x5002);
	OV13850_write_i2c(0x5002, (0x00 & 0x02) | (temp1 & (~0x02)));
	OV13850_write_i2c(0x3d84, 0xC0);
	
	OV13850_write_i2c(0x3d88, (start_addr >> 8) & 0xff);
	OV13850_write_i2c(0x3d89, start_addr & 0xff);
	
	OV13850_write_i2c(0x3d8A, (end_addr >> 8) & 0xff);
	OV13850_write_i2c(0x3d8B, end_addr & 0xff);
	
	OV13850_write_i2c(0x3d81, 0x01);
	Delay(5);
	temp = OV13850_read_i2c(start_addr + 4);
	(*otp_ptr).rg_ratio = (OV13850_read_i2c(start_addr)<<2) + ((temp>>6) & 0x03);
	
	(*otp_ptr).bg_ratio = (OV13850_read_i2c(start_addr + 1)<<2) + ((temp>>4) & 0x03);
	
	
	(*otp_ptr).typical_rg_ratio = (OV13850_read_i2c(start_addr+2)<<2) + ((temp>>2) & 0x03);
	
	
	(*otp_ptr).typical_bg_ratio = (OV13850_read_i2c(start_addr+3)<<2) + ((temp) & 0x03);

	
	rg_typical=(*otp_ptr).rg_ratio;
    bg_typical=(*otp_ptr).bg_ratio;
	OV13850DB("%s, rg_typical=%x\n",__func__,rg_typical);
    OV13850DB("%s, bg_typical=%x\n",__func__,bg_typical);

	
	for (i=start_addr; i<=end_addr; i++) {
		OV13850_write_i2c(i, 0x00);
	}
	
	temp1 = OV13850_read_i2c(0x5002);
	OV13850_write_i2c(0x5002, (0x02 & 0x02) | (temp1 & (~0x02)));
	return 0;
}
int read_otp_lenc(int index, struct otp_struct *otp_ptr)
{
	int i;
	int start_addr, end_addr;
	if (index == 1) {
		start_addr = 0x724B;
		end_addr = 0x7288;
	}
	else if (index == 2) {
		start_addr = 0x7289;
		end_addr = 0x72C6;
	}
	else if (index == 3) {
		start_addr = 0x72C7;
		end_addr = 0x7304;
	}
	
	int temp1;
	temp1 = OV13850_read_i2c(0x5002);
	OV13850_write_i2c(0x5002, (0x00 & 0x02) | (temp1 & (~0x02)));
	OV13850_write_i2c(0x3d84, 0xC0);
	
	OV13850_write_i2c(0x3d88, (start_addr >> 8) & 0xff);
	OV13850_write_i2c(0x3d89, start_addr & 0xff);
	
	OV13850_write_i2c(0x3d8A, (end_addr >> 8) & 0xff);
	OV13850_write_i2c(0x3d8B, end_addr & 0xff);
	
	OV13850_write_i2c(0x3d81, 0x01);
	Delay(10);
	for(i=0; i<62; i++) {
		(* otp_ptr).lenc[i] = OV13850_read_i2c((start_addr + i));
	}
	
	for (i=start_addr; i<=end_addr; i++) {
		OV13850_write_i2c(i, 0x00);
	}
	
	temp1 = OV13850_read_i2c(0x5002);
	OV13850_write_i2c(0x5002, (0x02 & 0x02) | (temp1 & (~0x02)));
	(* otp_ptr).lenc_status = 1;
	return 0;
}
int update_awb_gain(int R_gain, int G_gain, int B_gain)
{
	if (R_gain>0x400) {
		OV13850_write_i2c(0x5056, R_gain>>8);
		OV13850_write_i2c(0x5057, R_gain & 0x00ff);
	}
	if (G_gain>0x400) {
		OV13850_write_i2c(0x5058, G_gain>>8);
		OV13850_write_i2c(0x5059, G_gain & 0x00ff);
	}
	if (B_gain>0x400) {
		OV13850_write_i2c(0x505A, B_gain>>8);
		OV13850_write_i2c(0x505B, B_gain & 0x00ff);
	}
	return 0;
}
	
int update_lenc(struct otp_struct * otp_ptr)
{
	int i, temp;
	temp = OV13850_read_i2c(0x5000);
	temp = 0x01 | temp;
	OV13850_write_i2c(0x5000, temp);
	
	for(i=0;i<62 ;i++)
	{
		OV13850_write_i2c((0x5200 + i), (*otp_ptr).lenc[i]);
	}
	return 0;
}
int update_otp_wb()
{
	static struct otp_struct current_otp;
	int i;
	int otp_index;
	int temp;
	int rg,bg;
	int nR_G_gain, nB_G_gain, nG_G_gain;
	int R_gain, G_gain, B_gain;

	int nBase_gain;
	if (current_otp.awb_status != 1)
	{
		
		
		for(i=1;i<=3;i++) {
			temp = check_otp_wb(i);
			if (temp == 2) {
				otp_index = i;
				break;
			}
		}
		if (i>3) {
			
			return 1;
		}

		read_otp_wb(otp_index, &current_otp);
		
		
		
		
	
		if(current_otp.typical_rg_ratio==0) {
			
			
			rg = current_otp.rg_ratio;
		}
		else {
			rg = current_otp.rg_ratio * (current_otp.typical_rg_ratio +512) / 1024;
		}
	
		if(current_otp.typical_bg_ratio==0) {
			
			
			bg = current_otp.bg_ratio;
		}
		else {
			bg = current_otp.bg_ratio * (current_otp.typical_bg_ratio +512) / 1024;
		}
	
	
		
		

		OV13850DB("%s,before update ,RG = %x\n",__func__,rg);
		OV13850DB("%s,before update ,BG = %x\n",__func__,bg);

		
		
		nR_G_gain = (RG_Ratio_Typical*1000) / rg;
		nB_G_gain = (BG_Ratio_Typical*1000) / bg;
		nG_G_gain = 1000;
		if (nR_G_gain < 1000 || nB_G_gain < 1000)
		{
			if (nR_G_gain < nB_G_gain)
				nBase_gain = nR_G_gain;
			else
				nBase_gain = nB_G_gain;
		}
		else
		{
			nBase_gain = nG_G_gain;
		}
	
		R_gain = 0x400 * nR_G_gain / (nBase_gain);
		B_gain = 0x400 * nB_G_gain / (nBase_gain);
		G_gain = 0x400 * nG_G_gain / (nBase_gain);

		OV13850DB("%s,RG_Ratio_Typical = %x,BG_Ratio_Typical = %x\n",__func__,RG_Ratio_Typical,BG_Ratio_Typical);
		OV13850DB("%s,after calc, R_gain = %x,B_gain = %x,G_gain = %x\n",__func__,R_gain,B_gain,G_gain);
		current_otp.awb_status = 1;
	}
	update_awb_gain(R_gain, G_gain, B_gain);

	return 0;
}
int update_otp_lenc()
{
	static struct otp_struct current_otp;
	int i;
	int otp_index;
	int temp;
	OV13850DB("john %s start\n ",__func__);
    
	
	for(i=1;i<=3;i++) {
		temp = check_otp_lenc(i);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}
	if (i>3) {
		
		return 1;
	}
	if (current_otp.lenc_status != 1)
		read_otp_lenc(otp_index, &current_otp);
	update_lenc(&current_otp);
    
    OV13850DB("john %s end\n ",__func__);
	
	return 0;
}

int read_module_info(struct otp_struct *otp_ptr){
    return 0x10;
}

void otp_cali(unsigned char writeid){

}

