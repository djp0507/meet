/*
 * Copyright (C) 2012 Roger Shen  rogershen@pptv.com
 *
 */

#define LOG_TAG "MediaPlayer-JNI"

#include <sys/system_properties.h>

#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <jni.h>
#include <dlfcn.h>

#include "libcutils/cpufeatures/cpu-features.h"
#include "include-pp/utils/threads.h"
#include "include-pp/utils/Log.h"
#include "include-pp/nativehelper/JNIHelp.h"
#include "platform/platforminfo.h"
#include "include-pp/PPBox_Util.h"

#define OS_ANDROID
namespace android {
#include "player/player.h"
}
// ----------------------------------------------------------------------------

using namespace android;

// ----------------------------------------------------------------------------

struct fields_t {
	jfieldID    context;
	jfieldID    surface;
	/* actually in android.view.Surface XXX */
	jfieldID    surface_native;

	jmethodID   post_event;
};

static fields_t fields;

static Mutex sLock;

namespace android
{

	class Surface;

	JavaVM *gs_jvm = NULL;

	PlatformInfo* gPlatformInfo = NULL;

	bool START_P2P = false;

	static void* player_handle_software = NULL;
	static void* player_handle_hardware = NULL;

	// new
	typedef IPlayer* (*GET_PLAYER_FUN) (void*);

	GET_PLAYER_FUN getPlayerFun = NULL;

	// util method
#define vstrcat(first, ...) (vstrcat_impl(first, __VA_ARGS__, (char*)NULL))

static
char* vstrcat_impl(const char* first, ...)
{
	size_t len = 0;
	char* buf = NULL;
	va_list args;
	char* p = NULL;


	if (first == NULL)
		return NULL;

	len = strlen(first);

	va_start(args, first);
	while((p = va_arg(args, char*)) != NULL)
		len += strlen(p);
	va_end(args);

	buf = (char *)malloc(len + 1);

	strcpy(buf, first);

	va_start(args, first);
	while((p = va_arg(args, char*)) != NULL)
		strcat(buf, p);
	va_end(args);

	return buf;
}

static char* jstr2cstr(JNIEnv* env, const jstring jstr) 
{
	char* cstr = NULL;
	if (env != NULL) 
	{
		const char* tmp = env->GetStringUTFChars(jstr, NULL);
		const size_t len = strlen(tmp) + 1;
		cstr = (char*)malloc(len);
		bzero(cstr, len);
		snprintf(cstr, len, "%s", tmp);
		env->ReleaseStringUTFChars(jstr, tmp);
	}

	return cstr;
}

static jstring cstr2jstr(JNIEnv* env, const char* cstr)
{
	jstring jstr = NULL;
	if (env != NULL && cstr != NULL) 
	{
		jstr = env->NewStringUTF(cstr);
	}

	return jstr;
}

static
const char* getCodecLibName(uint64_t cpuFeatures)
{
	const char* codecLibName = NULL;
	if ((cpuFeatures & ANDROID_CPU_ARM_FEATURE_NEON) != 0)
	{
		//neon
		LOGI("the device supports neon");
		codecLibName = "libplayer_neon.so";
	}
	else if((cpuFeatures & ANDROID_CPU_ARM_FEATURE_ARMv7) != 0)
	{
		//v7_vfpv3d16
		LOGI("the device supports v7_vfpv3d16");
		codecLibName = "libplayer_tegra2.so";
	}
	else if ((cpuFeatures & ANDROID_CPU_ARM_FEATURE_VFP) != 0)
	{
		//armv6_vfp 
		LOGI("the device supports armv6_vfp");
		codecLibName = "libplayer_v6_vfp.so";
	}
	else if((cpuFeatures & ANDROID_CPU_ARM_FEATURE_LDREX_STREX) != 0)
	{
		//armv6
		LOGI("the device supports armv6");
		codecLibName = "libplayer_v6.so";
	}
	else
	{
		//armv5te or lower
		LOGI("the device supports armv5te");
		codecLibName = "libplayer_v5te.so";
	}

	return codecLibName;
}


JNIEnv* getJNIEnvPP()
{
	if(gs_jvm==NULL) return NULL;
	JNIEnv* env;

	if (gs_jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
	{
		return NULL;
	}

	return env;
}

int jniRegisterNativeMethodsPP(JNIEnv* env, const char* className, const JNINativeMethod* gMethods, int numMethods)
{
	jclass clazz;

	LOGV("Registering %s natives", className);
	clazz = env->FindClass(className);
	if (clazz == NULL) {
		LOGE("Native registration unable to find class '%s'", className);
		return -1;
	}

	int result = 0;
	if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
		LOGE("RegisterNatives failed for '%s'", className);
		result = -1;
	}

	env->DeleteLocalRef(clazz);
	return result;
}

// Returns the Unix file descriptor for a ParcelFileDescriptor object
int getParcelFileDescriptorFDPP(JNIEnv* env, jobject object)
{
	jclass clazz = env->FindClass("java/io/FileDescriptor");
	LOG_FATAL_IF(clazz == NULL, "Unable to find class java.io.FileDescriptor");
	jfieldID descriptor = env->GetFieldID(clazz, "descriptor", "I");
	LOG_FATAL_IF(descriptor == NULL,
			"Unable to find descriptor field in java.io.FileDescriptor");

	return env->GetIntField(object, descriptor);
}

}

// ----------------------------------------------------------------------------
// ref-counted object for callbacks
class JNIMediaPlayerListener: public MediaPlayerListener
{
	public:
		JNIMediaPlayerListener(JNIEnv* env, jobject thiz, jobject weak_thiz);
		~JNIMediaPlayerListener();
		void notify(int msg, int ext1, int ext2);
	private:
		JNIMediaPlayerListener();
		jclass      mClass;     // Reference to MediaPlayer class
		jobject     mObject;    // Weak ref to MediaPlayer Java object to call on
};

JNIMediaPlayerListener::JNIMediaPlayerListener(JNIEnv* env, jobject thiz, jobject weak_thiz)
{
	LOGD("JNIMediaPlayerListener constructor");

	// Hold onto the MediaPlayer class for use in calling the static method
	// that posts events to the application thread.
	jclass clazz = env->GetObjectClass(thiz);
	if (clazz == NULL) {
		LOGE("Can't find android/pplive/media/player/NativeMediaPlayer");
		jniThrowException(env, "java/lang/Exception", NULL);
		return;
	}
	mClass = (jclass)env->NewGlobalRef(clazz);

	// We use a weak reference so the MediaPlayer object can be garbage collected.
	// The reference is only used as a proxy for callbacks.
	mObject  = env->NewGlobalRef(weak_thiz);
}

JNIMediaPlayerListener::~JNIMediaPlayerListener()
{
	LOGD("JNIMediaPlayerListener destructor");
	// remove global references
	//JNIEnv *env = AndroidRuntime::getJNIEnv();
	JNIEnv *env = getJNIEnvPP();
	env->DeleteGlobalRef(mObject);
	env->DeleteGlobalRef(mClass);
}

