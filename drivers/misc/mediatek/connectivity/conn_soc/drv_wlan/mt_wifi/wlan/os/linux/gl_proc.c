





#include "gl_os.h"
#include "gl_kal.h"

#include "wlan_lib.h"
#include "debug.h"


#define PROC_MCR_ACCESS                         "mcr"
#define PROC_DRV_STATUS                         "status"
#define PROC_RX_STATISTICS                      "rx_statistics"
#define PROC_TX_STATISTICS                      "tx_statistics"
#define PROC_DBG_LEVEL_NAME             "dbgLevel"
#define PROC_ROOT_NAME			"wlan"


#define PROC_MCR_ACCESS_MAX_USER_INPUT_LEN      20
#define PROC_RX_STATISTICS_MAX_USER_INPUT_LEN   10
#define PROC_TX_STATISTICS_MAX_USER_INPUT_LEN   10
#define PROC_DBG_LEVEL_MAX_USER_INPUT_LEN       20
#define PROC_DBG_LEVEL_MAX_DISPLAY_STR_LEN      30
#define PROC_UID_SHELL							2000
#define PROC_GID_WIFI							1010



static UINT_32 u4McrOffset = 0;
static P_GLUE_INFO_T gprGlueInfo = NULL;
static struct proc_dir_entry *gprProcRoot;
static UINT_8 aucDbModuleName[][PROC_DBG_LEVEL_MAX_DISPLAY_STR_LEN] = {
	"INIT", "HAL", "INTR", "REQ", "TX", "RX", "RFTEST", "EMU", "SW1", "SW2",
	"SW3", "SW4", "HEM", "AIS", "RLM", "MEM", "CNM", "RSN", "BSS", "SCN",
	"SAA", "AAA", "P2P", "QM", "SEC", "BOW"
};
static UINT_8 aucDbgLevelBuff[1024];
extern UINT_8 aucDebugModule[];



static ssize_t
procMCRRead (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    PARAM_CUSTOM_MCR_RW_STRUC_T rMcrInfo;
    UINT_32 u4BufLen = count;
    char p[50] = {0,};
    char *temp = &p[0];
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;

    
    if (*f_pos > 0 || gprGlueInfo == NULL) {
        return 0; 
    }
 
    rMcrInfo.u4McrOffset = u4McrOffset;

    rStatus = kalIoctl(gprGlueInfo,
                        wlanoidQueryMcrRead,
                        (PVOID)&rMcrInfo,
                        sizeof(rMcrInfo),
                        TRUE,
                        TRUE,
                        TRUE,
                        FALSE,
                        &u4BufLen);


    SPRINTF(temp, ("MCR (0x%08xh): 0x%08x\n",
        rMcrInfo.u4McrOffset, rMcrInfo.u4McrData));

     u4BufLen = kalStrLen(p);
     if (u4BufLen > count)
	 u4BufLen = count;
    if (copy_to_user(buf, p, u4BufLen))
	return -EFAULT;
	
    *f_pos += u4BufLen;

    return (int)u4BufLen;

} 


static ssize_t
procMCRWrite (struct file *file, const char *buffer, unsigned long count, void *data)
{
    char acBuf[PROC_MCR_ACCESS_MAX_USER_INPUT_LEN + 1]; 
    int i4CopySize;
    PARAM_CUSTOM_MCR_RW_STRUC_T rMcrInfo;
    UINT_32 u4BufLen;
    WLAN_STATUS rStatus = WLAN_STATUS_SUCCESS;

    i4CopySize = (count < (sizeof(acBuf) - 1)) ? count : (sizeof(acBuf) - 1);
    if (copy_from_user(acBuf, buffer, i4CopySize) || gprGlueInfo == NULL) {
        return 0;
    }
    acBuf[i4CopySize] = '\0';

    switch (sscanf(acBuf, "0x%x 0x%x",
                   &rMcrInfo.u4McrOffset, &rMcrInfo.u4McrData)) {
    case 2:
        
        { 
            u4McrOffset = rMcrInfo.u4McrOffset;

            
                

            rStatus = kalIoctl(gprGlueInfo,
                                wlanoidSetMcrWrite,
                                (PVOID)&rMcrInfo,
                                sizeof(rMcrInfo),
                                FALSE,
                                FALSE,
                                TRUE,
                                FALSE,
                                &u4BufLen);

        }
        break;

    case 1:
        
        {
            u4McrOffset = rMcrInfo.u4McrOffset;
        }
        break;

    default:
        break;
    }

    return count;

} 

static const struct file_operations mcr_ops = {
	.owner = THIS_MODULE,
	.read = procMCRRead,
	.write = procMCRWrite,
};

