#define LOG_TAG "DeviceFactory_CM"
#include "PPPlatForm.h"

#include "device/htc_htcincredibles.h"
#include "device/htc_htcz510d.h"
#include "device/generic_device.h"
//#include "device/samsung_gti9000.h"
//#include "device/samsung_gti9100.h"
//#include "device/samsung_gti9220.h"
//#include "device/motorola_mx525.h"
//#include "device/motorola_mx722.h"
//#include "device/motorola_mx860.h"
//#include "device/htc_htcdesirez.h"
//#include "device/htc_htcdesirez.h"
//#include "device/htc_htca510e.h"
//#include "device/htc_htcx515d.h"
//#include "device/htc_htcincredibles2.h"
//#include "device/htc_htcsensationxl.h"
//#include "device/htc_htcz710t.h"
//#include "device/htc_htcz710e.h"
//#include "device/lge_optimus2x.h"
//#include "device/xiaomi_mioneplus.h"
//#include "device/huawei_u8800.h"
//#include "device/meizu_m9.h"
//#include "device/teclast_p76ti.h"
//#include "device/samsung_gts5660.h"
//#include "device/samsung_gts5820.h"
//#include "device/huawei_hi3716c.h"

#include "include-pp/PlatformInfo.h"

namespace android {

//const char *MODEL_NAME;
//const char *BOARD_NAME;
//const char *MANUFACTURE_NAME;
GET_AUDIOTRACK_FUN CREATE_AUDIOTRACK_FUN = NULL;

extern "C" IDevice* createDevice(
	GET_AUDIOTRACK_FUN createAudioTrackFun,
	PlatformInfo* platformInfo)
{
    CREATE_AUDIOTRACK_FUN = createAudioTrackFun;
	
	LOGD("get cm Device() start");

	if (platformInfo == NULL) {
		LOGE("platformInfo is NULL!!!");
		return NULL;
	}
	
	//Model name
	if(!strncmp(platformInfo->model_name, "GT-S5820", strlen("GT-S5820")))
    {
		return NULL;
    } else if (!strncmp(platformInfo->model_name, "ZTE-T U880", strlen("ZTE-T U880")))
    {
    	return NULL;
    }
	//else if(!strncmp(platformInfo->model_name, "M9", strlen("M9")))
	//{
	//    return new Meizu_M9();
    //}
    //else if(!strncmp(platformInfo->model_name, "HTC Sensation XL", strlen("HTC Sensation XL")))
	//{
	//    return new HTC_HTCSensationXL();
    //}
    //else if(!strncmp(platformInfo->model_name, "HTC Z510d", strlen("HTC Z510d")))
	//{
	//    return new HTC_HTCZ510D();
	//}
	
	/*
    else if(!strncmp(MODEL_NAME, "HTC Desire S", strlen("HTC Desire S")))
	{
	    return new HTC_HTCIncredibleS();
    }
    else if(!strncmp(MODEL_NAME, "HTC Desire", strlen("HTC Desire")) ||
			!strncmp(MODEL_NAME, "HTC Vision", strlen("HTC Vision")) )
	{
	    return new HTC_HTCDesireZ();
    }
    else if(!strncmp(MODEL_NAME, "MB860", strlen("MB860")) ||
			!strncmp(MODEL_NAME, "ME860", strlen("ME860")) )
	{
	    return new Motorola_MX860();
	}
	else if(!strncmp(MODEL_NAME, "A1_07", strlen("A1_07")))
	{
	    return new HTC_HTCIncredibleS();
    }
    */

	// Board name
	/*else if(!strncmp(platformInfo->board_name, "", strlen("") &&
			!strncmp(platformInfo->chip_name, "MARVELL_PXA920", strlen("MARVELL_PXA920"))))
	{
		return new Samsung_GTS5820();
	}*/
	else if(!strncmp(platformInfo->board_name, "msm7x30", strlen("msm7x30")))
	{
		return new HTC_HTCIncredibleS(); //internal codec does not support mLivesLocally=false
    }
	else if(!strncmp(platformInfo->board_name, "qsd8k", strlen("qsd8k")))
	{
	    //got some error, like
	    //QCvdec  : get_h264_nal_type - ERROR at extract_rbsp
	    //OMXCodec_cm: [OMX.qcom.video.decoder.avc] ERROR(0x8000100a, 0)
	    //so turn it off
	    //return new HTC_HTCZ510D();
	    return NULL;
    }
    else if(!strncmp(platformInfo->board_name, "omap4", strlen("omap4")))
	{
	    return NULL;//This has been verified incompatible.
    }
	
	//Manufacture name
	/*
    else if(!strncmp(platformInfo->manufacture_name, "huawei", strlen("huawei")))
	{
	    return new HUAWEI_U8800();
    }
    else if(!strncmp(platformInfo->manufacture_name, "HTC", strlen("HTC")))
	{
	    return new HTC_HTCZ710E();
    }
    else if(!strncmp(platformInfo->manufacture_name, "motorola", strlen("motorola")))
	{
	    return new Motorola_MX525();
    }
    else if(!strncmp(platformInfo->manufacture_name, "Meizu", strlen("Meizu")))
    {
		return new Meizu_M9();
    }
    else if(!strncmp(platformInfo->manufacture_name, "samsung", strlen("samsung")))
	{
	    return new Samsung_GTi9000();
    }
    else if(!strncmp(platformInfo->manufacture_name, "Xiaomi", strlen("Xiaomi")))
	{
	    return new Xiaomi_MIONEPlus();
    }
    */
	else
	{
	    return new Generic_Device();
	}
}

}
