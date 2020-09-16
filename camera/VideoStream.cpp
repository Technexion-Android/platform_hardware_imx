/*
 *  Copyright 2020 NXP.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#define LOG_TAG "VideoStream"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <log/log.h>

#include "CameraUtils.h"
#include "VideoStream.h"

namespace android {

VideoStream::VideoStream()
{
    mNumBuffers = 0;
    mOmitFrmCount = 0;
    mOmitFrames = 0;
    mCustomDriver = false;
    mRegistered = false;
    memset(mBuffers, 0, sizeof(mBuffers));
}

VideoStream::~VideoStream()
{
}

int32_t VideoStream::openDev(const char* name)
{
    ALOGI("%s", __func__);
    if (name == NULL) {
        ALOGE("invalid dev name");
        return BAD_VALUE;
    }

    //Mutex::Autolock lock(mLock);

    mDev = open(name, O_RDWR);
    if (mDev <= 0) {
        ALOGE("%s can not open camera devpath:%s", __func__, name);
        return BAD_VALUE;
    }

    return 0;
}

#define CLOSE_WAIT_ITVL_MS 5
#define CLOSE_WAIT_ITVL_US (uint32_t)(CLOSE_WAIT_ITVL_MS*1000)

int32_t VideoStream::closeDev()
{
    ALOGI("%s", __func__);

    if (mDev > 0) {
        close(mDev);
        mDev = -1;
    }

    return 0;
}

int32_t VideoStream::onFlushLocked() {
    int32_t ret = 0;
    struct v4l2_buffer cfilledbuffer;
    struct v4l2_plane planes;
    memset(&planes, 0, sizeof(struct v4l2_plane));

    ALOGI("%s, v4l2 memory type %d, mV4l2BufType %d", __func__, mV4l2MemType, mV4l2BufType);
    // refresh the v4l2 buffers
    for (uint32_t i = 0; i < mNumBuffers; i++) {
        struct v4l2_buffer cfilledbuffer;

        memset(&cfilledbuffer, 0, sizeof(cfilledbuffer));
        cfilledbuffer.memory = mV4l2MemType;
        cfilledbuffer.type = mV4l2BufType;

        if(mV4l2BufType == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
            cfilledbuffer.m.planes = &planes;
            cfilledbuffer.length = 1;
        }

        ret = ioctl(mDev, VIDIOC_DQBUF, &cfilledbuffer);
        if (ret < 0) {
            ALOGE("%s: VIDIOC_DQBUF Failed: %s (%d)", __func__, strerror(errno), errno);
            return BAD_VALUE;
        }
        ret = ioctl(mDev, VIDIOC_QBUF, &cfilledbuffer);
        if (ret < 0) {
            ALOGE("%s: VIDIOC_QBUF Failed: %s (%d)", __func__, strerror(errno), errno);
            return BAD_VALUE;
        }
      }

    return 0;
}


} // namespace android