void JNIMediaPlayerListener::notify(int msg, int ext1, int ext2)
{
	JNIEnv *env = getJNIEnvPP();
	env->CallStaticVoidMethod(mClass, fields.post_event, mObject, msg, ext1, ext2, 0);
}

// ----------------------------------------------------------------------------

static Surface* get_surface(JNIEnv* env, jobject clazz)
{
	return (Surface*)env->GetIntField(clazz, fields.surface_native);
}

static IPlayer* getMediaPlayer(JNIEnv* env, jobject thiz)
{
	Mutex::Autolock l(sLock);
	IPlayer* p = (IPlayer*)env->GetIntField(thiz, fields.context);
	return p;
}

static IPlayer* setMediaPlayer(JNIEnv* env, jobject thiz, IPlayer* player)
{
	Mutex::Autolock l(sLock);
	IPlayer* old = (IPlayer*)env->GetIntField(thiz, fields.context);
	env->SetIntField(thiz, fields.context, (int)player);
	return old;
}

// If exception is NULL and opStatus is not OK, this method sends an error
// event to the client application; otherwise, if exception is not NULL and
// opStatus is not OK, this method throws the given exception to the client
// application.
static void process_media_player_call(JNIEnv *env, jobject thiz, status_t opStatus, const char* exception, const char *message)
{
	if (exception == NULL) {  // Don't throw exception. Instead, send an event.
		if (opStatus != (status_t) OK) {
			IPlayer* mp = getMediaPlayer(env, thiz);
			if (mp != 0) mp->notify(MEDIA_ERROR, opStatus, 0);
		}
	} else {  // Throw exception!
		if ( opStatus == (status_t) INVALID_OPERATION ) {
			jniThrowException(env, "java/lang/IllegalStateException", NULL);
		} else if ( opStatus != (status_t) OK ) {
			if (strlen(message) > 230) {
				// if the message is too long, don't bother displaying the status code
				jniThrowException( env, exception, message);
			} else {
				char msg[256];
				// append the status code to the message
				sprintf(msg, "%s: status=0x%X", message, opStatus);
				jniThrowException( env, exception, msg);
			}
		}
	}
}

static void
android_media_MediaPlayer_setDataSourceAndHeaders(
		JNIEnv *env, jobject thiz, jstring path, jobject headers) {

	//LOGE("Start get MediaPlayer");
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		LOGE("create MediaPlayer failed");
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return;
	}

	if (path == NULL) {
		jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
		return;
	}

	const char *pathStr = env->GetStringUTFChars(path, NULL);
	if (pathStr == NULL) {  // Out of memory
		jniThrowException(env, "java/lang/RuntimeException", "Path is NULL.");
		return;
	}

	status_t opStatus = mp->setDataSource(pathStr);

	// Make sure that local ref is released before a potential exception
	env->ReleaseStringUTFChars(path, pathStr);

	process_media_player_call(
			env, thiz, opStatus, "java/io/IOException",
			"setDataSource failed." );
}

static 
void android_media_MediaPlayer_setDataSource(JNIEnv *env, jobject thiz, jstring path)
{
	android_media_MediaPlayer_setDataSourceAndHeaders(env, thiz, path, 0);
}

static 
void android_media_MediaPlayer_setDataSourceFD(JNIEnv *env, jobject thiz, jobject fileDescriptor, jlong offset, jlong length)
{
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return;
	}

	if (fileDescriptor == NULL) {
		jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
		return;
	}
	//int fd = getParcelFileDescriptorFD(env, fileDescriptor);
	int fd = getParcelFileDescriptorFDPP(env, fileDescriptor);
	LOGV("setDataSourceFD: fd %d", fd);
	process_media_player_call( env, thiz, mp->setDataSource(fd, offset, length), "java/io/IOException", "setDataSourceFD failed." );
}

static void setVideoSurface(IPlayer* mp, JNIEnv *env, jobject thiz)
{
	jobject surface = env->GetObjectField(thiz, fields.surface);

	if (surface != NULL)
    {
        // sdk_version >= 18
        gPlatformInfo->javaSurface = env->NewGlobalRef(surface);

	    void* native_surface = (void*)get_surface(env, surface);
	    mp->setVideoSurface((void*)native_surface);
	}
	else
	{
		LOGE("setVideoSurface is NULL");
	}
}

static 
void android_media_MediaPlayer_setVideoSurface(JNIEnv *env, jobject thiz)
{
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return;
	}
	setVideoSurface(mp, env, thiz);
}

static 
void android_media_MediaPlayer_prepare(JNIEnv *env, jobject thiz)
{
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return;
	}
	setVideoSurface(mp, env, thiz);
	process_media_player_call( env, thiz, mp->prepare(), "java/io/IOException", "Prepare failed." );
}

static
void android_media_MediaPlayer_prepareAsync(JNIEnv *env, jobject thiz)
{
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return;
	}
	jobject surface = env->GetObjectField(thiz, fields.surface);
	if (surface != NULL) {
		Surface* native_surface = get_surface(env, surface);
		mp->setVideoSurface((void*)native_surface);
	}
	process_media_player_call( env, thiz, mp->prepareAsync(), "java/io/IOException", "Prepare Async failed." );
}

static
void android_media_MediaPlayer_start(JNIEnv *env, jobject thiz)
{	
	LOGV("android_media_MediaPlayer_start()");
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return;
	}
	process_media_player_call( env, thiz, mp->start(), NULL, NULL );
}

static 
void android_media_MediaPlayer_stop(JNIEnv *env, jobject thiz)
{
	LOGD("++++++++Start stopping");
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return;
	}

	process_media_player_call( env, thiz, mp->stop(), NULL, NULL );
	LOGD("++++++++End stopping");
}

static 
void android_media_MediaPlayer_pause(JNIEnv *env, jobject thiz)
{
	LOGD("++++++++Start pausing");
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return;
	}
	process_media_player_call( env, thiz, mp->pause(), NULL, NULL );
	LOGD("++++++++End pausing");
}

static 
jboolean android_media_MediaPlayer_isPlaying(JNIEnv *env, jobject thiz)
{
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return false;
	}
	const jboolean is_playing = mp->isPlaying();

	LOGD("isPlaying: %d", is_playing);
	return is_playing;
}

static 
void android_media_MediaPlayer_seekTo(JNIEnv *env, jobject thiz, int msec)
{	
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return;
	}

	int playTime=0;
	mp->getCurrentPosition(&playTime);
	int mediaDiff = msec -playTime;//ms
	mediaDiff = mediaDiff>0?mediaDiff:-mediaDiff;

	if(mediaDiff > 2000)// && actionDiff > 500)//2sec
	{
		LOGD("&&&&seekTo: %d(msec)", msec);
		process_media_player_call( env, thiz, mp->seekTo(msec), NULL, NULL );
		//lastSeekMediaTime = msec;
		//lastSeekActionTime = nowMs;
		LOGD("&&&&seekTo: %d(msec) end", msec);
	}
	else
	{
		mp->notify(MEDIA_SEEK_COMPLETE, 0, 0);
	}
}

