/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <arpa/inet.h>

#include "include-pp/Utils.h"
#include "include-pp/utils/Log.h"
//#include <media/stagefright/Utils.h>
#include <stdio.h>
#include <math.h>

namespace android {

uint16_t U16_AT(const uint8_t *ptr) {
    return ptr[0] << 8 | ptr[1];
}

uint32_t U32_AT(const uint8_t *ptr) {
    return ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3];
}

uint64_t U64_AT(const uint8_t *ptr) {
    return ((uint64_t)U32_AT(ptr)) << 32 | U32_AT(ptr + 4);
}

uint16_t U16LE_AT(const uint8_t *ptr) {
    return ptr[0] | (ptr[1] << 8);
}

uint32_t U32LE_AT(const uint8_t *ptr) {
    return ptr[3] << 24 | ptr[2] << 16 | ptr[1] << 8 | ptr[0];
}

uint64_t U64LE_AT(const uint8_t *ptr) {
    return ((uint64_t)U32LE_AT(ptr + 4)) << 32 | U32LE_AT(ptr);
}

// XXX warning: these won't work on big-endian host.
uint64_t ntoh64(uint64_t x) {
    return ((uint64_t)ntohl(x & 0xffffffff) << 32) | ntohl(x >> 32);
}

uint64_t hton64(uint64_t x) {
    return ((uint64_t)htonl(x & 0xffffffff) << 32) | htonl(x >> 32);
}



void saveFrame(void *dst, int length, char* path) {
	if(path==NULL) return;
	
	FILE *pFile;
	LOGD("Start open file %s", path);
	pFile=fopen(path, "wb");
	if(pFile==NULL)
	{
		LOGE("open file %s failed", path);
		return;
	}
	LOGD("open file %s success", path);

	fwrite(dst, 1, length, pFile);
	fclose(pFile);
}

void loadFrame(void *src, int length, char* path) {
	if(path==NULL) return;
	
	FILE *pFile;
	LOGD("Start open file %s", path);
	pFile=fopen(path, "rb");
	if(pFile==NULL)
	{
		LOGE("open file %s failed", path);
		return;
	}
	LOGD("open file %s success", path);

	fread(src, 1, length, pFile);
	fclose(pFile);
}

double computePSNR(uint8_t* src, uint8_t* dst, int32_t len)
{
	double s=0;
	for(size_t k=0;k<len;k++)
	{
		s+=(dst[k]-src[k])*(dst[k]-src[k]);
	}
	
	double mse=s/(len);
	double s1=255.0*255.0/mse;
	double psnr=10.0*(log10(s1));
    return psnr;
}

}  // namespace android

