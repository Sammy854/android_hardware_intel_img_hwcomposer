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
#include <HwcTrace.h>
#include <Hwcomposer.h>
#include <DisplayPlane.h>
#include <IDisplayDevice.h>
#include <HwcLayerList.h>
#include <anniedale/AnnDisplayContext.h>


namespace android {
namespace intel {

AnnDisplayContext::AnnDisplayContext()
    : mIMGDisplayDevice(0),
      mInitialized(false),
      mCount(0)
{
    CTRACE();
}

AnnDisplayContext::~AnnDisplayContext()
{
    WARN_IF_NOT_DEINIT();
}

bool AnnDisplayContext::initialize()
{
    CTRACE();

    // open frame buffer device
    hw_module_t const* module;
    int err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    if (err) {
        ETRACE("failed to load gralloc module, error = %d", err);
        return false;
    }

    // init IMG display device
    mIMGDisplayDevice = (((IMG_gralloc_module_public_t *)module)->psDisplayDevice);
    if (!mIMGDisplayDevice) {
        ETRACE("failed to get display device");
        return false;
    }

    mCount = 0;
    mInitialized = true;
    return true;
}

bool AnnDisplayContext::commitBegin(size_t numDisplays, hwc_display_contents_1_t **displays)
{
    RETURN_FALSE_IF_NOT_INIT();
    mCount = 0;
    return true;
}

bool AnnDisplayContext::commitContents(hwc_display_contents_1_t *display, HwcLayerList *layerList)
{
    bool ret;

    RETURN_FALSE_IF_NOT_INIT();

    if (!display || !layerList) {
        ETRACE("invalid parameters");
        return false;
    }

    IMG_hwc_layer_t *imgLayerList = (IMG_hwc_layer_t*)mImgLayers;

    for (size_t i = 0; i < display->numHwLayers; i++) {
        if (mCount >= MAXIMUM_LAYER_NUMBER) {
            ETRACE("layer count exceeds the limit");
            return false;
        }

        // check layer parameters
        if (!display->hwLayers[i].handle) {
            close(display->hwLayers[i].acquireFenceFd);
            continue;
        }

        DisplayPlane* plane = layerList->getPlane(i);
        if (!plane) {
            close(display->hwLayers[i].acquireFenceFd);
            continue;
        }

        ret = plane->flip(NULL);
        if (ret == false) {
            VTRACE("failed to flip plane %d", i);
            close(display->hwLayers[i].acquireFenceFd);
            continue;
        }

        IMG_hwc_layer_t *imgLayer = &imgLayerList[mCount++];
        // update IMG layer
        imgLayer->psLayer = &display->hwLayers[i];
        imgLayer->custom = (uint32_t)plane->getContext();
        struct intel_dc_plane_ctx *ctx =
            (struct intel_dc_plane_ctx *)imgLayer->custom;

        // update z order
        memset(&ctx->zorder, 0, sizeof(ctx->zorder));

        VTRACE("count %d, handle %#x, trans %#x, blending %#x"
              " sourceCrop %f,%f - %fx%f, dst %d,%d - %dx%d, custom %#x"
              " plane (type = %d, idx = %d), zorder %d",
              mCount,
              (uint32_t)imgLayer->psLayer->handle,
              imgLayer->psLayer->transform,
              imgLayer->psLayer->blending,
              imgLayer->psLayer->sourceCropf.left,
              imgLayer->psLayer->sourceCropf.top,
              imgLayer->psLayer->sourceCropf.right - imgLayer->psLayer->sourceCropf.left,
              imgLayer->psLayer->sourceCropf.bottom - imgLayer->psLayer->sourceCropf.top,
              imgLayer->psLayer->displayFrame.left,
              imgLayer->psLayer->displayFrame.top,
              imgLayer->psLayer->displayFrame.right - imgLayer->psLayer->displayFrame.left,
              imgLayer->psLayer->displayFrame.bottom - imgLayer->psLayer->displayFrame.top,
              imgLayer->custom,
              plane->getType(),
              plane->getIndex(),
              plane->getZOrder());
    }

    layerList->postFlip();
    return true;
}

bool AnnDisplayContext::commitEnd(size_t numDisplays, hwc_display_contents_1_t **displays)
{
    int releaseFenceFd = -1;

    VTRACE("count = %d", mCount);

    // nothing need to be submitted
    if (!mCount)
        return true;

    if (mIMGDisplayDevice) {
        int err = mIMGDisplayDevice->post(mIMGDisplayDevice,
                                          mImgLayers,
                                          mCount,
                                          &releaseFenceFd);
        if (err) {
            ETRACE("post failed, err = %d", err);
            return false;
        }
    }

    // update release fence
    for (size_t i = 0; i < numDisplays; i++) {
        if (!displays[i]) {
            continue;
        }

        for (size_t j = 0; j < displays[i]->numHwLayers; j++) {
            displays[i]->hwLayers[j].releaseFenceFd = dup(releaseFenceFd);
            VTRACE("handle %#x, acquiredFD %d, releaseFD %d",
                 (uint32_t)displays[i]->hwLayers[j].handle,
                 displays[i]->hwLayers[j].acquireFenceFd,
                 displays[i]->hwLayers[j].releaseFenceFd);
        }
    }

    // close original release fence fd
    close(releaseFenceFd);
    return true;
}

bool AnnDisplayContext::compositionComplete()
{
    return true;
}

void AnnDisplayContext::deinitialize()
{
    mIMGDisplayDevice = 0;

    mCount = 0;
    mInitialized = false;
}


} // namespace intel
} // namespace android