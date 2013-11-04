LOCAL_PATH := $(call my-dir)

JNI_BASE := meet/jni
MEDIA_BASE := meet/libmedia
PLAYER_BASE := meet/libppplayer
OS_BASE := meet/platform-pp/os
DEVICE_BASE := meet/platform-pp/device
PLATFORM_BASE := meet/platform-pp
PREBUILT_BASE := meet/prebuilt

UTILS_BASE := meet/libutils
CUTILS_BASE := meet/libcutils
LOG_BASE := meet/liblog

ENGINE_BASE := ../../engine

########################[liblog]########################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(LOG_BASE)/logd_write.c \
	$(LOG_BASE)/logprint.c \
	$(LOG_BASE)/event_tag_map.c \
	$(LOG_BASE)/fake_log_device.c

LOCAL_C_INCLUDES := \
	meet

LOCAL_MODULE := log

include $(BUILD_STATIC_LIBRARY)

########################[liblog_a14]########################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(LOG_BASE)/a14/logd_write.c \
	$(LOG_BASE)/a14/logprint.c \
	$(LOG_BASE)/a14/event_tag_map.c \
	$(LOG_BASE)/a14/fake_log_device.c

LOCAL_C_INCLUDES := \
	meet

LOCAL_MODULE := log_a14

include $(BUILD_STATIC_LIBRARY)

########################[libcutils]########################
include $(CLEAR_VARS)

commonSources := \
	$(CUTILS_BASE)/array.c \
	$(CUTILS_BASE)/hashmap.c \
	$(CUTILS_BASE)/atomic.c.arm \
	$(CUTILS_BASE)/native_handle.c \
	$(CUTILS_BASE)/buffer.c \
	$(CUTILS_BASE)/socket_inaddr_any_server.c \
	$(CUTILS_BASE)/socket_local_client.c \
	$(CUTILS_BASE)/socket_local_server.c \
	$(CUTILS_BASE)/socket_loopback_client.c \
	$(CUTILS_BASE)/socket_loopback_server.c \
	$(CUTILS_BASE)/socket_network_client.c \
	$(CUTILS_BASE)/config_utils.c \
	$(CUTILS_BASE)/cpu_info.c \
	$(CUTILS_BASE)/load_file.c \
	$(CUTILS_BASE)/open_memstream.c \
	$(CUTILS_BASE)/strdup16to8.c \
	$(CUTILS_BASE)/strdup8to16.c \
	$(CUTILS_BASE)/record_stream.c \
	$(CUTILS_BASE)/process_name.c \
	$(CUTILS_BASE)/properties.c \
	$(CUTILS_BASE)/threads.c \
	$(CUTILS_BASE)/sched_policy.c \
	$(CUTILS_BASE)/iosched_policy.c

commonSources += \
	$(CUTILS_BASE)/abort_socket.c \
	$(CUTILS_BASE)/mspace.c \
	$(CUTILS_BASE)/selector.c \
	$(CUTILS_BASE)/tztime.c \
	$(CUTILS_BASE)/zygote.c

targetSources += $(CUTILS_BASE)/arch-arm/memset32.S

LOCAL_SRC_FILES := $(commonSources) $(targetSources)

LOCAL_C_INCLUDES := \
	meet

LOCAL_STATIC_LIBRARIES := \
	liblog \

LOCAL_MODULE := cutils

include $(BUILD_STATIC_LIBRARY)

########################[libcutils_a14]########################
include $(CLEAR_VARS)

