#ifndef _CUST_BATTERY_METER_TABLE_H
#define _CUST_BATTERY_METER_TABLE_H
#include "battery_param.h"

#include <generated/autoconf.h>
#include <mach/mt_typedefs.h>
#ifdef CONFIG_MTK_BATT_TEMP_NOT_READY
#define BAT_NTC_100 0
#define BAT_NTC_10 0
#define BAT_NTC_47 0
#define BAT_NTC_PORTING 1
#else
#define BAT_NTC_100 1
#define BAT_NTC_10 0
#define BAT_NTC_47 0
#define BAT_NTC_PORTING 0
#endif

#if (BAT_NTC_100 == 1)
#define RBAT_PULL_UP_R             100000
#define TABLE_SIZE             100
#define LOWEST_TEMP             -20
#define HIGHEST_TEMP             80
#endif

#if (BAT_NTC_10 == 1)
#define RBAT_PULL_UP_R             24000	
#define TABLE_SIZE             16
#define LOWEST_TEMP             -20
#define HIGHEST_TEMP             60
#endif

#if (BAT_NTC_47 == 1)
#define RBAT_PULL_UP_R             61900	
#define TABLE_SIZE             16
#define LOWEST_TEMP             -20
#define HIGHEST_TEMP             60
#endif

#if (BAT_NTC_PORTING == 1)
#define RBAT_PULL_UP_R             61900
#define TABLE_SIZE             1
#define LOWEST_TEMP             25
#define HIGHEST_TEMP             25
#endif

#define RBAT_PULL_UP_VOLT          2800





typedef struct _BATTERY_PROFILE_STRUC
{
    kal_int32 percentage;
    kal_int32 voltage;
} BATTERY_PROFILE_STRUC, *BATTERY_PROFILE_STRUC_P;

typedef struct _R_PROFILE_STRUC
{
    kal_int32 resistance; 
    kal_int32 voltage;
} R_PROFILE_STRUC, *R_PROFILE_STRUC_P;

typedef enum
{
    T1_0C,
    T2_25C,
    T3_50C
} PROFILE_TEMPERATURE;




#define MTK_MULTI_BAT_PROFILE_SUPPORT
#define MTK_GET_BATTERY_ID_BY_AUXADC
#define BATTERY_ID_CHANNEL_NUM 12
kal_int32 g_battery_id_voltage[] = {250, 400, -1};
#define TOTAL_BATTERY_NUMBER (sizeof(g_battery_id_voltage) / sizeof(kal_int32))

kal_int32 g_Q_MAX_POS_50[] = {2590, 2560, 2590};
kal_int32 g_Q_MAX_POS_25[] = {2548, 2593, 2548};
kal_int32 g_Q_MAX_POS_0[] = {2035, 2593, 2035};
kal_int32 g_Q_MAX_NEG_10[] = {1150, 2578, 1150};
kal_int32 g_Q_MAX_POS_50_H_CURRENT[] = {2590, 2536, 2590};
kal_int32 g_Q_MAX_POS_25_H_CURRENT[] = {2548, 2576, 2548};
kal_int32 g_Q_MAX_POS_0_H_CURRENT[] = {2035, 2193, 2035};
kal_int32 g_Q_MAX_NEG_10_H_CURRENT[] = {1150, 1536, 1150};

#if (BAT_NTC_100 == 1)
	BATT_TEMPERATURE Batt_Temperature_Table[TOTAL_BATTERY_NUMBER][17] = {
			Batt0_Temperature_Table,
			Batt0_Temperature_Table,
			Batt0_Temperature_Table
	};
#endif

#if (BAT_NTC_10 == 1)
    BATT_TEMPERATURE Batt_Temperature_Table[TOTAL_BATTERY_NUMBER][17] = {
		{
	        {-20,68237}, 
	        {-15,53650},
	        {-10,42506},
	        { -5,33892},
	        {  0,27219},
	        {  5,22021},
	        { 10,17926},
	        { 15,14674},
	        { 20,12081},
	        { 25,10000},
	        { 30,8315},
	        { 35,6948},
	        { 40,5834},
	        { 45,4917},
	        { 50,4161},
	        { 55,3535},
	        { 60,3014}
		},
		{
			{-20,68237}, 
			{-15,53650},
			{-10,42506},
			{ -5,33892},
			{  0,27219},
			{  5,22021},
			{ 10,17926},
			{ 15,14674},
			{ 20,12081},
			{ 25,10000},
			{ 30,8315},
			{ 35,6948},
			{ 40,5834},
			{ 45,4917},
			{ 50,4161},
			{ 55,3535},
			{ 60,3014}
		},
		{
			{-20,68237}, 
			{-15,53650},
			{-10,42506},
			{ -5,33892},
			{  0,27219},
			{  5,22021},
			{ 10,17926},
			{ 15,14674},
			{ 20,12081},
			{ 25,10000},
			{ 30,8315},
			{ 35,6948},
			{ 40,5834},
			{ 45,4917},
			{ 50,4161},
			{ 55,3535},
			{ 60,3014}
		}
    };