static 
int android_media_MediaPlayer_getVideoWidth(JNIEnv *env, jobject thiz)
{
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return 0;
	}
	int w;
	if (0 != mp->getVideoWidth(&w)) {
		LOGE("getVideoWidth failed");
		w = 0;
	}
	LOGV("getVideoWidth: %d", w);
	return w;
}

static 
int android_media_MediaPlayer_getVideoHeight(JNIEnv *env, jobject thiz)
{
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return 0;
	}
	int h;
	if (0 != mp->getVideoHeight(&h)) {
		LOGE("getVideoHeight failed");
		h = 0;
	}
	LOGV("getVideoHeight: %d", h);
	return h;
}


static
int android_media_MediaPlayer_getCurrentPosition(JNIEnv *env, jobject thiz)
{
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return 0;
	}
	LOGD("getCurrentPosition start");
	int msec;
	process_media_player_call( env, thiz, mp->getCurrentPosition(&msec), NULL, NULL );
	LOGD("getCurrentPosition: %d (msec)", msec);
	return msec;
}

static 
int android_media_MediaPlayer_getDuration(JNIEnv *env, jobject thiz)
{
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return 0;
	}
	LOGD("get media duration start");
	int msec;
	process_media_player_call( env, thiz, mp->getDuration(&msec), NULL, NULL );
	LOGD("get media duration is : %d (msec)", msec);
	return msec;
}

static 
void android_media_MediaPlayer_reset(JNIEnv *env, jobject thiz)
{
	LOGD("++++++++Start resetting");
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return;
	}
	process_media_player_call( env, thiz, mp->reset(), NULL, NULL );
	LOGD("++++++++End resetting");
}

static
void android_media_MediaPlayer_setAudioStreamType(JNIEnv *env, jobject thiz, int streamtype)
{
	LOGD("setAudioStreamType: %d", streamtype);
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return;
	}
	process_media_player_call( env, thiz, mp->setAudioStreamType(streamtype) , NULL, NULL );
}

static 
void android_media_MediaPlayer_setLooping(JNIEnv *env, jobject thiz, jboolean looping)
{
	LOGD("setLooping: %d", looping);
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return;
	}
	process_media_player_call( env, thiz, mp->setLooping(looping), NULL, NULL );
}

static 
jboolean android_media_MediaPlayer_isLooping(JNIEnv *env, jobject thiz)
{
	LOGV("isLooping");
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return false;
	}
	return mp->isLooping();
}

static 
void android_media_MediaPlayer_setVolume(JNIEnv *env, jobject thiz, float leftVolume, float rightVolume)
{
	LOGV("setVolume: left %f  right %f", leftVolume, rightVolume);
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return;
	}
	process_media_player_call( env, thiz, mp->setVolume(leftVolume, rightVolume), NULL, NULL );
}

// FIXME: deprecated
static 
jobject android_media_MediaPlayer_getFrameAt(JNIEnv *env, jobject thiz, jint msec)
{
	return NULL;
}


// Sends the request and reply parcels to the media player via the
// binder interface.
static 
jint android_media_MediaPlayer_invoke(JNIEnv *env, jobject thiz,
		jobject java_request, jobject java_reply)
{
	return 0;
}

// Sends the new filter to the client.
static 
jint android_media_MediaPlayer_setMetadataFilter(JNIEnv *env, jobject thiz, jobject request)
{
	return 0;
}

static 
jboolean android_media_MediaPlayer_getMetadata(JNIEnv *env, jobject thiz, jboolean update_only,
		jboolean apply_filter, jobject reply)
{
	return 0;
}

static 
jint android_media_MediaPlayer_flags(JNIEnv *env, jobject thiz)
{
	LOGD("get flag");
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return 0;
	}
	int flag = mp->flags();
	LOGD("flag: %d", flag);
	return flag;
}

static
char* getCStringFromJavaStaticStringField(JNIEnv* env, const char* clazz_name, const char* field_name)
{
	jclass clazz = env->FindClass(clazz_name);
	if (clazz == NULL)
	{
		LOGE("Can't find class %s", clazz_name);
		return NULL;
	}

	jfieldID fieldID = env->GetStaticFieldID(clazz, field_name, "Ljava/lang/String;");
	if (fieldID == NULL)
	{
		LOGE("Can't find fieldID %s.%s", clazz_name, field_name);
		return NULL;
	}

	jstring jstr = (jstring)env->GetStaticObjectField(clazz, fieldID);
	if (jstr == NULL)
	{
		LOGE("Get static string field %s.%s failed.", clazz_name, field_name);
		return NULL;
	}

	char* cstr = jstr2cstr(env, jstr);

	return cstr;
}