commonSources := \
	$(CUTILS_BASE)/a14/array.c \
	$(CUTILS_BASE)/a14/hashmap.c \
	$(CUTILS_BASE)/a14/atomic.c.arm \
	$(CUTILS_BASE)/a14/native_handle.c \
	$(CUTILS_BASE)/a14/buffer.c \
	$(CUTILS_BASE)/a14/socket_inaddr_any_server.c \
	$(CUTILS_BASE)/a14/socket_local_client.c \
	$(CUTILS_BASE)/a14/socket_local_server.c \
	$(CUTILS_BASE)/a14/socket_loopback_client.c \
	$(CUTILS_BASE)/a14/socket_loopback_server.c \
	$(CUTILS_BASE)/a14/socket_network_client.c \
	$(CUTILS_BASE)/a14/config_utils.c \
	$(CUTILS_BASE)/a14/cpu_info.c \
	$(CUTILS_BASE)/a14/load_file.c \
	$(CUTILS_BASE)/a14/open_memstream.c \
	$(CUTILS_BASE)/a14/strdup16to8.c \
	$(CUTILS_BASE)/a14/strdup8to16.c \
	$(CUTILS_BASE)/a14/record_stream.c \
	$(CUTILS_BASE)/a14/process_name.c \
	$(CUTILS_BASE)/a14/properties.c \
	$(CUTILS_BASE)/a14/threads.c \
	$(CUTILS_BASE)/a14/sched_policy.c \
	$(CUTILS_BASE)/a14/iosched_policy.c \
	$(CUTILS_BASE)/a14/system_properties.c

commonSources += \
	$(CUTILS_BASE)/a14/abort_socket.c \
	$(CUTILS_BASE)/a14/mspace.c \
	$(CUTILS_BASE)/a14/selector.c \
	$(CUTILS_BASE)/a14/tztime.c \
	$(CUTILS_BASE)/a14/zygote.c

targetSources += $(CUTILS_BASE)/a14/arch-arm/memset32.S

LOCAL_SRC_FILES := $(commonSources) $(targetSources)

LOCAL_C_INCLUDES := \
	meet

LOCAL_STATIC_LIBRARIES := \
	liblog_a14 \

LOCAL_MODULE := cutils_a14

include $(BUILD_STATIC_LIBRARY)

########################[libutils]########################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(UTILS_BASE)/BufferedTextOutput.cpp \
	$(UTILS_BASE)/Debug.cpp \
	$(UTILS_BASE)/RefBase.cpp \
	$(UTILS_BASE)/SharedBuffer.cpp \
	$(UTILS_BASE)/Static.cpp \
	$(UTILS_BASE)/String8.cpp \
	$(UTILS_BASE)/String16.cpp \
	$(UTILS_BASE)/SystemClock.cpp \
	$(UTILS_BASE)/TextOutput.cpp \
	$(UTILS_BASE)/Threads.cpp \
	$(UTILS_BASE)/Timers.cpp \
	$(UTILS_BASE)/VectorImpl.cpp \

LOCAL_C_INCLUDES := \
	meet

LOCAL_STATIC_LIBRARIES := \
	libcutils \

LOCAL_MODULE := utils

include $(BUILD_STATIC_LIBRARY)

########################[libutils_a14]########################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(UTILS_BASE)/a14/BufferedTextOutput.cpp \
	$(UTILS_BASE)/a14/Debug.cpp \
	$(UTILS_BASE)/a14/RefBase.cpp \
	$(UTILS_BASE)/a14/SharedBuffer.cpp \
	$(UTILS_BASE)/a14/Static.cpp \
	$(UTILS_BASE)/a14/String8.cpp \
	$(UTILS_BASE)/a14/String16.cpp \
	$(UTILS_BASE)/a14/SystemClock.cpp \
	$(UTILS_BASE)/a14/TextOutput.cpp \
	$(UTILS_BASE)/a14/Threads.cpp \
	$(UTILS_BASE)/a14/Timers.cpp \
	$(UTILS_BASE)/a14/VectorImpl.cpp \
	$(UTILS_BASE)/a14/Unicode.cpp

LOCAL_C_INCLUDES := \
	meet

LOCAL_STATIC_LIBRARIES := \
	libcutils_a14 \

LOCAL_MODULE := utils_a14

include $(BUILD_STATIC_LIBRARY)

########################[libos_a8]##############################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(OS_BASE)/factory_a8.cpp \
	$(OS_BASE)/audiotrack_a8.cpp

LOCAL_C_INCLUDES := \
	meet \
	meet/platform-pp \
	meet/android/frameworks/base/include
	
