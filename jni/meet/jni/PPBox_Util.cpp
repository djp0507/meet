#define LOG_TAG "PPBox_Util"

#include "include-pp/PPBox_Util.h"

#include <stdio.h>
#include <dlfcn.h>

#include "include-pp/utils/Log.h"

namespace android {

PPBoxHandle::PPBoxHandle() {
	ppbox_lib_handle = NULL;

	startP2PEngine = NULL;
	stopP2PEngine = NULL;
	open = NULL;
	asyncOpen = NULL;
	close = NULL;
	getStreamCount = NULL;
	//getStreamInfo = NULL;
	getStreamInfoEx = NULL;
	getDuration = NULL;
	seek = NULL;
	readSampleEx2 = NULL;
	getLastErrorMsg = NULL;
}

PPBoxHandle::~PPBoxHandle() {
	startP2PEngine = NULL;
	stopP2PEngine = NULL;
	open = NULL;
	asyncOpen = NULL;
	close = NULL;
	getStreamCount = NULL;
	//getStreamInfo = NULL;
	getStreamInfoEx = NULL;
	getDuration = NULL;
	seek = NULL;
	readSampleEx2 = NULL;
	getLastErrorMsg = NULL;

	if (ppbox_lib_handle != NULL)
	{
		dlclose(ppbox_lib_handle);
		ppbox_lib_handle = NULL;
	}
}

bool PPBoxHandle_Create(PPBoxHandle** handle, const char* ppbox_lib_path)
{
	const char* err_msg = NULL;

	if (ppbox_lib_path == NULL || strlen(ppbox_lib_path) == 0) 
	{
		LOGE("PPBox Lib Path is NULL or Empty!!!");
		return false;
	}

	void* lib_handle = dlopen(ppbox_lib_path, RTLD_NOW);
	if (lib_handle == NULL)
	{
		LOGE("PPBox Lib Load failed: %s", dlerror());
		return false;
	}

	PPBoxHandle* ppbox_handle = new PPBoxHandle();
	ppbox_handle->ppbox_lib_handle = lib_handle;

	bool ret =  fun_setUp(lib_handle, (void**)(&(ppbox_handle)->startP2PEngine), "PPBOX_StartP2PEngine") &&
				fun_setUp(lib_handle, (void**)(&(ppbox_handle)->stopP2PEngine), "PPBOX_StopP2PEngine") &&
				fun_setUp(lib_handle, (void**)(&(ppbox_handle)->open), "PPBOX_Open") &&
				fun_setUp(lib_handle, (void**)(&(ppbox_handle)->asyncOpen), "PPBOX_AsyncOpen") &&
				fun_setUp(lib_handle, (void**)(&(ppbox_handle)->close), "PPBOX_Close") &&
				fun_setUp(lib_handle, (void**)(&(ppbox_handle)->getStreamCount), "PPBOX_GetStreamCount") &&
				fun_setUp(lib_handle, (void**)(&(ppbox_handle)->getStreamInfoEx), "PPBOX_GetStreamInfoEx") &&
				fun_setUp(lib_handle, (void**)(&(ppbox_handle)->getDuration), "PPBOX_GetDuration") &&
				fun_setUp(lib_handle, (void**)(&(ppbox_handle)->seek), "PPBOX_Seek") &&
				fun_setUp(lib_handle, (void**)(&(ppbox_handle)->readSampleEx2), "PPBOX_ReadSampleEx2") &&
				fun_setUp(lib_handle, (void**)(&(ppbox_handle)->getLastErrorMsg), "PPBOX_GetLastErrorMsg");


	if (ret)
	{
		*handle = ppbox_handle;
	}
	else 
	{
		delete ppbox_handle;
		ppbox_handle = NULL;
	}

	return ret;

}

static
bool fun_setUp(void* handle, void** fun, const char* symbol)
{
	*fun = dlsym(handle, symbol);
	if (*fun == NULL) {
		LOGE("symbol: %s. err_msg: %s", symbol, dlerror());
		return false;
	}

	return true;
}

}