// This function gets some field IDs, which in turn causes class initialization.
// It is called from a static block in MediaPlayer, which won't run until the
// first time an instance of this class is used.
static 
void android_media_MediaPlayer_native_init(JNIEnv *env, jobject thiz, jboolean startP2PEngine)
{
	LOGD("native_init");

	if (gPlatformInfo == NULL)
	{
		gPlatformInfo = new PlatformInfo();
	}

	gPlatformInfo->jvm = (void*)gs_jvm;

	char* app_root_dir = getCStringFromJavaStaticStringField(env, "android/pplive/media/MeetSDK", "AppRootDir");
	if (app_root_dir != NULL) 
	{
		snprintf(gPlatformInfo->app_path, STRLEN, "%s", app_root_dir);
		free(app_root_dir);
	}

	char* ppbox_lib_name = getCStringFromJavaStaticStringField(env, "android/pplive/media/MeetSDK", "PPBoxLibName");
	if (ppbox_lib_name != NULL)
	{
		snprintf(gPlatformInfo->ppbox_lib_name, STRLEN, "%s", ppbox_lib_name);
		free(ppbox_lib_name);
	}
	
	if (strlen(gPlatformInfo->model_name) == 0) {
		__system_property_get("ro.product.model", gPlatformInfo->model_name);
		__system_property_get("ro.board.platform", gPlatformInfo->board_name);
		__system_property_get("ro.build.mainchipname", gPlatformInfo->chip_name);
		__system_property_get("ro.product.manufacturer", gPlatformInfo->manufacture_name);
		__system_property_get("ro.build.version.release", gPlatformInfo->release_version);

		char sdk_version[STRLEN];
		__system_property_get("ro.build.version.sdk", sdk_version);
		gPlatformInfo->sdk_version = atoi(sdk_version);

		LOGI("MODEL_NAME: %s", gPlatformInfo->model_name);
		LOGI("BOARD_NAME: %s", gPlatformInfo->board_name);
		LOGI("CHIP_NAME: %s", gPlatformInfo->chip_name);
		LOGI("MANUFACTURE_NAME: %s", gPlatformInfo->manufacture_name);
		LOGI("RELEASE_VERSION: %s", gPlatformInfo->release_version);
		LOGI("SDK_VERSION: %d", gPlatformInfo->sdk_version);
		LOGI("APP_PATH: %s", gPlatformInfo->app_path);
		LOGI("PPBOX_LIB_NAME: %s", gPlatformInfo->ppbox_lib_name);
		LOGI("START_P2P: %d", START_P2P);
	}

	START_P2P = startP2PEngine;

	if(START_P2P)
	{
		char envSetting[STRLEN] = {0};
		//strncpy(envSetting, "TMPDIR=", 7);
		if(strlen(gPlatformInfo->app_path) > 0)
		{
			snprintf(envSetting, STRLEN, "TMPDIR=%s", gPlatformInfo->app_path);

			putenv(envSetting);

			char* p = NULL;
			if(p = getenv("TMPDIR"))
			{
				LOGI("TMPDIR: %s", p);
			}
			else
			{
				LOGE("cannot get TMPDIR");
			}
		}
	}


	jclass clazz = env->FindClass("android/pplive/media/player/NativeMediaPlayer");
	if (clazz == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "Can't find android/ppilve/media/player/NativeMediaPlayer");
		return;
	}

	fields.context = env->GetFieldID(clazz, "mNativeContext", "I");
	if (fields.context == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "Can't find MediaPlayer.mNativeContext");
		return;
	}

	fields.post_event = env->GetStaticMethodID(clazz, "postEventFromNative",
			"(Ljava/lang/Object;IIILjava/lang/Object;)V");
	if (fields.post_event == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "Can't find MediaPlayer.postEventFromNative");
		return;
	}

	fields.surface = env->GetFieldID(clazz, "mSurface", "Landroid/view/Surface;");
	if (fields.surface == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "Can't find MediaPlayer.mSurface");
		return;
	}

	jclass surface = env->FindClass("android/view/Surface");
	if (surface == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "Can't find android/view/Surface");
		return;
	}
	
	const char* SURFACEID = NULL;    

	if(!strncmp(gPlatformInfo->release_version,"2.1",3))
	{
		SURFACEID = "mSurface";
	}
	else if(!strncmp(gPlatformInfo->release_version,"2.2",3))
	{
		SURFACEID = "mSurface";
	}
	else if(!strncmp(gPlatformInfo->release_version,"2.3",3))
	{
		SURFACEID = "mNativeSurface";
	}
	else
	{
		SURFACEID = "mNativeSurface";
	}

	fields.surface_native = env->GetFieldID(surface, SURFACEID, "I");
	if (fields.surface_native == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "Can't find Surface.mSurface");
		return;
	}

}

static
void* loadLibrary(const char* libPath) 
{
	LOGD("Before Load lib %s", libPath);
	void* lib_handle = dlopen(libPath, RTLD_NOW);

	if (lib_handle != NULL) 
	{
		LOGI("Load lib %s success", libPath);
	}
	else
	{
		LOGI("Load lib %s error: %s", libPath, dlerror());
	}

	return lib_handle;
}

static
bool loadPPBoxLib()
{
	// load ppbox lib
	if (gPlatformInfo->ppbox == NULL) {
		LOGD("Before PPBox Library Load.");
		char* libPath = NULL;
		libPath = vstrcat(gPlatformInfo->app_path, "lib/", gPlatformInfo->ppbox_lib_name);
		bool ret = PPBoxHandle_Create((PPBoxHandle**)&(gPlatformInfo->ppbox), libPath);
		free(libPath);
		LOGD("After Load Library.");
		return ret;
	}

	return true;
}

static void unloadPlayerLib(void** handler)
{
	LOGD("Start unloading player lib");
	if(*handler != NULL) 
	{
		dlclose(*handler);
		*handler = NULL;
	}
}

static
bool loadPlayerLib(bool generalPlayer)
{
	void** player_handle = NULL;
	if(generalPlayer)
	{
		if(player_handle_hardware)
		{
			unloadPlayerLib(&player_handle_hardware);
		}
		player_handle = &player_handle_software;
	}
	else
	{
		if(player_handle_software)
		{
			unloadPlayerLib(&player_handle_software);
		}
		player_handle = &player_handle_hardware;
	}

	if(*player_handle && getPlayerFun) return true;

	LOGD("Before Player Library Load.");
	char* libPath = NULL;

	// load player lib
	if(*player_handle == NULL)
	{
		if(generalPlayer)
		{
			uint64_t cpuFeatures = android_getCpuFeatures();
			const char* libName = getCodecLibName(cpuFeatures);

			//try to load from app lib path
			char* libPath = vstrcat(gPlatformInfo->app_path, "lib/", libName);
			LOGI("Load lib %s", libPath);
			*player_handle = loadLibrary(libPath);
			free(libPath);

			if (*player_handle == NULL)
			{
				//then try to load from app player path
				//libPathTmp = addstr(gPlatformInfo->app_path, "player/lib/");
				//libPath = addstr(libPathTmp, libName);
				libPath = vstrcat(gPlatformInfo->app_path, "player/lib/", libName);
				LOGI("Load lib %s", libPath);
				*player_handle = loadLibrary(libPath);
				free(libPath);

				//ppbox load system/lib
				if (*player_handle == NULL)
				{
					libPath = vstrcat("/system/lib/", libName);
					LOGI("Load lib %s", libPath);
					*player_handle = loadLibrary(libPath);
					free(libPath);
				}


				if (*player_handle == NULL)
				{
					//then use armv6_vfp codec
					libPath = vstrcat(gPlatformInfo->app_path, "lib/libplayer_v6_vfp.so");
					LOGI("Load lib %s", libPath);
					*player_handle = loadLibrary(libPath);
					free(libPath);

					if (NULL == *player_handle)
					{
						return false;
					}
				}
			}
		}
		else
		{
			const char* libName = NULL;
			if (!strncmp(gPlatformInfo->release_version, "4.0", strlen("4.0"))) {
				libName = "lib/libppplayer_a14.so";
			} else {
				libName = "lib/libppplayer.so";
			}

			libPath = vstrcat(gPlatformInfo->app_path, libName);
			LOGI("Load lib %s", libPath);
			*player_handle = loadLibrary(libPath);
			free(libPath);

			if (NULL == *player_handle)
			{
				return false;
			}
		}
	}
	LOGD("After Load library.");

	LOGD("Before init getPlayer()");

	getPlayerFun = (GET_PLAYER_FUN)dlsym(*player_handle, "getPlayer");
	if (getPlayerFun == NULL) {
		LOGE("Init getPlayer() failed: %s", dlerror());
		return false;
	}

	LOGD("After init getPlayer()");

	return true;
}

static bool loadLibraies(bool generalPlayer)
{
	bool ret = loadPlayerLib(generalPlayer);

	if (!generalPlayer)
	{
		ret = ret && loadPPBoxLib();
	}

	return ret;
}

