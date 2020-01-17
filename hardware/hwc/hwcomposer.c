
/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2015 TracMap Holdings Ltd
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

#include <stdlib.h>
#include <hardware/hardware.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <cutils/log.h>
#include <cutils/atomic.h>
#include <hardware/hwcomposer.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <EGL/egl.h>

/************************************************************************
 * Run/stop fence
 *
 * This has two operations: set a value, and wait for the value to be
 * non-zero.
 */

struct fence {
    pthread_mutex_t        lock;
    pthread_cond_t        cond;
    int            state;
};

static void fence_init(struct fence *f, int v)
{
    pthread_mutex_init(&f->lock, NULL);
    pthread_cond_init(&f->cond, NULL);
    f->state = v;
}

static void fence_destroy(struct fence *f)
{
    pthread_mutex_destroy(&f->lock);
    pthread_cond_destroy(&f->cond);
}

static void fence_set(struct fence *f, int value)
{
    pthread_mutex_lock(&f->lock);
    f->state = value;
    pthread_cond_signal(&f->cond);
    pthread_mutex_unlock(&f->lock);
}

static int fence_wait(struct fence *f)
{
    int r;
    
    pthread_mutex_lock(&f->lock);
    while (!f->state)
        pthread_cond_wait(&f->cond, &f->lock);
    r = f->state;
    pthread_mutex_unlock(&f->lock);
    
    return r;
}

/************************************************************************
 * HWComposer interface
 */

struct tm5xx_hwc {
    /* This field must come first */
    hwc_composer_device_1_t    base;
    int            fb_fd;
    
    /* VSYNC shared data. The vsync handler, although it doesn't
     * call back into the hardware composer API, might try to
     * acquire a resource which is held during a call to stop event
     * processing.
     */
    pthread_t        vs_thread;
    struct fence        vs_fence;
    const struct hwc_procs    *vs_procs;
};

static void dump_layer(hwc_layer_1_t const* l) {
    ALOGD("\ttype=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x, "
          "{%d,%d,%d,%d}, {%d,%d,%d,%d}",
          l->compositionType, l->flags, l->handle,
          l->transform, l->blending,
          l->sourceCrop.left,
          l->sourceCrop.top,
          l->sourceCrop.right,
          l->sourceCrop.bottom,
          l->displayFrame.left,
          l->displayFrame.top,
          l->displayFrame.right,
          l->displayFrame.bottom);
}

static int hwc_prepare(hwc_composer_device_1_t *dev,
                       size_t numDisplays,
                       hwc_display_contents_1_t **displays) {
    size_t i;
    
    if (!(displays && (displays[0]->flags & HWC_GEOMETRY_CHANGED)))
        return 0;
    
    for (i = 0; i < displays[0]->numHwLayers; i++) {
        dump_layer(&displays[0]->hwLayers[i]);
        displays[0]->hwLayers[i].compositionType =
        HWC_FRAMEBUFFER;
    }
    
    return 0;
}

static int hwc_set(hwc_composer_device_1_t *dev,
                   size_t numDisplays,
                   hwc_display_contents_1_t **displays)
{
    if (!eglSwapBuffers((EGLDisplay)displays[0]->dpy,
                        (EGLSurface)displays[0]->sur))
        return HWC_EGL_ERROR;
    
    return 0;
}

static int hwc_eventControl(hwc_composer_device_1_t *dev, int disp,
                            int event, int enabled)
{
    struct tm5xx_hwc *ctx = (struct tm5xx_hwc *)dev;
    
    if (event != HWC_EVENT_VSYNC)
        return -EINVAL;
    
    /* We don't synchronously wait for the thread to stop, because
     * I've discovered that the vsync callback might be waiting for
     * a resource that's currently held by our caller! Deadlock
     * results if we wait for the worker thread while it's in the
     * process of calling the vsync callback.
     */
    fence_set(&ctx->vs_fence, enabled ? 1 : 0);
    return 0;
}