LOCAL_LDLIBS := \
	-L$(PREBUILT_BASE)/a8 \
	-lmedia \
	-lstagefright \
	-lsurfaceflinger_client \
	-lbinder

LOCAL_MODULE := os_a8

#include $(BUILD_SHARED_LIBRARY)

########################[libos_a9]##############################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(OS_BASE)/factory_a9.cpp \
	$(OS_BASE)/audiotrack_a9.cpp \

LOCAL_SRC_FILES += \
	$(DEVICE_BASE)/xiaomi_mioneplus_ppAudioTrack.cpp

LOCAL_C_INCLUDES := \
	meet \
	meet/platform-pp \
	meet/android/frameworks/base/include
	
LOCAL_LDLIBS := \
	-L$(PREBUILT_BASE) \
	-lmedia \
	-lstagefright \
	-lsurfaceflinger_client \
	-lbinder

LOCAL_MODULE := os_a9

#include $(BUILD_SHARED_LIBRARY)

########################[libmedia_common]########################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(PLAYER_BASE)/MediaSource.cpp \
	$(PLAYER_BASE)/MetaData.cpp \
	$(PLAYER_BASE)/MediaDefs.cpp \
	$(PLAYER_BASE)/MediaBuffer.cpp \
	$(PLAYER_BASE)/Utils.cpp \
	$(PLAYER_BASE)/ESDS.cpp \

LOCAL_C_INCLUDES := \
	meet \
	meet/android/frameworks/base/include \
	meet/android/frameworks/base/include/media/stagefright/openmax \
	meet/android/system/core/include \
	meet/android/hardware/libhardware/include 

LOCAL_MODULE := media_common

include $(BUILD_STATIC_LIBRARY)

########################[libdevice]########################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(DEVICE_BASE)/samsung_gti9000.cpp \
	$(DEVICE_BASE)/samsung_gti9020.cpp \
	$(DEVICE_BASE)/samsung_gti9220.cpp \
	$(DEVICE_BASE)/samsung_gts5660.cpp \
	$(DEVICE_BASE)/xiaomi_mioneplus.cpp \
	$(DEVICE_BASE)/huawei_u8800.cpp \
	$(DEVICE_BASE)/lge_optimus2x.cpp \
	$(DEVICE_BASE)/motorola_mx525.cpp \
	$(DEVICE_BASE)/htc_htcz710t.cpp \
	$(DEVICE_BASE)/htc_htcz710e.cpp \
	$(DEVICE_BASE)/htc_htcx515d.cpp \
	$(DEVICE_BASE)/htc_htca510e.cpp


#	$(DEVICE_BASE)/motorola_mx722.cpp \
#	$(DEVICE_BASE)/teclast_p76ti.cpp
#	$(DEVICE_BASE)/htc_htcincredibles2.cpp \

LOCAL_C_INCLUDES := \
	meet \
	meet/platform-pp \
	meet/android/frameworks/base/include \
	meet/android/frameworks/base/include/media/stagefright/openmax \
	meet/android/system/core/include \
	meet/android/hardware/libhardware/include
	
LOCAL_MODULE := device

include $(BUILD_STATIC_LIBRARY)

########################[libdevice_internal]########################
# Dependent interfaces
# - AudioTrack.h (frameworks\base\include\media)
# - AudioSystem.h (frameworks\base\include\media)
# - IOMX.h (frameworks\base\include\media)
# - OMXClient.h (frameworks\base\include\media\stagefright)
# - OMXCodec.h (frameworks\base\include\media\stagefright)
# - Surface.h (frameworks\base\include\surfaceflinger)
# - RefBase.h (frameworks\base\include\utils)
# - MediaSource.h (frameworks\base\include\media\stagefright)
# - MediaBuffer.h (frameworks\base\include\media\stagefright)
# Dependent implementations
# - RefBase.cpp (frameworks\base\include\utils)
# - MediaSource.cpp (frameworks\base\include\media\stagefright)
# - MediaBuffer.cpp (frameworks\base\include\media\stagefright)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(DEVICE_BASE)/devicefactory.cpp

