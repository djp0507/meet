#define LOG_TAG "PPMediaExtractor-JNI"

#include "PPMediaExtractor.h"

#include "include-pp/a16/ABuffer.h"
#include "include-pp/a16/AMessage.h"
#include "include-pp/a16/AString.h"

#include "include-pp/a16/MetaData.h"
#include "include-pp/a16/NuPPMediaExtractor.h"

#include "include-pp/nativehelper/JNIHelp.h"
#include "include-pp/cutils/log.h"

#include "include-pp/PPBox_Util.h"
#include "platform/platforminfo.h"

namespace android {

static JavaVM *gJavaVM = NULL;

PlatformInfo* gPlatformInfo = NULL;

struct field_t {
	jfieldID context;
};

static field_t gFields;

/////////////////////////////////////////////////////////////////

static jobject makeIntegerObject(JNIEnv *env, int32_t value) {
    jclass clazz = env->FindClass("java/lang/Integer");
    CHECK(clazz != NULL);

    jmethodID integerConstructID = env->GetMethodID(clazz, "<init>", "(I)V");
    CHECK(integerConstructID != NULL);

    return env->NewObject(clazz, integerConstructID, value);
}

static jobject makeLongObject(JNIEnv *env, int64_t value) {
    jclass clazz = env->FindClass("java/lang/Long");
    CHECK(clazz != NULL);

    jmethodID longConstructID = env->GetMethodID(clazz, "<init>", "(J)V");
    CHECK(longConstructID != NULL);

    return env->NewObject(clazz, longConstructID, value);
}

static jobject makeFloatObject(JNIEnv *env, float value) {
    jclass clazz = env->FindClass("java/lang/Float");
    CHECK(clazz != NULL);

    jmethodID floatConstructID = env->GetMethodID(clazz, "<init>", "(F)V");
    CHECK(floatConstructID != NULL);

    return env->NewObject(clazz, floatConstructID, value);
}

static jobject makeByteBufferObject(
        JNIEnv *env, const void *data, size_t size) {
    jbyteArray byteArrayObj = env->NewByteArray(size);
    env->SetByteArrayRegion(byteArrayObj, 0, size, (const jbyte *)data);

    jclass clazz = env->FindClass("java/nio/ByteBuffer");
    CHECK(clazz != NULL);

    jmethodID byteBufWrapID =
        env->GetStaticMethodID(clazz, "wrap", "([B)Ljava/nio/ByteBuffer;");
    CHECK(byteBufWrapID != NULL);

    jobject byteBufObj = env->CallStaticObjectMethod(
            clazz, byteBufWrapID, byteArrayObj);

    env->DeleteLocalRef(byteArrayObj); byteArrayObj = NULL;

    return byteBufObj;
}


static void SetMapInt32(
        JNIEnv *env, jobject hashMapObj, jmethodID hashMapPutID,
        const char *key, int32_t value) {
    jstring keyObj = env->NewStringUTF(key);
    jobject valueObj = makeIntegerObject(env, value);

    jobject res = env->CallObjectMethod(
            hashMapObj, hashMapPutID, keyObj, valueObj);

    env->DeleteLocalRef(valueObj); valueObj = NULL;
    env->DeleteLocalRef(keyObj); keyObj = NULL;
}


status_t ConvertMessageToMap(
        JNIEnv *env, const sp<AMessage> &msg, jobject *map) {
    jclass hashMapClazz = env->FindClass("java/util/HashMap");

    if (hashMapClazz == NULL) {
        return -EINVAL;
    }

    jmethodID hashMapConstructID =
        env->GetMethodID(hashMapClazz, "<init>", "()V");

    if (hashMapConstructID == NULL) {
        return -EINVAL;
    }

    jmethodID hashMapPutID =
        env->GetMethodID(
                hashMapClazz,
                "put",
                "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

    if (hashMapPutID == NULL) {
        return -EINVAL;
    }

    jobject hashMap = env->NewObject(hashMapClazz, hashMapConstructID);

    for (size_t i = 0; i < msg->countEntries(); ++i) {
        AMessage::Type valueType;
        const char *key = msg->getEntryNameAt(i, &valueType);

        jobject valueObj = NULL;

        switch (valueType) {
            case AMessage::kTypeInt32:
            {
                int32_t val;
                CHECK(msg->findInt32(key, &val));

                valueObj = makeIntegerObject(env, val);
                break;
            }

            case AMessage::kTypeInt64:
            {
                int64_t val;
                CHECK(msg->findInt64(key, &val));

                valueObj = makeLongObject(env, val);
                break;
            }

            case AMessage::kTypeFloat:
            {
                float val;
                CHECK(msg->findFloat(key, &val));

                valueObj = makeFloatObject(env, val);
                break;
            }

            case AMessage::kTypeString:
            {
                AString val;
                CHECK(msg->findString(key, &val));

                valueObj = env->NewStringUTF(val.c_str());
                break;
            }

            case AMessage::kTypeBuffer:
            {
                sp<ABuffer> buffer;
                CHECK(msg->findBuffer(key, &buffer));

                valueObj = makeByteBufferObject(
                        env, buffer->data(), buffer->size());
                break;
            }

            case AMessage::kTypeRect:
            {
                int32_t left, top, right, bottom;
                CHECK(msg->findRect(key, &left, &top, &right, &bottom));

                SetMapInt32(
                        env,
                        hashMap,
                        hashMapPutID,
                        StringPrintf("%s-left", key).c_str(),
                        left);

                SetMapInt32(
                        env,
                        hashMap,
                        hashMapPutID,
                        StringPrintf("%s-top", key).c_str(),
                        top);

                SetMapInt32(
                        env,
                        hashMap,
                        hashMapPutID,
                        StringPrintf("%s-right", key).c_str(),
                        right);

                SetMapInt32(
                        env,
                        hashMap,
                        hashMapPutID,
                        StringPrintf("%s-bottom", key).c_str(),
                        bottom);
                break;
            }

            default:
                break;
        }

        if (valueObj != NULL) {
            jstring keyObj = env->NewStringUTF(key);

            jobject res = env->CallObjectMethod(
                    hashMap, hashMapPutID, keyObj, valueObj);

            env->DeleteLocalRef(keyObj); keyObj = NULL;
            env->DeleteLocalRef(valueObj); valueObj = NULL;
        }
    }

    *map = hashMap;

    return OK;
}

//////////////////////////////////////////////////////////////

JPPMediaExtractor::JPPMediaExtractor(JNIEnv *env, jobject thiz) 
	: mClass(),
	  mObject()
{
	jclass clazz = env->GetObjectClass(thiz);
	CHECK(clazz != NULL);

	mClass = (jclass)env->NewGlobalRef(clazz);
	mObject = env->NewWeakGlobalRef(thiz);

	mImpl = new NuPPMediaExtractor;
}

JPPMediaExtractor::~JPPMediaExtractor() {
	JNIEnv *env = NULL;
	CHECK(gJavaVM != NULL);
	gJavaVM->GetEnv((void**)&env, JNI_VERSION_1_4);
	CHECK(env != NULL);

    env->DeleteWeakGlobalRef(mObject);
    mObject = NULL;
    env->DeleteGlobalRef(mClass);
    mClass = NULL;
}

status_t JPPMediaExtractor::setDataSource(
        const char *path, const KeyedVector<String8, String8> *headers) {
    return mImpl->setDataSource(path, headers);
}

size_t JPPMediaExtractor::countTracks() const {
    return mImpl->countTracks();
}

status_t JPPMediaExtractor::getTrackFormat(size_t index, jobject *format) const {
	sp<AMessage> msg;
	status_t err;

	if ((err = mImpl->getTrackFormat(index, &msg)) != OK) {
		return err;
	}

	JNIEnv *env = NULL;
	CHECK(gJavaVM != NULL);
	gJavaVM->GetEnv((void**)&env, JNI_VERSION_1_4);
	CHECK(env != NULL);

	return ConvertMessageToMap(env, msg, format);
}

status_t JPPMediaExtractor::selectTrack(size_t index) {
    return mImpl->selectTrack(index);
}

status_t JPPMediaExtractor::unselectTrack(size_t index) {
    return mImpl->unselectTrack(index);
}

status_t JPPMediaExtractor::seekTo(
        int64_t timeUs, MediaSource::ReadOptions::SeekMode mode) {
    return mImpl->seekTo(timeUs, mode);
}

status_t JPPMediaExtractor::advance() {
    return mImpl->advance();
}

status_t JPPMediaExtractor::readSampleData(
		jobject byteBuf, size_t offset, size_t *sampleSize) {
	JNIEnv *env = NULL;
	CHECK(gJavaVM != NULL);
	gJavaVM->GetEnv((void**)&env, JNI_VERSION_1_4);
	CHECK(env != NULL);

	void *dst = env->GetDirectBufferAddress(byteBuf);

	jlong dstSize;
	jbyteArray byteArray = NULL;

    if (dst == NULL) {
        jclass byteBufClass = env->FindClass("java/nio/ByteBuffer");
        CHECK(byteBufClass != NULL);

        jmethodID arrayID =
            env->GetMethodID(byteBufClass, "array", "()[B");
        CHECK(arrayID != NULL);

        byteArray =
            (jbyteArray)env->CallObjectMethod(byteBuf, arrayID);

        if (byteArray == NULL) {
            return INVALID_OPERATION;
        }

        jboolean isCopy;
        dst = env->GetByteArrayElements(byteArray, &isCopy);

        dstSize = env->GetArrayLength(byteArray);
    } else {
        dstSize = env->GetDirectBufferCapacity(byteBuf);
    }

    if (dstSize < offset) {
        if (byteArray != NULL) {
            env->ReleaseByteArrayElements(byteArray, (jbyte *)dst, 0);
        }

        return -ERANGE;
    }

    sp<ABuffer> buffer = new ABuffer((char *)dst + offset, dstSize - offset);

    status_t err = mImpl->readSampleData(buffer);

    if (byteArray != NULL) {
        env->ReleaseByteArrayElements(byteArray, (jbyte *)dst, 0);
    }

    if (err != OK) {
        return err;
    }

    *sampleSize = buffer->size();

    return OK;


	return OK;
}

status_t JPPMediaExtractor::getSampleTrackIndex(size_t *trackIndex) {
    return mImpl->getSampleTrackIndex(trackIndex);
}

status_t JPPMediaExtractor::getSampleTime(int64_t *sampleTimeUs) {
    return mImpl->getSampleTime(sampleTimeUs);
}

status_t JPPMediaExtractor::getSampleFlags(uint32_t *sampleFlags) {
	*sampleFlags = 0;

	sp<MetaData> meta;
    status_t err = mImpl->getSampleMeta(&meta);

    if (err != OK) {
        return err;
    }

    int32_t val;
    if (meta->findInt32(kKeyIsSyncFrame, &val) && val != 0) {
        (*sampleFlags) |= NuPPMediaExtractor::SAMPLE_FLAG_SYNC;
    }

    uint32_t type;
    const void *data;
    size_t size;
    if (meta->findData(kKeyEncryptedSizes, &type, &data, &size)) {
        (*sampleFlags) |= NuPPMediaExtractor::SAMPLE_FLAG_ENCRYPTED;
    }


	return OK;
}

status_t JPPMediaExtractor::getSampleMeta(sp<MetaData> *sampleMeta) {
    return mImpl->getSampleMeta(sampleMeta);
}

bool JPPMediaExtractor::getCachedDuration(int64_t *durationUs, bool *eos) const {
    return mImpl->getCachedDuration(durationUs, eos);
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////


static sp<JPPMediaExtractor> setPPMediaExtractor(
	JNIEnv *env, jobject thiz, const sp<JPPMediaExtractor> &extractor) {
	sp<JPPMediaExtractor> old =
		(JPPMediaExtractor *)env->GetIntField(thiz, gFields.context);

	if (extractor != NULL) {
		extractor->incStrong(thiz);
	}
	if (old != NULL) {
		old->decStrong(thiz);
	}
	
	env->SetIntField(thiz, gFields.context, (int)extractor.get());

	return old;
}

static sp<JPPMediaExtractor> getPPMediaExtractor(JNIEnv *env, jobject thiz) {
    return (JPPMediaExtractor *)env->GetIntField(thiz, gFields.context);
}

__BEGIN_DECLS

JNIEXPORT jboolean JNICALL 
Java_android_pplive_media_player_PPMediaExtractor_advance(JNIEnv *env, jobject thiz) {
	sp<JPPMediaExtractor> extractor = getPPMediaExtractor(env, thiz);

	if (extractor == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }

    status_t err = extractor->advance();

    if (err == ERROR_END_OF_STREAM) {
		return false;
    } else if (err != OK) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return false;
    }

    return true;
}

JNIEXPORT jlong JNICALL 
Java_android_pplive_media_player_PPMediaExtractor_getCachedDuration(JNIEnv *env, jobject thiz) {
    sp<JPPMediaExtractor> extractor = getPPMediaExtractor(env, thiz);

    if (extractor == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1ll;
    }

    int64_t cachedDurationUs;
    bool eos;
    if (!extractor->getCachedDuration(&cachedDurationUs, &eos)) {
        return -1ll;
    }

    return cachedDurationUs;
}


JNIEXPORT jint JNICALL 
Java_android_pplive_media_player_PPMediaExtractor_getSampleFlags(JNIEnv *env, jobject thiz) {
    sp<JPPMediaExtractor> extractor = getPPMediaExtractor(env, thiz);

    if (extractor == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1ll;
    }

    uint32_t sampleFlags;
    status_t err = extractor->getSampleFlags(&sampleFlags);

    if (err == ERROR_END_OF_STREAM) {
        return -1ll;
    } else if (err != OK) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return false;
    }

    return sampleFlags;
}

JNIEXPORT jlong JNICALL 
Java_android_pplive_media_player_PPMediaExtractor_getSampleTime(JNIEnv *env, jobject thiz) {
    sp<JPPMediaExtractor> extractor = getPPMediaExtractor(env, thiz);

    if (extractor == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1ll;
    }

    int64_t sampleTimeUs;
    status_t err = extractor->getSampleTime(&sampleTimeUs);

    if (err == ERROR_END_OF_STREAM) {
        return -1ll;
    } else if (err != OK) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return false;
    }