static 
void android_media_MediaPlayer_native_setup(JNIEnv *env, jobject thiz, jobject weak_this, bool generalPlayer)
{
	LOGV("native_setup");

	if (!loadLibraies(generalPlayer)) 
	{
		jniThrowException(env, "java/lang/RuntimeException", "Load Library Failed!!!");
		return;
	}

	if(getPlayerFun == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "\"getPlayer()\" method init failed.");
		return;
	}

	IPlayer* mp = getPlayerFun((void*)gPlatformInfo);
	if (mp == NULL) {
		jniThrowException(env, "java/lang/RuntimeException", "Create IPlayer failed.");
		return;
	}

	// create new listener and give it to MediaPlayer
	JNIMediaPlayerListener* listener = new JNIMediaPlayerListener(env, thiz, weak_this);
	//IPlayer takes responsibility to release listener.
	mp->setListener(listener);

	// Stow our new C++ MediaPlayer in an opaque field in the Java object.
	setMediaPlayer(env, thiz, mp);
	//check if it needs to release old mediaplayer.
}

static 
void android_media_MediaPlayer_release(JNIEnv *env, jobject thiz)
{
	LOGD("++++++++ release 1");
	IPlayer* mp = setMediaPlayer(env, thiz, 0);
	LOGD("++++++++ release 2");
	if (mp != NULL)
	{
	    if(gPlatformInfo->javaSurface != NULL)
        {   
            env->DeleteGlobalRef(gPlatformInfo->javaSurface);
            gPlatformInfo->javaSurface = NULL;
        }
		delete mp;
		LOGD("++++++++ release 5");
	}
}

static 
void android_media_MediaPlayer_native_finalize(JNIEnv *env, jobject thiz)
{
	LOGV("native_finalize");
	android_media_MediaPlayer_release(env, thiz);
}

static jint
android_media_MediaPlayer_snoop(JNIEnv* env, jobject thiz, jobject data, jint kind) {
	jshort* ar = (jshort*)env->GetPrimitiveArrayCritical((jarray)data, 0);
	jsize len = env->GetArrayLength((jarray)data);
	int ret = 0;
	if (ar) {
		// roger
		// ret = MediaPlayer::snoop(ar, len, kind);
		// env->ReleasePrimitiveArrayCritical((jarray)data, ar, 0);
	}
	return ret;
}

static jint
android_media_MediaPlayer_native_suspend_resume(
		JNIEnv *env, jobject thiz, jboolean isSuspend) {

	LOGD("suspend_resume(%d)", isSuspend);
	IPlayer* mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return UNKNOWN_ERROR;
	}

	return isSuspend ? mp->suspend() : mp->resume();
}

// ----------------------------------------------------------------------------

enum DeviceCompability
{
	UNKNOWN,
	SUPPORTED,
	UNSUPPORTED,
};

class CompatibilityTestHelper: public MediaPlayerListener
{
	public:
		CompatibilityTestHelper()
		{
			LOGD("CompatibilityTestHelper constructor");
			mVideoTestComplete = false;
			mAudioTestComplete = false;
			mIsVideoCompatible = false;
			mIsAudioCompatible = false;
		}

		~CompatibilityTestHelper()
		{
			LOGD("CompatibilityTestHelper destructor");
		}

		void notify(int msg, int ext1, int ext2)
		{
			switch (msg) {
				case MEDIA_COMPATIBILITY_TEST_COMPLETE:
					if (ext1 == 0) //video
					{
						//Awesomeplayer thread runs here.
						LOGI("Awesomeplayer test complete result: %d", ext2);
						if(ext2 == true)
						{
							mIsVideoCompatible = true;
						}
						mVideoTestComplete = true;
						Mutex::Autolock autoLock(mLock);
						mConditionVideoTestComplete.signal();
					}
					else if(ext1 == 1) //audio
					{
						//Audioplayer thread runs here.
						LOGI("Audioplayer test complete result: %d", ext2);
						if(ext2 == true)
						{
							mIsAudioCompatible = true;
						}
						mAudioTestComplete = true;
						Mutex::Autolock autoLock(mLock);
						mConditionAudioTestComplete.signal();
					}
					else
					{
						//do nothing.
					}
				default:
					return;
			}
		}

		bool waitComplete()
		{
			//UI thread blocks here.
			int timePassed = 0; //in micro sec.
			int timeOut = 5000; //10 sec. generally test costs less than 1sec. 
			while(!mVideoTestComplete && timePassed < timeOut)
			{
				Mutex::Autolock autoLock(mLock);
				mConditionVideoTestComplete.waitRelative(mLock, 1000000000);//1sec
				timePassed += 1000;
			}

			while(!mAudioTestComplete && timePassed < timeOut)
			{
				Mutex::Autolock autoLock(mLock);
				mConditionAudioTestComplete.waitRelative(mLock, 1000000000);//1sec
				timePassed += 1000;
			}

			if(mIsVideoCompatible && mIsAudioCompatible)
				return true;
			else
				return false;
		}

		DeviceCompability ReadDeviceCompatibilityResult(bool run)
		{
			//return SUPPORTED;

			if(run)
			{
				//set device to be incompatible until device is tested as success, then update it.
				//SaveDeviceCompatibilityResult(false);
				return UNKNOWN;
			}
			else
			{
				FILE *pFile;
				const char fileName[] = "player.data";

				char szData[100] = {0};
				bzero(szData, 100);
				char* filePath = vstrcat(gPlatformInfo->app_path, fileName);

				LOGD("open file %s", filePath);

				pFile = fopen(filePath, "r");
				free(filePath);

				if(pFile == NULL)
				{
					LOGE("open file %s failed", filePath);
					return UNSUPPORTED;
				}
				LOGD("open file %s success", filePath);

				fread(szData, 1, 100, pFile);

				// Close file
				fclose(pFile);

				if(!strncmp(szData, "DeviceCompatibilityResult:1", 27))
				{
					return SUPPORTED;
				}
				else if(!strncmp(szData, "DeviceCompatibilityResult:0", 27))
				{
					return UNSUPPORTED;
				}
				else
				{
					return UNSUPPORTED;
				}
			}

		}

		void SaveDeviceCompatibilityResult(bool isDeviceCompatible)
		{
			//todo:

			FILE *pFile;
			const char fileName[] = "player.data";
			char* filePath = vstrcat(gPlatformInfo->app_path, fileName);

			LOGD("open file %s", filePath);

			pFile=fopen(filePath, "w");
			if(pFile==NULL)
			{
				LOGE("open file %s failed", filePath);
				return;
			}

			LOGD("open file %s success", filePath);

			char szData[100] = {0};
			bzero(szData, 100);

			LOGD("SaveDeviceCompatibilityResult:%d", isDeviceCompatible);

			if(isDeviceCompatible)
			{
				sprintf(szData, "DeviceCompatibilityResult:1");
			}
			else
			{
				sprintf(szData, "DeviceCompatibilityResult:0");
			}

			fwrite(szData, 1, 27, pFile);

			// Close file
			fclose(pFile);
		}