#if 0
static int
procDrvStatusRead (
    char *page,
    char **start,
    off_t off,
    int count,
    int *eof,
    void *data
    )
{
    P_GLUE_INFO_T prGlueInfo = ((struct net_device *)data)->priv;
    char *p = page;
    UINT_32 u4Count;

    GLUE_SPIN_LOCK_DECLARATION();


    ASSERT(data);

    
    if (off != 0) {
        return 0; 
    }


    SPRINTF(p, ("GLUE LAYER STATUS:"));
    SPRINTF(p, ("\n=================="));

    SPRINTF(p, ("\n* Number of Pending Frames: %ld\n",
        prGlueInfo->u4TxPendingFrameNum));

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_FSM);

    wlanoidQueryDrvStatusForLinuxProc(prGlueInfo->prAdapter, p, &u4Count);

    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_FSM);

    u4Count += (UINT_32)(p - page);

    *eof = 1;

    return (int)u4Count;

} 


static int
procRxStatisticsRead (
    char *page,
    char **start,
    off_t off,
    int count,
    int *eof,
    void *data
    )
{
    P_GLUE_INFO_T prGlueInfo = ((struct net_device *)data)->priv;
    char *p = page;
    UINT_32 u4Count;

    GLUE_SPIN_LOCK_DECLARATION();


    ASSERT(data);

    
    if (off != 0) {
        return 0; 
    }


    SPRINTF(p, ("RX STATISTICS (Write 1 to clear):"));
    SPRINTF(p, ("\n=================================\n"));

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_FSM);

    wlanoidQueryRxStatisticsForLinuxProc(prGlueInfo->prAdapter, p, &u4Count);

    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_FSM);

    u4Count += (UINT_32)(p - page);

    *eof = 1;

    return (int)u4Count;

} 


static int
procRxStatisticsWrite (
    struct file *file,
    const char *buffer,
    unsigned long count,
    void *data
    )
{
    P_GLUE_INFO_T prGlueInfo = ((struct net_device *)data)->priv;
    char acBuf[PROC_RX_STATISTICS_MAX_USER_INPUT_LEN + 1]; 
    UINT_32 u4CopySize;
    UINT_32 u4ClearCounter;

    GLUE_SPIN_LOCK_DECLARATION();


    ASSERT(data);

    u4CopySize = (count < (sizeof(acBuf) - 1)) ? count : (sizeof(acBuf) - 1);
    copy_from_user(acBuf, buffer, u4CopySize);
    acBuf[u4CopySize] = '\0';

    if (sscanf(acBuf, "%ld", &u4ClearCounter) == 1) {
        if (u4ClearCounter == 1) {
            GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_FSM);

            wlanoidSetRxStatisticsForLinuxProc(prGlueInfo->prAdapter);

            GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_FSM);
        }
    }

    return count;

} 


static int
procTxStatisticsRead (
    char *page,
    char **start,
    off_t off,
    int count,
    int *eof,
    void *data
    )
{
    P_GLUE_INFO_T prGlueInfo = ((struct net_device *)data)->priv;
    char *p = page;
    UINT_32 u4Count;

    GLUE_SPIN_LOCK_DECLARATION();


    ASSERT(data);

    
    if (off != 0) {
        return 0; 
    }


    SPRINTF(p, ("TX STATISTICS (Write 1 to clear):"));
    SPRINTF(p, ("\n=================================\n"));

    GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_FSM);

    wlanoidQueryTxStatisticsForLinuxProc(prGlueInfo->prAdapter, p, &u4Count);

    GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_FSM);

    u4Count += (UINT_32)(p - page);

    *eof = 1;

    return (int)u4Count;

} 


static int
procTxStatisticsWrite (
    struct file *file,
    const char *buffer,
    unsigned long count,
    void *data
    )
{
    P_GLUE_INFO_T prGlueInfo = ((struct net_device *)data)->priv;
    char acBuf[PROC_RX_STATISTICS_MAX_USER_INPUT_LEN + 1]; 
    UINT_32 u4CopySize;
    UINT_32 u4ClearCounter;

    GLUE_SPIN_LOCK_DECLARATION();


    ASSERT(data);

    u4CopySize = (count < (sizeof(acBuf) - 1)) ? count : (sizeof(acBuf) - 1);
    copy_from_user(acBuf, buffer, u4CopySize);
    acBuf[u4CopySize] = '\0';

    if (sscanf(acBuf, "%ld", &u4ClearCounter) == 1) {
        if (u4ClearCounter == 1) {
            GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_FSM);

            wlanoidSetTxStatisticsForLinuxProc(prGlueInfo->prAdapter);

            GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_FSM);
        }
    }

    return count;

} 
#endif

