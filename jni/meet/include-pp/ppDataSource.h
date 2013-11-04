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

#ifndef PPDATA_SOURCE_H_

#define PPDATA_SOURCE_H_

#include "include-pp/DataSource.h"
#include "include-pp/MediaErrors.h"

#include <stdio.h>
#include "include-pp/utils/threads.h"

namespace android {

class PPDataSource : public DataSource {
public:
    PPDataSource();
    static PPDataSource* getInstance();
    static void releaseInstance();
    virtual status_t initCheck() const;

    virtual ssize_t readAt(off_t offset, void *data, size_t size);

    virtual status_t getSize(off_t *size);
    virtual status_t openStream(const char *url);
    virtual void closeStream();

protected:
    virtual status_t init();
    virtual void uninit();
    virtual ~PPDataSource();

private:

	enum State {
        DISCONNECTED,
        CONNECTING,
        CONNECTED
    };
    static PPDataSource* mInstance;
    State mState;
	
    PPDataSource(const PPDataSource &);
    PPDataSource &operator=(const PPDataSource &);

};

}  // namespace android

#endif  // PPDATA_SOURCE_H_