    return sampleTimeUs;
}

JNIEXPORT jint JNICALL 
Java_android_pplive_media_player_PPMediaExtractor_getSampleTrackIndex(JNIEnv *env, jobject thiz) {
    sp<JPPMediaExtractor> extractor = getPPMediaExtractor(env, thiz);

    if (extractor == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }

    size_t trackIndex = -1;
    status_t err = extractor->getSampleTrackIndex(&trackIndex);
    
    if (err == ERROR_END_OF_STREAM) {
        return -1;
    } else if (err != OK) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return false;
    }

    return trackIndex;
}

JNIEXPORT jint JNICALL 
Java_android_pplive_media_player_PPMediaExtractor_getTrackCount(JNIEnv *env, jobject thiz) {
	LOGE("getTrackCount");
    sp<JPPMediaExtractor> extractor = getPPMediaExtractor(env, thiz);

    if (extractor == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }

    return extractor->countTracks();}

JNIEXPORT jobject JNICALL 
Java_android_pplive_media_player_PPMediaExtractor_getTrackFormatNative(JNIEnv *env, jobject thiz, jint index) {
	LOGE("getTrackFormatNative");
    sp<JPPMediaExtractor> extractor = getPPMediaExtractor(env, thiz);

    if (extractor == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return NULL;
    }

    jobject format;
    status_t err = extractor->getTrackFormat(index, &format);

    if (err != OK) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return NULL;
    }

    return format;
}