	private:
		bool mVideoTestComplete;
		bool mAudioTestComplete;
		bool mIsVideoCompatible;
		bool mIsAudioCompatible;

		Mutex mLock;
		Condition mConditionVideoTestComplete;
		Condition mConditionAudioTestComplete;
};

static bool native_checkCompatibility_hardware(Surface* native_surface)
{
	if (native_surface == NULL)
	{
		LOGE("Surface is NULL for test");
		return false;
	}

	// TODO: need to support other version
	if(strncmp(gPlatformInfo->release_version, "2.3", strlen("2.3")) != 0 &&
			strncmp(gPlatformInfo->release_version, "2.2", strlen("2.2")) != 0 &&
			strncmp(gPlatformInfo->release_version, "4.0", strlen("4.0")) != 0  )
	{
		LOGE("Incompatible android version:%s", gPlatformInfo->release_version);
		return false;
	}

	//IPlayer takes responsibility to release test helper.
	CompatibilityTestHelper* helper = new CompatibilityTestHelper();

	//init
	if(!loadPlayerLib(false)) return false;

	IPlayer* mp = getPlayerFun((void*)gPlatformInfo);

	if(mp == NULL) return false;

	//check audio/video decoder
	mp->setVideoSurface((void*)native_surface);

	char* samplePath = vstrcat(gPlatformInfo->app_path, "lib/libsample.so");
	LOGI("Load lib %s", samplePath);
	mp->setDataSource(samplePath);

	free(samplePath);

	mp->setListener(helper);
	if(mp->prepare() != OK)
	{
		LOGE("checkCompatibility prepare failed");
		goto ERROR;
	}
	//this could be blocked, the caller should check its timeout.
	if(mp->startCompatibilityTest() != OK)
	{
		LOGE("checkCompatibility startCompatibilityTest failed");
		goto ERROR;
	}

	//wait for testing result in limited time.
	if(!(helper->waitComplete()))
	{
		LOGE("checkCompatibility waitComplete failed");
		goto ERROR;
	}

	//uninit, mp will be destoryed automatically.
	mp->stopCompatibilityTest();

	//save test result
	//helper->SaveDeviceCompatibilityResult(true);

	//release mediaplayer
	mp->stop();
	LOGD("Delete IPlayer");
	delete mp;

	return true;//OK

ERROR:

	LOGD("Delete IPlayer");
	delete mp;
	//mp->stopCompatibilityTest();
	unloadPlayerLib(&player_handle_hardware);
	//save test result
	//helper->SaveDeviceCompatibilityResult(false);
	return false;
}

static bool native_checkCompatibility_software()
{
	bool ret = false;
	uint64_t cpuFeatures = android_getCpuFeatures();
	if ((cpuFeatures & ANDROID_CPU_ARM_FEATURE_NEON) != 0 ||
			(cpuFeatures & ANDROID_CPU_ARM_FEATURE_ARMv7) != 0 ||
			(cpuFeatures & ANDROID_CPU_ARM_FEATURE_VFP) != 0)
	{
		if(loadPlayerLib(true))
		{
			IPlayer* mp = getPlayerFun((void*)gPlatformInfo);
			if(mp != NULL)
			{
				ret = (mp->startCompatibilityTest()==OK)?true:false;
				delete mp;
			}
		}
	}
	return ret;
}

static int COMPATIBILITY_HARDWARE_DECODE = 1;
static int COMPATIBILITY_SOFTWARE_DECODE = 2;

static 
jboolean android_media_MediaPlayer_native_checkCompatibility(JNIEnv *env, jobject thiz, jint checkWhat, jobject jsurface)
{
	if(checkWhat == COMPATIBILITY_HARDWARE_DECODE)
	{
		Surface* native_surface = get_surface(env, jsurface);
		return native_checkCompatibility_hardware(native_surface);
	}
	else if(checkWhat == COMPATIBILITY_SOFTWARE_DECODE)
	{
		return native_checkCompatibility_software();
	}
	else
	{
		return false;
	}
}


#define LEVEL_HARDWARE              1
#define LEVEL_SOFTWARE_LIUCHANG     2
#define LEVEL_SOFTWARE_GAOQING      3
#define LEVEL_SOFTWARE_CHAOQING     4
#define LEVEL_SOFTWARE_LANGUANG     5
static
jint android_media_MediaPlayer_native_checkSoftwareDecodeLevel()
{
	uint64_t cpuFeatures = android_getCpuFeatures();

    //non-NEON arch
    int level = LEVEL_HARDWARE;
	if ((cpuFeatures & ANDROID_CPU_ARM_FEATURE_NEON) != 0)
	{
	    //neon arch
        int cpuCount = android_getCpuCount();
        int cpuFreq = android_getCpuFreq();

        if(cpuCount >= 4)// 4 cores
        {
            level = LEVEL_SOFTWARE_LANGUANG;
        }
        else if(cpuCount >= 2 && cpuFreq >= 1400000) // cpu:1.4G, 2 cores
        {
            level = LEVEL_SOFTWARE_CHAOQING;
        }
        else if(cpuCount >= 1 && cpuFreq >= 1000000)
        {
            level = LEVEL_SOFTWARE_GAOQING;
        }
        else
        {
            level = LEVEL_SOFTWARE_LIUCHANG;
        }
    }
	return level;
}

static 
int accessCodec(const char* app_root, const char* lib_name)
{
	char* libPath = NULL;
	int canRead;

	libPath = vstrcat(app_root, "player/lib/", lib_name);
	canRead = access(libPath, R_OK);
	free(libPath);

	if (canRead != 0)
	{
		libPath = vstrcat(app_root, "lib/" , lib_name);
		canRead = access(libPath, R_OK);
		free(libPath);
	}

	return canRead;

}

static 
jstring android_media_MediaPlayer_getBestCodec(JNIEnv *env, jobject thiz, jstring applicationPath)
{
	//char* appPath = (char*)env->GetStringUTFChars(applicationPath, NULL);
	const char* appPath = NULL;
	appPath = (char*)env->GetStringUTFChars(applicationPath, NULL);
	if (appPath == NULL)
	{
		appPath = "/"; //for not crash programe
	}

	uint64_t cpuFeatures = android_getCpuFeatures();
	const char* codecName = getCodecLibName(cpuFeatures);
	if (accessCodec(appPath, codecName) == 0)
	{
		codecName = "";
	}

	return env->NewStringUTF(codecName); 
}

