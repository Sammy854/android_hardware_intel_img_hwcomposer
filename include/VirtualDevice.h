/*
 * Copyright © 2012 Intel Corporation
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Jackie Li <yaodong.li@intel.com>
 *
 */
#ifndef VIRTUAL_DEVICE_H
#define VIRTUAL_DEVICE_H


#include <IDisplayDevice.h>

namespace android {
namespace intel {

class Hwcomposer;
class DisplayPlaneManager;

class VirtualDevice : public IDisplayDevice  {
public:
    VirtualDevice(Hwcomposer& hwc, DisplayPlaneManager& dpm);
    virtual ~VirtualDevice();

public:
    virtual bool prePrepare(hwc_display_contents_1_t *display);
    virtual bool prepare(hwc_display_contents_1_t *display);
    virtual bool commit(hwc_display_contents_1_t *display,
                          void* context,
                          int& count) = 0;

    virtual bool vsyncControl(int enabled);
    virtual bool blank(int blank);
    virtual bool getDisplayConfigs(uint32_t *configs,
                                       size_t *numConfigs);
    virtual bool getDisplayAttributes(uint32_t config,
                                          const uint32_t *attributes,
                                          int32_t *values);
    virtual bool compositionComplete();
    virtual bool initialize();
    virtual bool isConnected() const;
    virtual const char* getName() const;
    virtual int getType() const;
    virtual void dump(Dump& d);

protected:
    virtual void deinitialize();

protected:
    bool mInitialized;
    Hwcomposer& mHwc;
    DisplayPlaneManager& mDisplayPlaneManager;
};

}
}

#endif /* VIRTUAL_DEVICE_H */
