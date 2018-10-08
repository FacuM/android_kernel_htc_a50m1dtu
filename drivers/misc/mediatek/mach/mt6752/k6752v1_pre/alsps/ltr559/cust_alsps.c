#include <linux/types.h>
#include <mach/mt_pm_ldo.h>
#include <cust_alsps.h>

static struct alsps_hw cust_alsps_hw = {
    .i2c_num    = 1,
    .polling_mode_ps = 0,   
    .polling_mode_als =1,
    .power_id   = MT65XX_POWER_NONE,    
    .power_vol  = VOL_DEFAULT,          
    .i2c_addr   = {0x72, 0x48, 0x78, 0x00},
     .als_level  =   {3, 6, 9, 15, 20, 35, 55, 80, 100, 360, 609, 913, 1490,  2019,  3500,  4605,  7043, 11000,  208040},	
     .als_value  =  {10, 10, 10, 60, 80, 100, 150, 200, 250, 360, 3500, 3500, 3500,  3500,  3500,  3500,  3500,  3500,   10000, 10000}, 
    .ps_threshold_high = 500,  
    .ps_threshold_low = 200,  
    .ps_threshold = 900,
};
struct alsps_hw *get_cust_alsps_hw(void) {
    return &cust_alsps_hw;
}

