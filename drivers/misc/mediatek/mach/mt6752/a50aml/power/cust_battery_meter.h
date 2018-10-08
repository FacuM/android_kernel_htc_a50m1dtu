#ifndef _CUST_BATTERY_METER_H
#define _CUST_BATTERY_METER_H

#include <mach/mt_typedefs.h>
#include <linux/htc_devices_dtb.h>

#define SOC_BY_HW_FG


#if 0
#define CUST_TABT_NUMBER 17
#define VBAT_CHANNEL_NUMBER      7
#define ISENSE_CHANNEL_NUMBER	 6
#define VCHARGER_CHANNEL_NUMBER  4
#define VBATTEMP_CHANNEL_NUMBER  5
#endif
#define R_BAT_SENSE 4					
#define R_I_SENSE 4						
#define R_CHARGER_1 330
#define R_CHARGER_2 39

#define TEMPERATURE_T0             110
#define TEMPERATURE_T1             0
#define TEMPERATURE_T2             25
#define TEMPERATURE_T3             50
#define TEMPERATURE_T              255 

#define FG_METER_RESISTANCE 	0

#define Q_MAX_POS_50	2545
#define Q_MAX_POS_25	2554
#define Q_MAX_POS_0	3386
#define Q_MAX_NEG_10	3996

#define Q_MAX_SPEC 2600   

#define Q_MAX_POS_50_H_CURRENT	2525
#define Q_MAX_POS_25_H_CURRENT	2533
#define Q_MAX_POS_0_H_CURRENT	2181
#define Q_MAX_NEG_10_H_CURRENT	1434


#define OAM_D5		 1		


#define CHANGE_TRACKING_POINT
#define CUST_TRACKING_POINT  1
#define CUST_R_SENSE         56 

#define CUST_HW_CC 		    0
#define AGING_TUNING_VALUE   103
#define CUST_R_FG_OFFSET    0

#define OCV_BOARD_COMPESATE	0 
#define R_FG_BOARD_BASE		1000
#define R_FG_BOARD_SLOPE	1000 
#define CAR_TUNE_VALUE		((of_machine_hwid() >= 2) ? 102 : 88)


#define CURRENT_DETECT_R_FG	10  
#define MinErrorOffset       1000
#define FG_VBAT_AVERAGE_SIZE 18
#define R_FG_VALUE 			10 

#define CUST_POWERON_DELTA_CAPACITY_TOLRANCE	40
#define CUST_POWERON_LOW_CAPACITY_TOLRANCE		5
#define CUST_POWERON_MAX_VBAT_TOLRANCE			90
#define CUST_POWERON_DELTA_VBAT_TOLRANCE		30

#ifdef CONFIG_MTK_DISABLE_POWER_ON_OFF_VOLTAGE_LIMITATION
#define FIXED_TBAT_25
#endif

#define VBAT_NORMAL_WAKEUP		3600		
#define VBAT_LOW_POWER_WAKEUP		3500		
#define NORMAL_WAKEUP_PERIOD		3600 		
#define LOW_POWER_WAKEUP_PERIOD		300		
#define CLOSE_POWEROFF_WAKEUP_PERIOD	30	

#define INIT_SOC_BY_SW_SOC
#define MTK_ENABLE_AGING_ALGORITHM	
#define MD_SLEEP_CURRENT_CHECK	
#define Q_MAX_BY_CURRENT		

#define DISABLE_RFG_EXIST_CHECK
#endif	
