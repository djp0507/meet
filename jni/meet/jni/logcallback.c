#include <sys/system_properties.h>

#include "logcallback.h"
#include <stdio.h>
#include <jni.h>
#include <stdarg.h>

#define OS_ANDROID

namespace android {

extern JavaVM *gs_jvm;

void android_log_callback(int level, char* text)
{
	JNIEnv *env = NULL;

	printf("enter android_log_callback\n");
	if(gs_jvm==NULL) return;

	if (gs_jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
	{
		return;
	}

	jclass clazz = env->FindClass("android/pplive/media/player/MeetSDK");

	jmethodID jlog = env->GetStaticMethodID(clazz, "nativeLog",
			"(ILjava/lang/String;)V");
	if (jlog == NULL) {
		//jniThrowException(env, "java/lang/RuntimeException", "Can't find method MeetSDK.nativeLog");
		return;
	}


	jstring arg = env->NewStringUTF(text);

	printf("enter CallStaticVoidMethod\n");;
	env->CallStaticVoidMethod(clazz, jlog, level, arg);
}

void __android_file_log_print(int priority, const char *tag, const char *fmt, ...)
{
	//__android_log_print(prio, tag, fmt, __VA_ARGS__);
	printf("enter __android_file_log_print\n");
	va_list argp;
    va_start(argp, fmt);

    char msg[2048] = {0};
    vsnprintf(msg, sizeof(msg), fmt, argp);

    va_end(argp);

    printf("end __android_file_log_print\n");
    android_log_callback(priority, msg);
}

}