JNIEXPORT jboolean JNICALL 
Java_android_pplive_media_player_PPMediaExtractor_hasCachedReachedEndOfStream(JNIEnv *env, jobject thiz) {
    sp<JPPMediaExtractor> extractor = getPPMediaExtractor(env, thiz);

    if (extractor == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return true;
    }

    int64_t cachedDurationUs;
    bool eos = false;
    if (!extractor->getCachedDuration(&cachedDurationUs, &eos)) {
        return true;
    }

    return eos;
}

JNIEXPORT void JNICALL
Java_android_pplive_media_player_PPMediaExtractor_init(JNIEnv *env, jobject thiz) {
	LOGE("init");
	jclass clazz = env->FindClass("android/pplive/media/player/PPMediaExtractor");
	CHECK(clazz != NULL);

	gFields.context = env->GetFieldID(clazz, "mNativeContext", "I");
	CHECK(gFields.context != NULL);
}

/*
static void* gExceptionHandler = NULL;

static void init_breakpad(const char* path)
{
	if (path != NULL && strlen(path) > 0) 
	{
		gPlatformInfo->breakpad->init_breakpad_handler(&gExceptionHandler, path);
	}
}
*/

JNIEXPORT void JNICALL
Java_android_pplive_media_player_PPMediaExtractor_setup(JNIEnv *env, jobject thiz) {
	LOGE("setup");
	sp<JPPMediaExtractor> extractor = new JPPMediaExtractor(env, thiz);
	setPPMediaExtractor(env, thiz, extractor);

	if (gPlatformInfo == NULL) {
	
		gPlatformInfo = new PlatformInfo();
		jclass clazz = env->FindClass("android/pplive/media/MeetSDK");
		if (clazz == NULL)
		{
			jniThrowException(env, "java/lang/RuntimeException", "Can't find android/pplive/media/MeetSDK");
			return;
		}

		jfieldID f_app_root_dir = env->GetStaticFieldID(clazz, "AppRootDir", "Ljava/lang/String;");
		jfieldID f_ppbox_lib_name = env->GetStaticFieldID(clazz, "PPBoxLibName", "Ljava/lang/String;");
		//jfieldID f_breakpad_dump_dir = env->GetStaticFieldID(clazz, "BreakpadDumpDir", "Ljava/lang/String;");

		jstring js_app_root_dir = (jstring)env->GetStaticObjectField(clazz, f_app_root_dir);
		jstring js_ppbox_lib_name = (jstring)env->GetStaticObjectField(clazz, f_ppbox_lib_name);
		//jstring js_breakpad_dump_dir = (jstring)env->GetStaticObjectField(clazz, f_breakpad_dump_dir);
		
		const char* app_root_dir = env->GetStringUTFChars(js_app_root_dir, NULL);
		const char* ppbox_lib_path = env->GetStringUTFChars(js_ppbox_lib_name, NULL);
		//const char* breakpad_dump_dir = env->GetStringUTFChars(js_breakpad_dump_dir, NULL);

		size_t len = strlen(app_root_dir) + strlen("lib/") + strlen(ppbox_lib_path);
		char* libPath = (char*)malloc(len + 1);
		sprintf(libPath, "%s%s%s", app_root_dir, "lib/", ppbox_lib_path);
		bool ret = PPBoxHandle_Create((PPBoxHandle**)&(gPlatformInfo->ppbox), libPath);
		free(libPath);
		if (!ret) 
		{
			jniThrowException(env, "java/lang/RuntimeException", "PPBoxHandle_Create failed.");
			return;
		}

		/*
		len = strlen(app_root_dir) + strlen("lib/") + strlen("libbreakpad_util.so");
		libPath = (char*)malloc(len + 1);
		sprintf(libPath, "%s%s%s", app_root_dir, "lib/", "libbreakpad_util.so");
		ret = BreakpadHandle_Create(&(gPlatformInfo->breakpad), libPath);
		free(libPath);
		if (!ret) 
		{
			jniThrowException(env, "java/lang/RuntimeException", "BreakpadHandle_Create failed.");
			return;
		}
		init_breakpad(breakpad_dump_dir);
		*/
	}
	
}

