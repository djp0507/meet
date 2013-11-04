# The ARMv7 is significanly faster due to the use of the hardware FPU
APP_ABI := armeabi

APP_PLATFORM := android-9

APP_OPTIM := release

APP_CFLAGS := \
	-Wno-multichar \
	-mfpu=vfpv3-d16 \
	-DHAVE_ANDROID_OS=1 \
	-DANDROID_SMP=0 \
	-DBUILD_WITH_FULL_STAGEFRIGHT=1 \
	-include AndroidConfig.h \
