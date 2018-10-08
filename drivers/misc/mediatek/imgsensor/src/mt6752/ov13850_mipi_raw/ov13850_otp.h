
#define OTP_DRV_LSC_SIZE 186

struct otp_struct {
	int module_integrator_id;
	int lens_id;
	int production_year;
	int production_month;
	int production_day;
	int rg_ratio;
	int bg_ratio;
	int light_rg;
	int light_bg;
	int typical_rg_ratio;
	int typical_bg_ratio;
	int lenc[OTP_DRV_LSC_SIZE ];
	int awb_status;
	int lenc_status;
	int VCM_start;
	int VCM_end;
	int VCM_dir;
};

static int RG_Ratio_Typical = 0x13C;
static int BG_Ratio_Typical = 0x159;


extern int read_otp_info(int index, struct otp_struct *otp_ptr);
extern int update_otp_wb(void);
extern int update_otp_lenc(void);