static ssize_t procDbgLevelRead(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	UINT_8 *temp = &aucDbgLevelBuff[0];
	UINT_32 u4CopySize = 0;
	UINT_32 i;

	
	if (*f_pos > 0) {
		return 0;
	}
	SPRINTF(temp, ("\nERROR |WARN |STATE |EVENT |TRACE |INFO |LOUD |TEMP\n"
					"bit0 | bit1 | bit2 | bit3 | bit4 | bit5 |bit6 |bit7\n\nDebug Module\tIndex\tLevel\n\n"));
	for (i = 0; i < (sizeof(aucDbModuleName) / PROC_DBG_LEVEL_MAX_DISPLAY_STR_LEN); i++) {
		SPRINTF(temp, ("DBG_%s_IDX\t(0x%02x):\t0x%02x\n", &aucDbModuleName[i][0], i, aucDebugModule[i]));
	}
	u4CopySize = kalStrLen(aucDbgLevelBuff);
	if (u4CopySize > count)
		u4CopySize = count;
	if (copy_to_user(buf, aucDbgLevelBuff, u4CopySize) ) {
		pr_err("copy to user failed\n");
		return -EFAULT;
	}
	
	*f_pos += u4CopySize;
	return (ssize_t)u4CopySize;
}

static ssize_t procDbgLevelWrite(struct file *file, const char *buffer, unsigned long count, void *data)
{
	UINT_32 u4NewDbgModule, u4NewDbgLevel;
	UINT_8 i = 0;
	UINT_32 u4CopySize = sizeof(aucDbgLevelBuff);
	UINT_8 *temp = &aucDbgLevelBuff[0];

	kalMemSet(aucDbgLevelBuff, 0, u4CopySize);
	if (u4CopySize >= count+1)
		u4CopySize = count;

	if (copy_from_user(aucDbgLevelBuff, buffer, u4CopySize)) {
		pr_err("error of copy from user\n");
		return -EFAULT;
	}
	aucDbgLevelBuff[u4CopySize] = '\0';

	while (temp) {
		if (sscanf(temp, "0x%x:0x%x", &u4NewDbgModule, &u4NewDbgLevel) != 2)  {
			pr_info("debug module and debug level should be one byte in length\n");
			break;
		}
		if (u4NewDbgModule >= DBG_MODULE_NUM) {
			pr_info("debug module index should less than %d\n", DBG_MODULE_NUM);
			break;
		}
		aucDebugModule[u4NewDbgModule] =  u4NewDbgLevel & DBG_CLASS_MASK;
		temp = kalStrChr(temp, ',');
		if (!temp)
			break;
		temp++; 
	}
	return count;
}

static const struct file_operations dbglevel_ops = {
	.owner = THIS_MODULE,
	.read = procDbgLevelRead,
	.write = procDbgLevelWrite,
};

INT_32 procInitFs()
{
	struct proc_dir_entry *prEntry;

	if (init_net.proc_net == (struct proc_dir_entry *)NULL) {
		pr_err("init proc fs fail: proc_net == NULL\n");
		return -ENOENT;
	}


	gprProcRoot = proc_mkdir(PROC_ROOT_NAME, init_net.proc_net);
	if (!gprProcRoot) {
		pr_err("gprProcRoot == NULL\n");
		return -ENOENT;
	}
	proc_set_user(gprProcRoot, KUIDT_INIT(PROC_UID_SHELL), KGIDT_INIT(PROC_GID_WIFI));
	
	prEntry = proc_create(PROC_DBG_LEVEL_NAME, 0664, gprProcRoot, &dbglevel_ops);
	if (prEntry == NULL) {
		pr_err("Unable to create /proc entry dbgLevel\n\r");
		return -1;
	}
	proc_set_user(prEntry, KUIDT_INIT(PROC_UID_SHELL), KGIDT_INIT(PROC_GID_WIFI));
	return 0;
}				

INT_32 procUninitProcFs()
{
	remove_proc_entry(PROC_DBG_LEVEL_NAME, gprProcRoot);
	remove_proc_subtree(PROC_ROOT_NAME, init_net.proc_net);
}

INT_32 procRemoveProcfs()
{
	
	
	remove_proc_entry(PROC_MCR_ACCESS, gprProcRoot);
	gprGlueInfo = NULL;
	return 0;
}				

INT_32 procCreateFsEntry(P_GLUE_INFO_T prGlueInfo)
{
	struct proc_dir_entry *prEntry;

	prGlueInfo->pProcRoot = gprProcRoot;
	gprGlueInfo = prGlueInfo;
	prEntry = proc_create(PROC_MCR_ACCESS, 0, gprProcRoot, &mcr_ops);
	if (prEntry == NULL) {
		printk("Unable to create /proc entry\n\r");
		return -1;
	}

	return 0;
}

