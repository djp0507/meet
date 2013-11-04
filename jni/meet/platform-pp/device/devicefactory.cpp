#define LOG_TAG "DeviceFactory"
#include "PPPlatForm.h"

#include "device/samsung_gti9000.h"
#include "device/samsung_gti9020.h"
#include "device/samsung_gti9220.h"
#include "device/motorola_mx525.h"
#include "device/htc_htca510e.h"
#include "device/htc_htcx515d.h"
#include "device/htc_htcz710t.h"
#include "device/htc_htcz710e.h"
#include "device/lge_optimus2x.h"
#include "device/xiaomi_mioneplus.h"
#include "device/huawei_u8800.h"
#include "device/samsung_gts5660.h"
//#include "device/generic_device.h"
//#include "device/samsung_gti9100.h"
//#include "device/motorola_mx722.h"
//#include "device/motorola_mx860.h"
//#include "device/htc_htcdesirez.h"
//#include "device/htc_htcincredibles2.h"
//#include "device/htc_htcsensationxl.h"
//#include "device/meizu_m9.h"
//#include "device/teclast_p76ti.h"

#include "include-pp/PlatformInfo.h"

namespace android {

GET_AUDIOTRACK_FUN CREATE_AUDIOTRACK_FUN = NULL;

extern "C" IDevice* createDevice(
	GET_AUDIOTRACK_FUN createAudioTrackFun,
	PlatformInfo* platformInfo) {
	
    CREATE_AUDIOTRACK_FUN = createAudioTrackFun;
	
	LOGD("get internal Device() start");

	if (platformInfo == NULL) {
		LOGE("platformInfo is NULL!!!");
		return NULL;
	}

	// Model name
	if(!strncmp(platformInfo->model_name, "MI-ONE", strlen("MI-ONE"))) //MI-ONE Plus, MI-ONE C1
	{
		//audio track api is special, keep here.
	    return new Xiaomi_MIONEPlus();
	}
    else if(!strncmp(platformInfo->model_name, "HTC A510e", strlen("HTC A510e"))) 
    {
		//color format is special. keep here.
		return new HTC_HTCA510e();
    }
    else if(!strncmp(platformInfo->model_name, "HTC X515d", strlen("HTC X515d")))
	{
	    return new HTC_HTCX515D();
    }
    else if(!strncmp(platformInfo->model_name, "GT-S5660", strlen("GT-S5660")))
	{
	    return new Samsung_GTS5660();
    }
	//else if(!strncmp(platformInfo->model_name, "GT-I9000", strlen("GT-I9000")))
	//{
	//    return new Samsung_GTi9000();
	//}
	//else if(!strncmp(platformInfo->model_name, "GT-I9020", strlen("GT-I9020")))
	//{
	//    return new Samsung_GTi9020();
	//}
	//else if(!strncmp(platformInfo->model_name, "GT-I9220", strlen("GT-I9220")))
	//{
	//    return new Samsung_GTi9220();
	//}
    //else if(!strncmp(platformInfo->model_name, "MB525", strlen("MB525")) ||
	//		!strncmp(platformInfo->model_name, "ME525", strlen("ME525")) )
	//{
	//    return new Motorola_MX525();
	//}
    //else if(!strncmp(platformInfo->model_name, "MB722", strlen("MB722")) ||
	//		!strncmp(platformInfo->model_name, "ME722", strlen("ME722")) )
	//{
	//    return new Motorola_MX722();
	//}
    //else if(!strncmp(platformInfo->model_name, "Optimus 2X", strlen("Optimus 2X")))
	//{
	//    return new LGE_Optimus2X();
	//}
    //else if(!strncmp(platformInfo->model_name, "U8800", strlen("U8800")))
	//{
	//    return new HUAWEI_U8800();
	//}
    //else if(!strncmp(platformInfo->model_name, "HTC Z710t", strlen("HTC Z710t")))
	//{
	//    return new HTC_HTCZ710T();
	//}
    //else if(!strncmp(platformInfo->model_name, "HTC Z710e", strlen("HTC Z710e")))
	//{
	//    return new HTC_HTCZ710E();
	//}
    //else if(!strncmp(platformInfo->model_name, "HTC Incredible S", strlen("HTC Incredible S")))
	//{
	//    return new HTC_HTCIncredibleS2();
    //}
	
	
	//Board name
	//else if(!strncmp(platformInfo->board_name, "msm7x30", strlen("msm7x30")))
	//{
	//	return NULL;
	    //return new HUAWEI_U8800();
    //}
	else if(!strncmp(platformInfo->board_name, "msm7k", strlen("msm7k")))
	{
	    return new HUAWEI_U8800();
    }
	else if(!strncmp(platformInfo->board_name, "pydtd", strlen("pydtd")))
	{
	    return new HTC_HTCZ710T();
    }
	else if(!strncmp(platformInfo->board_name, "s5pc110", strlen("s5pc110")))
	{
	    return new Samsung_GTi9020();
    }
	else if(!strncmp(platformInfo->board_name, "s5pv210", strlen("s5pv210")))
	{
	    return new Samsung_GTi9000();
    }
	else if(!strncmp(platformInfo->board_name, "s5pc210", strlen("s5pc210")))
	{
	    return new Samsung_GTi9220();
    }
	else if(!strncmp(platformInfo->board_name, "omap3", strlen("omap3")))
	{
	    return new Motorola_MX525();
    }
	else if(!strncmp(platformInfo->board_name, "tegra", strlen("tegra")))
	{
	    return new LGE_Optimus2X();
    }
	else if(!strncmp(platformInfo->board_name, "msm8660", strlen("msm8660")))
	{
	    return new HTC_HTCZ710E();
    }
	
    //else if(!strncmp(BOARD_NAME, "exDroid", strlen("exDroid")))
	//{
	//    return new Teclast_P76ti();
    //}
    
	/*
    if(!strncmp(MODEL_NAME, "P76TI", strlen("P76TI")))
	{
	    return new Teclast_P76ti();
    }
	else if(!strncmp(MODEL_NAME, "GT-I9100", strlen()))
	{
	    return NULL;// new Samsung_GTi9100();
	}
    else if(!strncmp(MODEL_NAME, "HTC Incredible", 14) ||
			!strncmp(MODEL_NAME, "HTC Desire S", 12)) //"HTC Incredible S"
	{
	    return NULL;// new HTC_HTCIncredibleS();
    }
    else if(!strncmp(MODEL_NAME, "HTC Sensation XL", 16))
	{
	    return NULL;// new HTC_HTCSensationXL();
    }
    else if(!strncmp(MODEL_NAME, "HTC Desire", 10) ||
			!strncmp(MODEL_NAME, "HTC Vision", 10) )
	{
	    return NULL;// new HTC_HTCDesireZ();
    }
    else if(!strncmp(MODEL_NAME, "HTC Z710t", 9) ||
			!strncmp(MODEL_NAME, "HTC Z710e", 9) )
	{
	    return NULL;// new HTC_HTCZ710T();
	}
    else if(!strncmp(MODEL_NAME, "MB860", 5) ||
			!strncmp(MODEL_NAME, "ME860", 5) )
	{
	    return NULL;// new Motorola_MX860();
	}
	else if(!strncmp(MODEL_NAME, "M9", 2))
	{
	    return NULL;// new Meizu_M9();
    }
    */
	else
	{
	    return NULL;// new Generic_Device();
	}
}

}