static jboolean
android_media_MediaPlayer_native_getMediaInfo(JNIEnv *env, jobject thiz, jstring js_media_file_path, jobject info)
{
	bool ret = false;
	if (loadPlayerLib(true))
	{
		IPlayer* mp = getPlayerFun((void*)gPlatformInfo);
		if (mp == NULL)
		{
			LOGE("Player init failed.");
			return false;
		}

		const char* url = jstr2cstr(env, js_media_file_path);		
		MediaInfo native_info;
		ret = mp->getMediaInfo(url, &native_info);
		if (!ret)
		{
			LOGE("Get MediaInfo failed: %s", url);
		}
		else
		{
			LOGD("Get MediaInfo succeed.");
			jclass clazz = env->FindClass("android/pplive/media/player/MediaInfo");
			jfieldID f_path = env->GetFieldID(clazz, "mPath", "Ljava/lang/String;");
			jfieldID f_duration = env->GetFieldID(clazz, "mDurationMS", "J");
			jfieldID f_size = env->GetFieldID(clazz, "mSizeByte", "J");
			//jfieldID f_audio_channels = env->GetFieldID(clazz, "mAudioChannels", "I");
			//jfieldID f_video_channels = env->GetFieldID(clazz, "mVideoChannels", "I");

			env->SetObjectField(info, f_path, js_media_file_path);
			env->SetLongField(info, f_duration, native_info.duration_ms);
			env->SetLongField(info, f_size, native_info.size_byte);
			//env->SetIntField(info, f_audio_channels, native_info.audio_channels);
			//env->SetIntField(info, f_video_channels, native_info.video_channels);
		}
		
		if (mp != NULL) 
		{
			delete mp;
			mp = NULL;
		}
	}

	return ret;
}

static jboolean
android_media_MediaPlayer_native_getMediaDetailInfo(JNIEnv *env, jobject thiz, jstring js_media_file_path, jobject info)
{
	bool ret = false;
	if (loadPlayerLib(true))
	{
		IPlayer* mp = getPlayerFun((void*)gPlatformInfo);
		if (mp == NULL)
		{
			LOGE("Player init failed.");
			return false;
		}

		const char* url = jstr2cstr(env, js_media_file_path);		
		MediaInfo native_info;
		if(mp->getMediaDetailInfo(url, &native_info))
		{
			jclass clazz = env->FindClass("android/pplive/media/player/MediaInfo");
			jfieldID f_path = env->GetFieldID(clazz, "mPath", "Ljava/lang/String;");
			jfieldID f_duration = env->GetFieldID(clazz, "mDurationMS", "J");
			jfieldID f_size = env->GetFieldID(clazz, "mSizeByte", "J");
			jfieldID f_width = env->GetFieldID(clazz, "mWidth", "I");
			jfieldID f_height = env->GetFieldID(clazz, "mHeight", "I");
			jfieldID f_format = env->GetFieldID(clazz, "mFormatName", "Ljava/lang/String;");
			jfieldID f_audio = env->GetFieldID(clazz, "mAudioName", "Ljava/lang/String;");
			jfieldID f_video = env->GetFieldID(clazz, "mVideoName", "Ljava/lang/String;");
			jfieldID f_audio_channels = env->GetFieldID(clazz, "mAudioChannels", "I");
			jfieldID f_video_channels = env->GetFieldID(clazz, "mVideoChannels", "I");

			env->SetObjectField(info, f_path, js_media_file_path);
			env->SetLongField(info, f_duration, native_info.duration_ms);
			env->SetLongField(info, f_size, native_info.size_byte);
			env->SetIntField(info, f_width, native_info.width);
			env->SetIntField(info, f_height, native_info.height);
			env->SetObjectField(info, f_format, env->NewStringUTF(native_info.format_name));
			env->SetObjectField(info, f_audio, env->NewStringUTF(native_info.audio_name));
			env->SetObjectField(info, f_video, env->NewStringUTF(native_info.video_name));
			env->SetIntField(info, f_audio_channels, native_info.audio_channels);
			env->SetIntField(info, f_video_channels, native_info.video_channels);
            ret = true;
			LOGD("Get MediaDetailInfo succeed.");
		}
        else
        {
			LOGE("Get MediaDetailInfo failed.");
        }
		
		if (mp != NULL) 
		{
			delete mp;
			mp = NULL;
		}
	}

	return ret;
}

static jboolean
android_media_MediaPlayer_native_getThumbnail(JNIEnv *env, jobject thiz, jstring js_media_file_path, jobject info)
{
	bool ret = false;
	if (loadPlayerLib(true))
	{
		IPlayer* mp = getPlayerFun((void*)gPlatformInfo);
		if (mp == NULL)
		{
			LOGE("Player init failed.");
			return false;
		}

		const char* url = jstr2cstr(env, js_media_file_path);		
		MediaInfo native_info;
		bool isSuccess = mp->getThumbnail(url, &native_info);
		if (!isSuccess || native_info.thumbnail == NULL)
		{
		    if(native_info.thumbnail != NULL)
            {      
                free(native_info.thumbnail);
                native_info.thumbnail = NULL;
            }
			LOGE("Get Thumbnail failed: %s", url);
		}
		else
		{
			jclass clazz = env->FindClass("android/pplive/media/player/MediaInfo");
			jfieldID f_path = env->GetFieldID(clazz, "mPath", "Ljava/lang/String;");
			jfieldID f_duration = env->GetFieldID(clazz, "mDurationMS", "J");
			jfieldID f_size = env->GetFieldID(clazz, "mSizeByte", "J");
			jfieldID f_width = env->GetFieldID(clazz, "mWidth", "I");
			jfieldID f_height = env->GetFieldID(clazz, "mHeight", "I");
			jfieldID f_format = env->GetFieldID(clazz, "mFormatName", "Ljava/lang/String;");
			jfieldID f_audio = env->GetFieldID(clazz, "mAudioName", "Ljava/lang/String;");
			jfieldID f_video = env->GetFieldID(clazz, "mVideoName", "Ljava/lang/String;");
			jfieldID f_thumbnailwidth = env->GetFieldID(clazz, "mThumbnailWidth", "I");
			jfieldID f_thumbnailheight = env->GetFieldID(clazz, "mThumbnailHeight", "I");
			jfieldID f_thumbnail = env->GetFieldID(clazz, "mThumbnail", "[I");
			jfieldID f_audio_channels = env->GetFieldID(clazz, "mAudioChannels", "I");
			jfieldID f_video_channels = env->GetFieldID(clazz, "mVideoChannels", "I");

			env->SetObjectField(info, f_path, js_media_file_path);
			env->SetLongField(info, f_duration, native_info.duration_ms);
			env->SetLongField(info, f_size, native_info.size_byte);
			env->SetIntField(info, f_width, native_info.width);
			env->SetIntField(info, f_height, native_info.height);
			env->SetObjectField(info, f_format, env->NewStringUTF(native_info.format_name));
			env->SetObjectField(info, f_audio, env->NewStringUTF(native_info.audio_name));
			env->SetObjectField(info, f_video, env->NewStringUTF(native_info.video_name));
            
			env->SetIntField(info, f_thumbnailwidth, native_info.thumbnail_width);
			env->SetIntField(info, f_thumbnailheight, native_info.thumbnail_height);
            int size = native_info.thumbnail_width*native_info.thumbnail_height;
            jintArray thumbnail = env->NewIntArray(size);
            env->SetIntArrayRegion(thumbnail,0, size, native_info.thumbnail);
		    env->SetObjectField(info, f_thumbnail, thumbnail);
            LOGD("get thumbnail success");
            free(native_info.thumbnail);
            native_info.thumbnail = NULL;
            
			env->SetIntField(info, f_audio_channels, native_info.audio_channels);
			env->SetIntField(info, f_video_channels, native_info.video_channels);
            ret = true;
		}
		
		if (mp != NULL) 
		{
			delete mp;
			mp = NULL;
		}
	}

	return ret;
}