JNIEXPORT jint JNICALL 
Java_android_pplive_media_player_PPMediaExtractor_readSampleData(JNIEnv *env, jobject thiz, jobject byteBuf, jint offset) {
    sp<JPPMediaExtractor> extractor = getPPMediaExtractor(env, thiz);

    if (extractor == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }

    size_t sampleSize;
    status_t err = extractor->readSampleData(byteBuf, offset, &sampleSize);

    if (err == ERROR_END_OF_STREAM) {
        return -1;
    } else if (err != OK) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return false;
    }

    return sampleSize;
}

JNIEXPORT void JNICALL 
Java_android_pplive_media_player_PPMediaExtractor_release(JNIEnv *env, jobject thiz) {
	setPPMediaExtractor(env, thiz, NULL);
}

JNIEXPORT void JNICALL 
Java_android_pplive_media_player_PPMediaExtractor_seekTo(JNIEnv *env, jobject thiz, jlong timeUs, jint mode) {
    sp<JPPMediaExtractor> extractor = getPPMediaExtractor(env, thiz);

    if (extractor == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    if (mode < MediaSource::ReadOptions::SEEK_PREVIOUS_SYNC
            || mode >= MediaSource::ReadOptions::SEEK_CLOSEST) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return;
    }

    extractor->seekTo(timeUs, (MediaSource::ReadOptions::SeekMode)mode);
}