LOCAL_C_INCLUDES := \
	meet \
	meet/platform-pp \
	meet/android/frameworks/base/include \
	meet/android/frameworks/base/include/media/stagefright/openmax \
	meet/android/hardware/libhardware/include \
	meet/android/system/core/include \
	

LOCAL_LDLIBS := \
    -L$(PREBUILT_BASE) \
	-lmedia \
	-lstagefright \
	-lsurfaceflinger_client \
	-lbinder	
	

LOCAL_STATIC_LIBRARIES := \
	libdevice \
	libmedia_common \
	libutils \
	libcutils \
	liblog

LOCAL_MODULE := device_internal

#include $(BUILD_SHARED_LIBRARY)


########################[libdevice_cm]########################
# Dependent interfaces
# - OMXClient.h (frameworks\base\include\media\stagefright)
# - Surface.h (frameworks\base\include\surfaceflinger)
# - IMemory.h (frameworks\base\include\binder)
# - IAudioTrack.h (frameworks\base\include\media)
# - IAudioFlinger.h (frameworks\base\include\media)
# - IOMX.h (frameworks\base\include\media)
# - IInterface.h (frameworks\base\include\binder)
# - IBinder.h (frameworks\base\include\binder)
# - Binder.h (frameworks\base\include\binder)


include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(DEVICE_BASE)/devicefactory_cm.cpp

LOCAL_SRC_FILES += \
	$(PLAYER_BASE)/OMXCodec_cm.cpp

LOCAL_SRC_FILES += \
	$(MEDIA_BASE)/ppAudioTrack_cm.cpp \
	$(MEDIA_BASE)/AudioTrack_cm.cpp \
	$(MEDIA_BASE)/AudioSystem_cm.cpp
	
# 	$(MEDIA_BASE)/IAudioPolicyService.cpp 

LOCAL_SRC_FILES += \
	$(DEVICE_BASE)/generic_device.cpp \
	$(DEVICE_BASE)/htc_htcz510d.cpp \
	$(DEVICE_BASE)/htc_htcincredibles.cpp

#	$(DEVICE_BASE)/samsung_gts5820.cpp
#	$(DEVICE_BASE)/huawei_hi3716c.cpp \
#	$(DEVICE_BASE)/meizu_m9.cpp \
#	$(DEVICE_BASE)/htc_htcsensationxl.cpp \
#	$(DEVICE_BASE)/motorola_mx860.cpp
#	$(DEVICE_BASE)/htc_htcdesirez.cpp \
#	$(DEVICE_BASE)/samsung_gti9100.cpp \
#	$(DEVICE_BASE)/samsung_gti9100_ppAudioTrack.cpp \
#	$(DEVICE_BASE)/samsung_gti9100_TICameraParameters.cpp

LOCAL_C_INCLUDES := \
	meet \
	meet/platform-pp \
	meet/android/frameworks/base/include \
	meet/android/frameworks/base/include/media/stagefright/openmax \
	meet/android/system/core/include \
	meet/android/hardware/libhardware/include 

LOCAL_LDLIBS := \
	-L$(PREBUILT_BASE) \
	-lmedia \
	-lbinder \
	-lstagefright \
	-lsurfaceflinger_client \

LOCAL_STATIC_LIBRARIES := \
	libdevice \
	libmedia_common \
	libutils \
	libcutils \
	liblog 
	
LOCAL_MODULE := device_cm

#include $(BUILD_SHARED_LIBRARY)


########################[libppplayer]########################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(PLATFORM_BASE)/PPPlatForm.cpp \