static
jint android_media_MediaPlayer_native_getCpuArchNumber()
{
	return android_getCpuArchNumber();
}

// ----------------------------------------------------------------------------

static JNINativeMethod gMethods[] = {
	{"setDataSource_",       "(Ljava/lang/String;)V",		(void *)android_media_MediaPlayer_setDataSource},
	{"setDataSource",       "(Ljava/lang/String;Ljava/util/Map;)V",	(void *)android_media_MediaPlayer_setDataSourceAndHeaders},
	{"setDataSource",       "(Ljava/io/FileDescriptor;JJ)V",	(void *)android_media_MediaPlayer_setDataSourceFD},
	{"_setVideoSurface",    "()V",					(void *)android_media_MediaPlayer_setVideoSurface},
	{"prepare",             "()V",					(void *)android_media_MediaPlayer_prepare},
	{"prepareAsync",        "()V",					(void *)android_media_MediaPlayer_prepareAsync},
	{"_start",              "()V",					(void *)android_media_MediaPlayer_start},
	{"_stop",               "()V",					(void *)android_media_MediaPlayer_stop},
	{"getVideoWidth",       "()I",					(void *)android_media_MediaPlayer_getVideoWidth},
	{"getVideoHeight",      "()I",					(void *)android_media_MediaPlayer_getVideoHeight},
	{"seekTo",              "(I)V",					(void *)android_media_MediaPlayer_seekTo},
	{"_pause",              "()V",					(void *)android_media_MediaPlayer_pause},
	{"isPlaying",           "()Z",					(void *)android_media_MediaPlayer_isPlaying},
	{"getCurrentPosition",  "()I",					(void *)android_media_MediaPlayer_getCurrentPosition},
	{"getDuration",         "()I",					(void *)android_media_MediaPlayer_getDuration},
	{"_release",            "()V",					(void *)android_media_MediaPlayer_release},
	{"_reset",              "()V",					(void *)android_media_MediaPlayer_reset},
	{"setAudioStreamType",  "(I)V",					(void *)android_media_MediaPlayer_setAudioStreamType},
	{"setLooping",          "(Z)V",					(void *)android_media_MediaPlayer_setLooping},
	{"isLooping",           "()Z",					(void *)android_media_MediaPlayer_isLooping},
	{"setVolume",           "(FF)V",				(void *)android_media_MediaPlayer_setVolume},
	{"getFrameAt",          "(I)Landroid/graphics/Bitmap;",		(void *)android_media_MediaPlayer_getFrameAt},
	{"native_invoke",       "(Landroid/os/Parcel;Landroid/os/Parcel;)I",(void *)android_media_MediaPlayer_invoke},
	{"native_setMetadataFilter", "(Landroid/os/Parcel;)I",		(void *)android_media_MediaPlayer_setMetadataFilter},
	{"native_getMetadata", "(ZZLandroid/os/Parcel;)Z",		(void *)android_media_MediaPlayer_getMetadata},
	{"flags",               "()I",					(void *)android_media_MediaPlayer_flags},
	{"native_init",         "(Z)V",					(void *)android_media_MediaPlayer_native_init},
	{"native_setup",        "(Ljava/lang/Object;Z)V",		(void *)android_media_MediaPlayer_native_setup},
	{"native_finalize",     "()V",					(void *)android_media_MediaPlayer_native_finalize},
	{"snoop",               "([SI)I",				(void *)android_media_MediaPlayer_snoop},
	{"native_suspend_resume", "(Z)I",				(void *)android_media_MediaPlayer_native_suspend_resume},
	{"native_checkCompatibility","(ILandroid/view/Surface;)Z",	(void *)android_media_MediaPlayer_native_checkCompatibility},
	{"getBestCodec",        "(Ljava/lang/String;)Ljava/lang/String;",(void *)android_media_MediaPlayer_getBestCodec},
	{"native_getMediaInfo",	"(Ljava/lang/String;Landroid/pplive/media/player/MediaInfo;)Z",(void *)android_media_MediaPlayer_native_getMediaInfo},
	{"native_getMediaDetailInfo",	"(Ljava/lang/String;Landroid/pplive/media/player/MediaInfo;)Z",(void *)android_media_MediaPlayer_native_getMediaDetailInfo},
	{"native_getThumbnail",	"(Ljava/lang/String;Landroid/pplive/media/player/MediaInfo;)Z",(void *)android_media_MediaPlayer_native_getThumbnail},
	{"native_checkSoftwareDecodeLevel",	"()I",(void *)android_media_MediaPlayer_native_checkSoftwareDecodeLevel},
	{"native_getCpuArchNumber",	"()I",(void *)android_media_MediaPlayer_native_getCpuArchNumber},
};

static const char* const kClassPathName = "android/media/MediaPlayer";

// This function only registers the native methods
static int register_android_media_MediaPlayer(JNIEnv *env)
{
	return jniRegisterNativeMethodsPP(env,
			"android/pplive/media/player/NativeMediaPlayer", gMethods, NELEM(gMethods));

}

//extern int register_android_media_MediaMetadataRetriever(JNIEnv *env);
//extern int register_android_media_MediaRecorder(JNIEnv *env);
//extern int register_android_media_MediaScanner(JNIEnv *env);
//extern int register_android_media_ResampleInputStream(JNIEnv *env);
//extern int register_android_media_MediaProfiles(JNIEnv *env);
//extern int register_android_hardware_Camera(JNIEnv *env);

//#ifndef NO_OPENCORE
//extern int register_android_media_AmrInputStream(JNIEnv *env);
//#endif

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv* env = NULL;
	jint result = -1;

	LOGI("JNI_OnLoad");

	if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
		LOGE("ERROR: GetEnv failed");
		goto bail;
	}

	if (env == NULL) {
		goto bail;
	}

	if (register_android_media_MediaPlayer(env) < 0) {
		LOGE("ERROR: MediaPlayer native registration failed");
		goto bail;
	} else {

	}

	//save jvm for multiple thread invoking to java application.
	gs_jvm = vm; 

	/* success -- return valid version number */
	result = JNI_VERSION_1_4;

bail:
	return result;
}

void JNI_OnUnload(JavaVM* vm, void* reserved)
{
	LOGI("JNI_OnUnload");
	//stop p2p engine.
	//PPDataSource::releaseInstance();
	//LOGE("JNI_OnUnload");
	unloadPlayerLib(&player_handle_hardware);
	unloadPlayerLib(&player_handle_software);

	if (gPlatformInfo != NULL) {
		delete gPlatformInfo;
		gPlatformInfo = NULL;
	}

}

// KTHXBYE