static int hwc_blank(hwc_composer_device_1_t *dev, int disp, int blank)
{
    /* We do nothing */
    return 0;
}

static int hwc_query(hwc_composer_device_1_t *dev,
                     int what, int *value)
{
    /* No queries supported */
    return -EINVAL;
}

static void hwc_registerProcs(hwc_composer_device_1_t *dev,
                              const hwc_procs_t *procs)
{
    struct tm5xx_hwc *ctx = (struct tm5xx_hwc *)dev;
    
    ctx->vs_procs = procs;
}

static int hwc_device_close(struct hw_device_t *dev)
{
    struct tm5xx_hwc *ctx = (struct tm5xx_hwc *)dev;
    
    if (!ctx)
        return 0;
    
    fence_set(&ctx->vs_fence, -1);
    pthread_join(ctx->vs_thread, NULL);
    
    fence_destroy(&ctx->vs_fence);
    close(ctx->fb_fd);
    
    free(ctx);
    return 0;
}

static void *vs_worker(void *arg)
{
    struct tm5xx_hwc *ctx = (struct tm5xx_hwc *)arg;
    
    for (;;) {
        struct timespec ts;
        int dummy = 0;
        
        if (fence_wait(&ctx->vs_fence) < 0)
            break;
        
        if (ioctl(ctx->fb_fd, FBIO_WAITFORVSYNC, &dummy) < 0) {
            ALOGE("FBIO_WAITFORVSYNC error: %s", strerror(errno));
            break;
        }
        
        if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
            ALOGE("clock_gettime error: %s", strerror(errno));
            break;
        }
        
        if (ctx->vs_procs && ctx->vs_procs->vsync)
            ctx->vs_procs->vsync(ctx->vs_procs, 0,
                                 ((int64_t)ts.tv_sec) * 1000000000LL + ts.tv_nsec);
    }
    
    return NULL;
}

static int hwc_device_open(const struct hw_module_t *module,
                           const char *name,
                           struct hw_device_t** device)
{
    struct tm5xx_hwc *dev;
    
    if (strcmp(name, HWC_HARDWARE_COMPOSER))
        return -EINVAL;
    
    dev = malloc(sizeof(*dev));
    if (!dev)
        return -ENOMEM;
    
    memset(dev, 0, sizeof(*dev));
    
    dev->base.common.tag = HARDWARE_DEVICE_TAG;
    dev->base.common.version = HWC_DEVICE_API_VERSION_1_0;
    dev->base.common.module = (hw_module_t *)module;
    dev->base.common.close = hwc_device_close;
    dev->base.prepare = hwc_prepare;
    dev->base.set = hwc_set;
    dev->base.eventControl = hwc_eventControl;
    dev->base.blank = hwc_blank;
    dev->base.query = hwc_query;
    dev->base.registerProcs = hwc_registerProcs;
    
    fence_init(&dev->vs_fence, 0);
    
    dev->fb_fd = open("/dev/graphics/fb0", O_RDWR);
    if (dev->fb_fd < 0) {
        ALOGE("hwc: can't open framebuffer: %s", strerror(errno));
        fence_destroy(&dev->vs_fence);
        return -errno;
    }
    
    if (pthread_create(&dev->vs_thread, NULL, vs_worker, dev) < 0) {
        ALOGE("hwc: pthread_create: %s", strerror(errno));
        fence_destroy(&dev->vs_fence);
        close(dev->fb_fd);
        free(dev);
        return -errno;
    }
    
    *device = &dev->base.common;
    return 0;
}

static struct hw_module_methods_t hwc_module_methods = {
    .open = hwc_device_open,
};

hwc_module_t HAL_MODULE_INFO_SYM = {
    .common    = {
        .tag        = HARDWARE_MODULE_TAG,
        .version_major    = 1,
        .version_minor    = 0,
        .id        = HWC_HARDWARE_MODULE_ID,
        .name        = "TM5xx hardware composer",
        .author        = (char *)"TracMap Holdings Ltd",
        .methods    = &hwc_module_methods,
    }
};