#endif

#if (BAT_NTC_47 == 1)
    BATT_TEMPERATURE Batt_Temperature_Table[TOTAL_BATTERY_NUMBER][17] = {
		{
	        {-20,483954}, 
	        {-15,360850},
	        {-10,271697},
	        { -5,206463},
	        {  0,158214},
	        {  5,122259},
	        { 10,95227},
	        { 15,74730},
	        { 20,59065},
	        { 25,47000},
	        { 30,37643},
	        { 35,30334},
	        { 40,24591},
	        { 45,20048},
	        { 50,16433},
	        { 55,13539},
	        { 60,11210}
		},
		{
			{-20,483954}, 
			{-15,360850},
	        {-10,271697},
	        { -5,206463},
	        {  0,158214},
	        {  5,122259},
	        { 10,95227},
	        { 15,74730},
	        { 20,59065},
	        { 25,47000},
	        { 30,37643},
	        { 35,30334},
	        { 40,24591},
	        { 45,20048},
	        { 50,16433},
	        { 55,13539},
	        { 60,11210}
		},
		{
			{-20,483954}, 
			{-15,360850},
	        {-10,271697},
	        { -5,206463},
	        {  0,158214},
	        {  5,122259},
	        { 10,95227},
	        { 15,74730},
	        { 20,59065},
	        { 25,47000},
	        { 30,37643},
	        { 35,30334},
	        { 40,24591},
	        { 45,20048},
	        { 50,16433},
	        { 55,13539},
	        { 60,11210}
		}
    };
#endif

#if (BAT_NTC_PORTING == 1)
    BATT_TEMPERATURE Batt_Temperature_Table[] = {
        {48,999999},
        {1,0}
    };
#endif

BATTERY_PROFILE_STRUC battery_profile_t0[TOTAL_BATTERY_NUMBER][80] = {
	battery0_profile_t0,
	battery1_profile_t0,
	battery0_profile_t0
};

BATTERY_PROFILE_STRUC battery_profile_t1[TOTAL_BATTERY_NUMBER][80] = {
	battery0_profile_t1,
	battery1_profile_t1,
	battery0_profile_t1
};

BATTERY_PROFILE_STRUC battery_profile_t2[TOTAL_BATTERY_NUMBER][80] = {
	battery0_profile_t2,
	battery1_profile_t2,
	battery0_profile_t2
};

BATTERY_PROFILE_STRUC battery_profile_t3[TOTAL_BATTERY_NUMBER][80] = {
	battery0_profile_t3,
	battery1_profile_t3,
	battery0_profile_t3
};

BATTERY_PROFILE_STRUC battery_profile_temperature[80] = VALUE_ZERO;

R_PROFILE_STRUC r_profile_t0[TOTAL_BATTERY_NUMBER][80] = {
	battery0_r_profile_t0,
	battery1_r_profile_t0,
	battery0_r_profile_t0
};

R_PROFILE_STRUC r_profile_t1[TOTAL_BATTERY_NUMBER][80] = {
	battery0_r_profile_t1,
	battery1_r_profile_t1,
	battery0_r_profile_t1
};

R_PROFILE_STRUC r_profile_t2[TOTAL_BATTERY_NUMBER][80] = {
	battery0_r_profile_t2,
	battery1_r_profile_t2,
	battery0_r_profile_t2
};

R_PROFILE_STRUC r_profile_t3[TOTAL_BATTERY_NUMBER][80] = {
	battery0_r_profile_t3,
	battery1_r_profile_t3,
	battery0_r_profile_t3
};

R_PROFILE_STRUC r_profile_temperature[80] = VALUE_ZERO;

int fgauge_get_saddles(void);
BATTERY_PROFILE_STRUC_P fgauge_get_profile(kal_uint32 temperature);

int fgauge_get_saddles_r_table(void);
R_PROFILE_STRUC_P fgauge_get_profile_r_table(kal_uint32 temperature);

#endif	