LOCAL_SRC_FILES += \
	$(PLAYER_BASE)/PPPlayer.cpp \
	$(PLAYER_BASE)/PPMediaBufferGroup.cpp \
	$(PLAYER_BASE)/MediaBufferGroup.cpp \
	$(PLAYER_BASE)/AwesomePlayer.cpp \
	$(PLAYER_BASE)/DataSource.cpp \
	$(PLAYER_BASE)/FileSource.cpp \
	$(PLAYER_BASE)/TimeSource.cpp \
	$(PLAYER_BASE)/MediaExtractor.cpp \
	$(PLAYER_BASE)/AudioPlayer.cpp \
	$(PLAYER_BASE)/TIVideoConfigParser.cpp \
	$(PLAYER_BASE)/ppDataSource.cpp \
	$(PLAYER_BASE)/ppExtractor.cpp \
	$(PLAYER_BASE)/TimedEventQueue.cpp \
	$(PLAYER_BASE)/MPEG4Extractor.cpp \
	$(PLAYER_BASE)/SampleIterator.cpp \
	$(PLAYER_BASE)/SampleTable.cpp \


#	$(PLAYER_BASE)/ColorConverter.cpp
	

LOCAL_C_INCLUDES := \
	meet \
	meet/android/frameworks/base/include \
	meet/android/frameworks/base/include/media/stagefright/openmax \
	meet/android/system/core/include \
	meet/android/hardware/libhardware/include \
	$(ENGINE_BASE)

LOCAL_LDLIBS := \
	-L$(PREBUILT_BASE) \

LOCAL_STATIC_LIBRARIES := \
	libmedia_common \
	libutils \
	liblog \
	libcutils
	
LOCAL_MODULE := ppplayer

#include $(BUILD_SHARED_LIBRARY)

########################[libppplayer_a14]########################
# Dependent interfaces
# - MediaSource/ReadOptions (frameworks\base\include\media\stagefright\MediaSource.h)
# - MetaData (frameworks\base\include\media\stagefright\MetaData.h)
# - MediaBuffer (frameworks\base\include\media\stagefright\MediaBuffer.h)
# - ANativeWindow  (frameworks\base\include\ui\egl\Android_natives.h)
# - RefBase/SP (frameworks\base\include\utils\RefBase.h)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(PLATFORM_BASE)/a14/PPPlatForm.cpp \
	$(PLATFORM_BASE)/a14/samsung_gti9000.cpp \
	$(PLATFORM_BASE)/a14/samsung_gti9300.cpp


LOCAL_SRC_FILES += \
	$(PLAYER_BASE)/a14/PPPlayer.cpp \
	$(PLAYER_BASE)/a14/PPMediaBufferGroup.cpp \
	$(PLAYER_BASE)/a14/AwesomePlayer.cpp \
	$(PLAYER_BASE)/a14/MediaExtractor.cpp \
	$(PLAYER_BASE)/a14/AudioPlayer.cpp \
	$(PLAYER_BASE)/a14/ppDataSource.cpp \
	$(PLAYER_BASE)/a14/ppExtractor.cpp \
	$(PLAYER_BASE)/a14/TimedEventQueue.cpp \
	$(PLAYER_BASE)/a14/DataSource.cpp \
	$(PLAYER_BASE)/a14/MPEG4Extractor.cpp \
	$(PLAYER_BASE)/a14/SampleIterator.cpp \
	$(PLAYER_BASE)/a14/SampleTable.cpp \
	$(PLAYER_BASE)/a14/MediaBufferGroup.cpp \

LOCAL_SRC_FILES += \
	$(PLAYER_BASE)/a14/MediaSource.cpp \
	$(PLAYER_BASE)/a14/MediaBuffer.cpp \
	$(PLAYER_BASE)/a14/MetaData.cpp \
	$(PLAYER_BASE)/a14/MediaDefs.cpp \
	$(PLAYER_BASE)/a14/ESDS.cpp \


#	$(PLAYER_BASE)/a14/OMXCodec.cpp \
#	$(PLAYER_BASE)/SampleIterator.cpp \
#	$(PLAYER_BASE)/SampleTable.cpp \
#	$(PLAYER_BASE)/MPEG4Extractor.cpp \
#	$(PLAYER_BASE)/TIVideoConfigParser.cpp \
#	$(PLAYER_BASE)/DataSource.cpp \
#	$(PLAYER_BASE)/FileSource.cpp \
#	$(PLAYER_BASE)/TimeSource.cpp \
#	$(PLAYER_BASE)/MediaBufferGroup.cpp \ 
	

