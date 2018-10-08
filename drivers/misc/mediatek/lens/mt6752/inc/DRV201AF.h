#ifndef _DRV201AF_H
#define _DRV201AF_H

#include <linux/ioctl.h>

#define DRV201AF_MAGIC 'A'


typedef struct {
	u32 u4CurrentPosition;
	u32 u4MacroPosition;
	u32 u4InfPosition;
	bool bIsMotorMoving;
	bool bIsMotorOpen;
	bool bIsSupportSR;
} stDRV201AF_MotorInfo;


#ifdef LensdrvCM3
typedef struct {
	
	float Aperture;
	
	float FilterDensity;
	
	float FocalLength;
	
	float FocalDistance;
	
	u16 u4OIS_Mode;
	
	u16 Facing;
	
	float OpticalAxisAng[2];
	
	float Position[3];
	
	float FocusRange;
	
	u16 State;
	
	float InfoAvalibleMinFocusDistance;
	float InfoAvalibleApertures;
	float InfoAvalibleFilterDensity;
	u16 InfoAvalibleOptStabilization;
	float InfoAvalibleFocalLength;
	float InfoAvalibleHypeDistance;
} stDRV201AF_MotorMETAInfo;
#endif

#define DRV201AFIOC_G_MOTORINFO _IOR(DRV201AF_MAGIC, 0, stDRV201AF_MotorInfo)

#define DRV201AFIOC_T_MOVETO _IOW(DRV201AF_MAGIC, 1, u32)

#define DRV201AFIOC_T_SETINFPOS _IOW(DRV201AF_MAGIC, 2, u32)

#define DRV201AFIOC_T_SETMACROPOS _IOW(DRV201AF_MAGIC, 3, u32)
#ifdef LensdrvCM3
#define DRV201AFIOC_G_MOTORMETAINFO _IOR(DRV201AF_MAGIC, 4, stDRV201AF_MotorMETAInfo)
#endif

#else
#endif