JNIEXPORT void JNICALL 
Java_android_pplive_media_player_PPMediaExtractor_selectTrack(JNIEnv *env, jobject thiz, jint index) {
	LOGE("selectTrack");
	sp<JPPMediaExtractor> extractor = getPPMediaExtractor(env, thiz);
	
    if (extractor == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    status_t err = extractor->selectTrack(index);

    LOGE("err: %d", err);

    if (err != OK) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return;
    }
}

JNIEXPORT void JNICALL
Java_android_pplive_media_player_PPMediaExtractor_unselectTrack(JNIEnv *env, jobject thiz, jint index) {
	LOGE("unselectTrack");
    sp<JPPMediaExtractor> extractor = getPPMediaExtractor(env, thiz);

    if (extractor == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    status_t err = extractor->unselectTrack(index);

    if (err != OK) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return;
    }
}

JNIEXPORT void JNICALL 
Java_android_pplive_media_player_PPMediaExtractor_setDataSource(
	JNIEnv *env, jobject thiz, jstring pathObj) {
	LOGE("setDataSource");
	sp<JPPMediaExtractor> extractor = getPPMediaExtractor(env, thiz);

	if (extractor == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

	if (pathObj == NULL) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return;
    }

    const char *path = env->GetStringUTFChars(pathObj, NULL);

    if (path == NULL) {
		return;
    }

    status_t err = extractor->setDataSource(path, NULL);

    env->ReleaseStringUTFChars(pathObj, path);
    path = NULL;

    if (err != OK) {
        jniThrowException(
                env,
                "java/io/IOException",
                "Failed to instantiate extractor.");
        return;
    }	
    
}

jint JNI_OnLoad(JavaVM *jvm, void *reserved) {
	LOGE("OnLoad");
	
	gJavaVM = jvm;
	CHECK(gJavaVM != NULL);

	return JNI_VERSION_1_4;
}

void JNI_OnUnload(JavaVM* vm, void* reserved)
{
    LOGI("JNI_OnUnload");
    //stop p2p engine.
	//PPDataSource::releaseInstance();
	//LOGE("JNI_OnUnload");

	/*
	if (gExceptionHandler != NULL)
	{
		gPlatformInfo->breakpad->finit_breakpad_handler(gExceptionHandler);
		gExceptionHandler = NULL;
	}
    */
	
    if (gPlatformInfo != NULL) 
    {
    	delete gPlatformInfo;
    	gPlatformInfo = NULL;
    }
}

__END_DECLS

}