LOCAL_C_INCLUDES := \
	meet \
	$(ENGINE_BASE)

LOCAL_LDLIBS := \
	-L$(PREBUILT_BASE)/a14 \
	-lbinder \
	-lmedia \
	-lstagefright \
	-lstagefright_foundation \

#	-lutils \
#	-lcutils \
#	-llog \
#	-lsurfaceflinger_client \
#	-lstagefright_foundation \
#	-lgui \
#	-lui \

LOCAL_STATIC_LIBRARIES := \
	libutils_a14 \
	libcutils_a14 \
	liblog_a14
	
LOCAL_MODULE := ppplayer_a14

include $(BUILD_SHARED_LIBRARY)

########################[libffplayer_v6_vfp]########################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(ENGINE_BASE)/output/android/libplayer_v6_vfp.so

LOCAL_MODULE := ffplayer_v6_vfp

#include $(PREBUILT_SHARED_LIBRARY)

########################[libffplayer_neon]########################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(ENGINE_BASE)/output/android/libplayer_neon.so

LOCAL_MODULE := player_neon

include $(PREBUILT_SHARED_LIBRARY)

########################[libppmediaextractor-jni]########################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(JNI_BASE)/PPMediaExtractor.cpp \
	$(JNI_BASE)/PPBox_Util.cpp \
	$(JNI_BASE)/JNIHelp.c \

LOCAL_SRC_FILES += \
	$(PLAYER_BASE)/DataSource.cpp \
	$(PLAYER_BASE)/FileSource.cpp \
	$(PLAYER_BASE)/MediaExtractor.cpp \
	$(PLAYER_BASE)/PPMediaBufferGroup.cpp \
	$(PLAYER_BASE)/ppDataSource.cpp \
	$(PLAYER_BASE)/ppExtractor.cpp \

LOCAL_SRC_FILES += \
	$(PLAYER_BASE)/a16/ABuffer.cpp \
	$(PLAYER_BASE)/a16/AString.cpp \
	$(PLAYER_BASE)/a16/AMessage.cpp \
	$(PLAYER_BASE)/a16/AAtomizer.cpp \
	$(PLAYER_BASE)/a16/NuPPMediaExtractor.cpp \

LOCAL_C_INCLUDES := \
	meet \
	meet/android/frameworks/base/include \
	meet/android/frameworks/base/include/media/stagefright/openmax \
	meet/android/system/core/include \
	meet/android/hardware/libhardware/include \
	$(ENGINE_BASE)
	
LOCAL_CFLAGS := \
	-DANDROID_PLATFORM=16

LOCAL_STATIC_LIBRARIES := \
	libmedia_common \
	liblog \
	libutils \
	libcutils \

LOCAL_MODULE := ppmediaextractor-jni

include $(BUILD_SHARED_LIBRARY)

########################[libmeet]########################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(JNI_BASE)/android_media_MediaPlayer.cpp \
	$(JNI_BASE)/PPBox_Util.cpp \
	$(JNI_BASE)/JNIHelp.c \


LOCAL_SRC_FILES += \
	$(CUTILS_BASE)/cpufeatures/cpu-features.c \
	
LOCAL_C_INCLUDES := \
	meet \
	$(ENGINE_BASE)
	
LOCAL_STATIC_LIBRARIES := \
	liblog \

LOCAL_MODULE := meet

include $(BUILD_SHARED_LIBRARY)

#######################[libsubtitle-jni]#######################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(JNI_BASE)/SimpleSubTitleParser.cpp

LOCAL_C_INCLUDES := \
	meet \
	$(ENGINE_BASE) 

LOCAL_LDLIBS := \
	-llog \
	-L$(ENGINE_BASE)/subtitle/output/android \
	-lsubtitle \
	-lass \
	-lenca \
	-liconv \
	-lcharset \
	-L$(NDK_HOME)/sources/cxx-stl/stlport/libs/armeabi \
	-lstlport_static 


LOCAL_MODULE := subtitle-jni

include $(BUILD_SHARED_LIBRARY)
