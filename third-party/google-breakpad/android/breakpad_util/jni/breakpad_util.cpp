#include <stdio.h>
#include <jni.h>

#include <android/log.h>

#include "client/linux/handler/exception_handler.h"
#include "client/linux/handler/minidump_descriptor.h"

#ifndef LOGD
#define LOGD(tag, ...) ((void)__android_log_print(ANDROID_LOG_DEBUG, tag, __VA_ARGS__))
#endif

namespace {

__BEGIN_DECLS
	
google_breakpad::ExceptionHandler* gExceptionHandler = NULL;

bool DumpCallback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded) {
	LOGD("GoogleBreakpad-jni", "succeeded: %d", succeeded);
	return succeeded;
}

bool init_breakpad_handler(google_breakpad::ExceptionHandler** eh, const char* path)
{
	if (path != NULL && strlen(path) > 0) 
	{
		if (*eh == NULL) 
		{
			LOGD("GoogleBreakpad-jni", "dump path: %s", path);
			google_breakpad::MinidumpDescriptor descriptor(path);
			*eh = new google_breakpad::ExceptionHandler(descriptor, NULL, DumpCallback, NULL, true, -1);
		}

		return true;
	}

	return false;
}

void finit_breakpad_handler(google_breakpad::ExceptionHandler** eh)
{
	if (*eh != NULL)
	{
		delete (*eh);
		*eh = NULL;
	}
}

JNIEXPORT jboolean JNICALL
Java_com_pplive_thirdparty_BreakpadUtil_registerBreakpad(JNIEnv *env, jobject thiz, jstring js_dump_dir_path)
{
	const char* dump_dir_path = env->GetStringUTFChars(js_dump_dir_path, NULL);	

	return init_breakpad_handler(&gExceptionHandler, dump_dir_path);
}

JNIEXPORT void JNICALL
Java_com_pplive_thirdparty_BreakpadUtil_unregisterBreakpad(JNIEnv *env, jobject thiz)
{
	finit_breakpad_handler(&gExceptionHandler);
}

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM* jvm, void* reserved)
{
	return JNI_VERSION_1_4;
}

__END_DECLS

}  // namespace
