#ifndef PPBOX_UTIL_H_
#define PPBOX_UTIL_H_

#include "include-pp/ppbox/IDemuxer.h"

namespace android {

typedef PPBOX_DECL PP_int32 (*PPBOX_START_P2PENGINE)(PP_char const *, PP_char const *, PP_char const *);

typedef PPBOX_DECL void (*PPBOX_STOP_P2PENGINE)(void);

typedef PPBOX_DECL PP_int32 (*PPBOX_OPEN)(PP_char const *);

typedef PPBOX_DECL void (*PPBOX_ASYNCOPEN)(PP_char const *, PPBOX_Open_Callback);

typedef PPBOX_DECL void (*PPBOX_CLOSE)(void);

typedef PPBOX_DECL PP_int32 (*PPBOX_GET_STREAMCOUNT)(void);

//typedef PPBOX_DECL PP_int32 (*PPBOX_GET_STREAMINFO)(PP_uint32, PPBOX_StreamInfo*);

typedef PPBOX_DECL PP_int32 (*PPBOX_GET_STREAMINFOEX)(PP_uint32, PPBOX_StreamInfoEx*);

typedef PPBOX_DECL PP_uint32 (*PPBOX_GET_DURATION)(void);

typedef PPBOX_DECL PP_int32 (*PPBOX_SEEK)(PP_uint32);

typedef PPBOX_DECL PP_int32 (*PPBOX_READSAMPLEEX2)(PPBOX_SampleEx2*);

typedef PPBOX_DECL PP_char const * (*PPBOX_GET_LAST_ERROR_MSG)(void);

typedef struct PPBoxHandle {
    void* ppbox_lib_handle;

    PPBOX_START_P2PENGINE startP2PEngine;
    PPBOX_STOP_P2PENGINE stopP2PEngine;
    PPBOX_OPEN open;
    PPBOX_ASYNCOPEN asyncOpen;
    PPBOX_CLOSE close;
    PPBOX_GET_STREAMCOUNT getStreamCount;
//    PPBOX_GET_STREAMINFO getStreamInfo;
    PPBOX_GET_STREAMINFOEX getStreamInfoEx;
    PPBOX_GET_DURATION getDuration;
    PPBOX_SEEK seek;
    PPBOX_READSAMPLEEX2 readSampleEx2;
    PPBOX_GET_LAST_ERROR_MSG getLastErrorMsg;

    PPBoxHandle();
    ~PPBoxHandle();

} PPBoxHandle;

bool PPBoxHandle_Create(PPBoxHandle**, const char*);

static bool fun_setUp(void*, void**, const char*);

}

#endif
