/*
** Copyright 2008, Google Inc.
** Copyright (c) 2009-2010 Code Aurora Forum. All rights reserved.
** Copyright (c) 2012-2014 Tomasz Rostanski
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

//#define ALOG_NDEBUG 0
//#define ALOG_NIDEBUG 0
#define LOG_TAG "QualcommCameraHardware"
#include <utils/Log.h>

#include "QualcommCameraHardware.h"

#include <utils/Errors.h>
#include <utils/threads.h>
#include <camera/Camera.h>
#include <hardware/camera.h>
#include <utils/String16.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cutils/properties.h>
#include <math.h>
#if HAVE_ANDROID_OS
#include <linux/android_pmem.h>
#endif
#include <linux/ioctl.h>
#include "QCameraParameters.h"
#include <media/mediarecorder.h>
#include <genlock.h>
#include <system/camera.h>

#include <linux/msm_mdp.h>
#include <linux/msm_ion.h>
#include <linux/fb.h>

#define LIKELY(exp)   __builtin_expect(!!(exp), 1)
#define UNLIKELY(exp) __builtin_expect(!!(exp), 0)
#define CAMERA_HAL_UNUSED(expr) do { (void)(expr); } while (0)

extern "C" {
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/system_properties.h>
#include <sys/time.h>
#include <stdlib.h>

#define LIVESHOT_SUCCESS 0

#define DUMP_LIVESHOT_JPEG_FILE 0

#define DEFAULT_PICTURE_WIDTH  640
#define DEFAULT_PICTURE_HEIGHT 480
#define THUMBNAIL_BUFFER_SIZE (THUMBNAIL_WIDTH * THUMBNAIL_HEIGHT * 3/2)
#define MAX_ZOOM_LEVEL 5
#define NOT_FOUND -1
// Number of video buffers held by kernal (initially 1,2 &3)
#define ACTIVE_VIDEO_BUFFERS 3
#define ACTIVE_PREVIEW_BUFFERS 3

#define APP_ORIENTATION 90

#define DUMMY_CAMERA_STARTED 1;
#define DUMMY_CAMERA_STOPPED 0;

#if DLOPEN_LIBMMCAMERA
#include <dlfcn.h>

void *libmmcamera;
void* (*LINK_cam_conf)(void *data);
void* (*LINK_cam_frame)(void *data);
bool  (*LINK_jpeg_encoder_init)();
void  (*LINK_jpeg_encoder_join)();
bool  (*LINK_jpeg_encoder_encode)(const cam_ctrl_dimension_t *dimen,
                                  const uint8_t *thumbnailbuf, int thumbnailfd,
                                  const uint8_t *snapshotbuf, int snapshotfd,
                                  common_crop_t *scaling_parms, exif_tags_info_t *exif_data,
                                  int exif_table_numEntries, int jpegPadding, const int32_t cbcroffset);
void (*LINK_camframe_terminate)(void);
//for 720p
// Function to add a video buffer to free Q
void (*LINK_camframe_free_video)(struct msm_frame *frame);
// Function pointer , called by camframe when a video frame is available.
void (**LINK_camframe_video_callback)(struct msm_frame * frame);
// To flush free Q in cam frame.
void (*LINK_cam_frame_flush_free_video)(void);

int8_t (*LINK_jpeg_encoder_setMainImageQuality)(uint32_t quality);
int8_t (*LINK_jpeg_encoder_setThumbnailQuality)(uint32_t quality);
int8_t (*LINK_jpeg_encoder_setRotation)(uint32_t rotation);
int8_t (*LINK_jpeg_encoder_get_buffer_offset)(uint32_t width, uint32_t height,
                                               uint32_t* p_y_offset,
                                                uint32_t* p_cbcr_offset,
                                                 uint32_t* p_buf_size);
int8_t (*LINK_jpeg_encoder_setLocation)(const camera_position_type *location);
const struct camera_size_type *(*LINK_default_sensor_get_snapshot_sizes)(int *len);
int (*LINK_launch_cam_conf_thread)(void);
int (*LINK_release_cam_conf_thread)(void);
mm_camera_status_t (*LINK_mm_camera_init)(mm_camera_config *, mm_camera_notify*, mm_camera_ops*,uint8_t);
mm_camera_status_t (*LINK_mm_camera_deinit)();
mm_camera_status_t (*LINK_mm_camera_destroy)();
mm_camera_status_t (*LINK_mm_camera_exec)();
int8_t (*LINK_zoom_crop_upscale)(uint32_t width, uint32_t height,
    uint32_t cropped_width, uint32_t cropped_height, uint8_t *img_buf);

// callbacks
void  (**LINK_mmcamera_shutter_callback)(common_crop_t *crop);
void  (**LINK_cancel_liveshot)(void);
int8_t  (*LINK_set_liveshot_params)(uint32_t a_width, uint32_t a_height, exif_tags_info_t *a_exif_data,
                         int a_exif_numEntries, uint8_t* a_out_buffer, uint32_t a_outbuffer_size);
#else
#define LINK_cam_conf cam_conf
#define LINK_cam_frame cam_frame
#define LINK_jpeg_encoder_init jpeg_encoder_init
#define LINK_jpeg_encoder_join jpeg_encoder_join
#define LINK_jpeg_encoder_encode jpeg_encoder_encode
#define LINK_camframe_terminate camframe_terminate
#define LINK_jpeg_encoder_setMainImageQuality jpeg_encoder_setMainImageQuality
#define LINK_jpeg_encoder_setThumbnailQuality jpeg_encoder_setThumbnailQuality
#define LINK_jpeg_encoder_setRotation jpeg_encoder_setRotation
#define LINK_jpeg_encoder_get_buffer_offset jpeg_encoder_get_buffer_offset
#define LINK_jpeg_encoder_setLocation jpeg_encoder_setLocation
#define LINK_default_sensor_get_snapshot_sizes default_sensor_get_snapshot_sizes
#define LINK_launch_cam_conf_thread launch_cam_conf_thread
#define LINK_release_cam_conf_thread release_cam_conf_thread
#define LINK_zoom_crop_upscale zoom_crop_upscale
#define LINK_mm_camera_init mm_camera_config_init
#define LINK_mm_camera_deinit mm_camera_config_deinit
#define LINK_mm_camera_destroy mm_camera_config_destroy
#define LINK_mm_camera_exec mm_camera_exec
extern void (*mmcamera_camframe_callback)(struct msm_frame *frame);
extern void (*mmcamera_camstats_callback)(camstats_type stype, camera_preview_histogram_info* histinfo);
extern void (*mmcamera_jpegfragment_callback)(uint8_t *buff_ptr,
                                      uint32_t buff_size);
extern void (*mmcamera_jpeg_callback)(jpeg_event_t status);
extern void (*mmcamera_shutter_callback)(common_crop_t *crop);
extern void (*mmcamera_liveshot_callback)(liveshot_status status, uint32_t jpeg_size);
#define LINK_set_liveshot_params set_liveshot_params
#endif

} // extern "C"

#ifndef HAVE_CAMERA_SIZE_TYPE
struct camera_size_type {
    int width;
    int height;
};
#endif
union zoomimage
{
    char d[sizeof(struct mdp_blit_req_list) + sizeof(struct mdp_blit_req) * 1];
    struct mdp_blit_req_list list;
} zoomImage;

//Default to VGA
#define DEFAULT_PREVIEW_WIDTH 640
#define DEFAULT_PREVIEW_HEIGHT 480

//Default to VGA
#define DEFAULT_VIDEO_WIDTH 640
#define DEFAULT_VIDEO_HEIGHT 480

//Default FPS
#define MINIMUM_FPS 15
#define MAXIMUM_FPS 31
#define DEFAULT_FPS MAXIMUM_FPS

/*
 * Modifying preview size requires modification
 * in bitmasks for boardproperties
 */


static uint32_t  PREVIEW_SIZE_COUNT;

board_property boardProperties[] = {
    {TARGET_MSM7625, 0x00000fff, false, false, false},
    {TARGET_MSM7627, 0x000006ff, false, false, false},
    {TARGET_MSM7630, 0x00000fff, true, true, false},
    {TARGET_MSM8660, 0x00002fff, true, true, false},
    {TARGET_QSD8250, 0x00000fff, false, false, false}
};

/*       TODO
 * Ideally this should be a populated by lower layers.
 * But currently this is no API to do that at lower layer.
 * Hence populating with default sizes for now. This needs
 * to be changed once the API is supported.
 */
static int data_counter = 0;
//sorted on column basis
static camera_size_type* picture_sizes;
static camera_size_type* preview_sizes;
static unsigned int PICTURE_SIZE_COUNT;
static const camera_size_type * picture_sizes_ptr;
static int supportedPictureSizesCount;
static liveshotState liveshot_state = LIVESHOT_DONE;
static int sensor_rotation = 0;

#ifdef Q12
#undef Q12
#endif

#define Q12 4096

static const target_map targetList [] = {
    { "msm7625", TARGET_MSM7625 },
    { "msm7627", TARGET_MSM7627 },
    { "qsd8250", TARGET_QSD8250 },
    { "msm7630", TARGET_MSM7630 },
    { "msm8660", TARGET_MSM8660 }
};
static targetType mCurrentTarget = TARGET_MAX;

typedef struct {
    uint32_t aspect_ratio;
    uint32_t width;
    uint32_t height;
} thumbnail_size_type;

static thumbnail_size_type thumbnail_sizes[] = {
    { 7281, 512, 288 }, //1.777778
    { 6826, 480, 288 }, //1.666667
    { 6808, 256, 154 }, //1.662337
    { 6144, 432, 288 }, //1.5
    { 5461, 192, 144 }, //1.333333
    { 5006, 352, 288 }, //1.222222
};
#define THUMBNAIL_SIZE_COUNT (sizeof(thumbnail_sizes)/sizeof(thumbnail_size_type))
#define DEFAULT_THUMBNAIL_SETTING 4
#define THUMBNAIL_WIDTH_STR "192"
#define THUMBNAIL_HEIGHT_STR "144"
#define THUMBNAIL_SMALL_HEIGHT 144
static camera_size_type jpeg_thumbnail_sizes[]  = {
    { 512, 288 },
    { 480, 288 },
    { 256, 154 },
    { 432, 288 },
    { 192, 144 },
    { 352, 288 },
    {0,0}
};
//supported preview fps ranges should be added to this array in the form (minFps,maxFps)
static  android::FPSRange FpsRangesSupported[1] = {android::FPSRange(MINIMUM_FPS*1000,MAXIMUM_FPS*1000)};

#define FPS_RANGES_SUPPORTED_COUNT (sizeof(FpsRangesSupported)/sizeof(FpsRangesSupported[0]))

#define JPEG_THUMBNAIL_SIZE_COUNT (sizeof(jpeg_thumbnail_sizes)/sizeof(camera_size_type))
static int attr_lookup(const str_map arr[], int len, const char *name)
{
    if (name) {
        for (int i = 0; i < len; i++) {
            if (!strcmp(arr[i].desc, name))
                return arr[i].val;
        }
    }
    return NOT_FOUND;
}

// round to the next power of two
static inline unsigned clp2(unsigned x)
{
    x = x - 1;
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >>16);
    return x + 1;
}

static int exif_table_numEntries = 0;
#define MAX_EXIF_TABLE_ENTRIES 11
exif_tags_info_t exif_data[MAX_EXIF_TABLE_ENTRIES];
static android_native_rect_t zoomCropInfo;
static void *mLastQueuedFrame = NULL;
#define RECORD_BUFFERS 9
#define RECORD_BUFFERS_8x50 8
static int kRecordBufferCount;
/* controls whether VPE is avialable for the target
 * under consideration.
 * 1: VPE support is available
 * 0: VPE support is not available (default)
 */
static bool mVpeEnabled;

static int HAL_numOfCameras = 0;
static qcamera_info_t HAL_cameraInfo[MSM_MAX_CAMERA_SENSORS];
static int HAL_currentCameraId = 0;
static mm_camera_config mCfgControl;
static bool mCameraOpen;

namespace android {
    extern void native_send_data_callback(int32_t msgType,
                              camera_memory_t * framebuffer,
                              void* user);

    extern camera_memory_t* get_mem(int fd,size_t buf_size,
                                unsigned int num_bufs,
                                void *user);

static const int PICTURE_FORMAT_JPEG = 1;
static const int PICTURE_FORMAT_RAW = 2;

// from aeecamera.h
static const str_map whitebalance[] = {
    { QCameraParameters::WHITE_BALANCE_AUTO,            CAMERA_WB_AUTO },
    { QCameraParameters::WHITE_BALANCE_INCANDESCENT,    CAMERA_WB_INCANDESCENT },
    { QCameraParameters::WHITE_BALANCE_FLUORESCENT,     CAMERA_WB_FLUORESCENT },
    { QCameraParameters::WHITE_BALANCE_DAYLIGHT,        CAMERA_WB_DAYLIGHT },
    { QCameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT, CAMERA_WB_CLOUDY_DAYLIGHT }
};

// from camera_effect_t. This list must match aeecamera.h
static const str_map effects[] = {
    { QCameraParameters::EFFECT_NONE,       CAMERA_EFFECT_OFF },
    { QCameraParameters::EFFECT_MONO,       CAMERA_EFFECT_MONO },
    { QCameraParameters::EFFECT_NEGATIVE,   CAMERA_EFFECT_NEGATIVE },
    { QCameraParameters::EFFECT_SOLARIZE,   CAMERA_EFFECT_SOLARIZE },
    { QCameraParameters::EFFECT_SEPIA,      CAMERA_EFFECT_SEPIA },
    { QCameraParameters::EFFECT_POSTERIZE,  CAMERA_EFFECT_POSTERIZE },
    { QCameraParameters::EFFECT_WHITEBOARD, CAMERA_EFFECT_WHITEBOARD },
    { QCameraParameters::EFFECT_BLACKBOARD, CAMERA_EFFECT_BLACKBOARD },
    { QCameraParameters::EFFECT_AQUA,       CAMERA_EFFECT_AQUA }
};

// from qcamera/common/camera.h
static const str_map autoexposure[] = {
    { QCameraParameters::AUTO_EXPOSURE_FRAME_AVG,  CAMERA_AEC_FRAME_AVERAGE },
    { QCameraParameters::AUTO_EXPOSURE_CENTER_WEIGHTED, CAMERA_AEC_CENTER_WEIGHTED },
    { QCameraParameters::AUTO_EXPOSURE_SPOT_METERING, CAMERA_AEC_SPOT_METERING }
};

// from qcamera/common/camera.h
static const str_map antibanding[] = {
    { QCameraParameters::ANTIBANDING_OFF,  CAMERA_ANTIBANDING_OFF },
    { QCameraParameters::ANTIBANDING_50HZ, CAMERA_ANTIBANDING_50HZ },
    { QCameraParameters::ANTIBANDING_60HZ, CAMERA_ANTIBANDING_60HZ },
    { QCameraParameters::ANTIBANDING_AUTO, CAMERA_ANTIBANDING_AUTO }
};

/* Mapping from MCC to antibanding type */
struct country_map {
    uint32_t country_code;
    camera_antibanding_type type;
};

static struct country_map country_numeric[] = {
    { 202, CAMERA_ANTIBANDING_50HZ }, // Greece
    { 204, CAMERA_ANTIBANDING_50HZ }, // Netherlands
    { 206, CAMERA_ANTIBANDING_50HZ }, // Belgium
    { 208, CAMERA_ANTIBANDING_50HZ }, // France
    { 212, CAMERA_ANTIBANDING_50HZ }, // Monaco
    { 213, CAMERA_ANTIBANDING_50HZ }, // Andorra
    { 214, CAMERA_ANTIBANDING_50HZ }, // Spain
    { 216, CAMERA_ANTIBANDING_50HZ }, // Hungary
    { 219, CAMERA_ANTIBANDING_50HZ }, // Croatia
    { 220, CAMERA_ANTIBANDING_50HZ }, // Serbia
    { 222, CAMERA_ANTIBANDING_50HZ }, // Italy
    { 226, CAMERA_ANTIBANDING_50HZ }, // Romania
    { 228, CAMERA_ANTIBANDING_50HZ }, // Switzerland
    { 230, CAMERA_ANTIBANDING_50HZ }, // Czech Republic
    { 231, CAMERA_ANTIBANDING_50HZ }, // Slovakia
    { 232, CAMERA_ANTIBANDING_50HZ }, // Austria
    { 234, CAMERA_ANTIBANDING_50HZ }, // United Kingdom
    { 235, CAMERA_ANTIBANDING_50HZ }, // United Kingdom
    { 238, CAMERA_ANTIBANDING_50HZ }, // Denmark
    { 240, CAMERA_ANTIBANDING_50HZ }, // Sweden
    { 242, CAMERA_ANTIBANDING_50HZ }, // Norway
    { 244, CAMERA_ANTIBANDING_50HZ }, // Finland
    { 246, CAMERA_ANTIBANDING_50HZ }, // Lithuania
    { 247, CAMERA_ANTIBANDING_50HZ }, // Latvia
    { 248, CAMERA_ANTIBANDING_50HZ }, // Estonia
    { 250, CAMERA_ANTIBANDING_50HZ }, // Russian Federation
    { 255, CAMERA_ANTIBANDING_50HZ }, // Ukraine
    { 257, CAMERA_ANTIBANDING_50HZ }, // Belarus
    { 259, CAMERA_ANTIBANDING_50HZ }, // Moldova
    { 260, CAMERA_ANTIBANDING_50HZ }, // Poland
    { 262, CAMERA_ANTIBANDING_50HZ }, // Germany
    { 266, CAMERA_ANTIBANDING_50HZ }, // Gibraltar
    { 268, CAMERA_ANTIBANDING_50HZ }, // Portugal
    { 270, CAMERA_ANTIBANDING_50HZ }, // Luxembourg
    { 272, CAMERA_ANTIBANDING_50HZ }, // Ireland
    { 274, CAMERA_ANTIBANDING_50HZ }, // Iceland
    { 276, CAMERA_ANTIBANDING_50HZ }, // Albania
    { 278, CAMERA_ANTIBANDING_50HZ }, // Malta
    { 280, CAMERA_ANTIBANDING_50HZ }, // Cyprus
    { 282, CAMERA_ANTIBANDING_50HZ }, // Georgia
    { 283, CAMERA_ANTIBANDING_50HZ }, // Armenia
    { 284, CAMERA_ANTIBANDING_50HZ }, // Bulgaria
    { 286, CAMERA_ANTIBANDING_50HZ }, // Turkey
    { 288, CAMERA_ANTIBANDING_50HZ }, // Faroe Islands
    { 290, CAMERA_ANTIBANDING_50HZ }, // Greenland
    { 293, CAMERA_ANTIBANDING_50HZ }, // Slovenia
    { 294, CAMERA_ANTIBANDING_50HZ }, // Macedonia
    { 295, CAMERA_ANTIBANDING_50HZ }, // Liechtenstein
    { 297, CAMERA_ANTIBANDING_50HZ }, // Montenegro
    { 302, CAMERA_ANTIBANDING_60HZ }, // Canada
    { 310, CAMERA_ANTIBANDING_60HZ }, // United States of America
    { 311, CAMERA_ANTIBANDING_60HZ }, // United States of America
    { 312, CAMERA_ANTIBANDING_60HZ }, // United States of America
    { 313, CAMERA_ANTIBANDING_60HZ }, // United States of America
    { 314, CAMERA_ANTIBANDING_60HZ }, // United States of America
    { 315, CAMERA_ANTIBANDING_60HZ }, // United States of America
    { 316, CAMERA_ANTIBANDING_60HZ }, // United States of America
    { 330, CAMERA_ANTIBANDING_60HZ }, // Puerto Rico
    { 334, CAMERA_ANTIBANDING_60HZ }, // Mexico
    { 338, CAMERA_ANTIBANDING_50HZ }, // Jamaica
    { 340, CAMERA_ANTIBANDING_50HZ }, // Martinique
    { 342, CAMERA_ANTIBANDING_50HZ }, // Barbados
    { 346, CAMERA_ANTIBANDING_60HZ }, // Cayman Islands
    { 350, CAMERA_ANTIBANDING_60HZ }, // Bermuda
    { 352, CAMERA_ANTIBANDING_50HZ }, // Grenada
    { 354, CAMERA_ANTIBANDING_60HZ }, // Montserrat
    { 362, CAMERA_ANTIBANDING_50HZ }, // Netherlands Antilles
    { 363, CAMERA_ANTIBANDING_60HZ }, // Aruba
    { 364, CAMERA_ANTIBANDING_60HZ }, // Bahamas
    { 365, CAMERA_ANTIBANDING_60HZ }, // Anguilla
    { 366, CAMERA_ANTIBANDING_50HZ }, // Dominica
    { 368, CAMERA_ANTIBANDING_60HZ }, // Cuba
    { 370, CAMERA_ANTIBANDING_60HZ }, // Dominican Republic
    { 372, CAMERA_ANTIBANDING_60HZ }, // Haiti
    { 401, CAMERA_ANTIBANDING_50HZ }, // Kazakhstan
    { 402, CAMERA_ANTIBANDING_50HZ }, // Bhutan
    { 404, CAMERA_ANTIBANDING_50HZ }, // India
    { 405, CAMERA_ANTIBANDING_50HZ }, // India
    { 410, CAMERA_ANTIBANDING_50HZ }, // Pakistan
    { 413, CAMERA_ANTIBANDING_50HZ }, // Sri Lanka
    { 414, CAMERA_ANTIBANDING_50HZ }, // Myanmar
    { 415, CAMERA_ANTIBANDING_50HZ }, // Lebanon
    { 416, CAMERA_ANTIBANDING_50HZ }, // Jordan
    { 417, CAMERA_ANTIBANDING_50HZ }, // Syria
    { 418, CAMERA_ANTIBANDING_50HZ }, // Iraq
    { 419, CAMERA_ANTIBANDING_50HZ }, // Kuwait
    { 420, CAMERA_ANTIBANDING_60HZ }, // Saudi Arabia
    { 421, CAMERA_ANTIBANDING_50HZ }, // Yemen
    { 422, CAMERA_ANTIBANDING_50HZ }, // Oman
    { 424, CAMERA_ANTIBANDING_50HZ }, // United Arab Emirates
    { 425, CAMERA_ANTIBANDING_50HZ }, // Israel
    { 426, CAMERA_ANTIBANDING_50HZ }, // Bahrain
    { 427, CAMERA_ANTIBANDING_50HZ }, // Qatar
    { 428, CAMERA_ANTIBANDING_50HZ }, // Mongolia
    { 429, CAMERA_ANTIBANDING_50HZ }, // Nepal
    { 430, CAMERA_ANTIBANDING_50HZ }, // United Arab Emirates
    { 431, CAMERA_ANTIBANDING_50HZ }, // United Arab Emirates
    { 432, CAMERA_ANTIBANDING_50HZ }, // Iran
    { 434, CAMERA_ANTIBANDING_50HZ }, // Uzbekistan
    { 436, CAMERA_ANTIBANDING_50HZ }, // Tajikistan
    { 437, CAMERA_ANTIBANDING_50HZ }, // Kyrgyz Rep
    { 438, CAMERA_ANTIBANDING_50HZ }, // Turkmenistan
    { 440, CAMERA_ANTIBANDING_60HZ }, // Japan
    { 441, CAMERA_ANTIBANDING_60HZ }, // Japan
    { 452, CAMERA_ANTIBANDING_50HZ }, // Vietnam
    { 454, CAMERA_ANTIBANDING_50HZ }, // Hong Kong
    { 455, CAMERA_ANTIBANDING_50HZ }, // Macao
    { 456, CAMERA_ANTIBANDING_50HZ }, // Cambodia
    { 457, CAMERA_ANTIBANDING_50HZ }, // Laos
    { 460, CAMERA_ANTIBANDING_50HZ }, // China
    { 466, CAMERA_ANTIBANDING_60HZ }, // Taiwan
    { 470, CAMERA_ANTIBANDING_50HZ }, // Bangladesh
    { 472, CAMERA_ANTIBANDING_50HZ }, // Maldives
    { 502, CAMERA_ANTIBANDING_50HZ }, // Malaysia
    { 505, CAMERA_ANTIBANDING_50HZ }, // Australia
    { 510, CAMERA_ANTIBANDING_50HZ }, // Indonesia
    { 514, CAMERA_ANTIBANDING_50HZ }, // East Timor
    { 515, CAMERA_ANTIBANDING_60HZ }, // Philippines
    { 520, CAMERA_ANTIBANDING_50HZ }, // Thailand
    { 525, CAMERA_ANTIBANDING_50HZ }, // Singapore
    { 530, CAMERA_ANTIBANDING_50HZ }, // New Zealand
    { 535, CAMERA_ANTIBANDING_60HZ }, // Guam
    { 536, CAMERA_ANTIBANDING_50HZ }, // Nauru
    { 537, CAMERA_ANTIBANDING_50HZ }, // Papua New Guinea
    { 539, CAMERA_ANTIBANDING_50HZ }, // Tonga
    { 541, CAMERA_ANTIBANDING_50HZ }, // Vanuatu
    { 542, CAMERA_ANTIBANDING_50HZ }, // Fiji
    { 544, CAMERA_ANTIBANDING_60HZ }, // American Samoa
    { 545, CAMERA_ANTIBANDING_50HZ }, // Kiribati
    { 546, CAMERA_ANTIBANDING_50HZ }, // New Caledonia
    { 548, CAMERA_ANTIBANDING_50HZ }, // Cook Islands
    { 602, CAMERA_ANTIBANDING_50HZ }, // Egypt
    { 603, CAMERA_ANTIBANDING_50HZ }, // Algeria
    { 604, CAMERA_ANTIBANDING_50HZ }, // Morocco
    { 605, CAMERA_ANTIBANDING_50HZ }, // Tunisia
    { 606, CAMERA_ANTIBANDING_50HZ }, // Libya
    { 607, CAMERA_ANTIBANDING_50HZ }, // Gambia
    { 608, CAMERA_ANTIBANDING_50HZ }, // Senegal
    { 609, CAMERA_ANTIBANDING_50HZ }, // Mauritania
    { 610, CAMERA_ANTIBANDING_50HZ }, // Mali
    { 611, CAMERA_ANTIBANDING_50HZ }, // Guinea
    { 613, CAMERA_ANTIBANDING_50HZ }, // Burkina Faso
    { 614, CAMERA_ANTIBANDING_50HZ }, // Niger
    { 616, CAMERA_ANTIBANDING_50HZ }, // Benin
    { 617, CAMERA_ANTIBANDING_50HZ }, // Mauritius
    { 618, CAMERA_ANTIBANDING_50HZ }, // Liberia
    { 619, CAMERA_ANTIBANDING_50HZ }, // Sierra Leone
    { 620, CAMERA_ANTIBANDING_50HZ }, // Ghana
    { 621, CAMERA_ANTIBANDING_50HZ }, // Nigeria
    { 622, CAMERA_ANTIBANDING_50HZ }, // Chad
    { 623, CAMERA_ANTIBANDING_50HZ }, // Central African Republic
    { 624, CAMERA_ANTIBANDING_50HZ }, // Cameroon
    { 625, CAMERA_ANTIBANDING_50HZ }, // Cape Verde
    { 627, CAMERA_ANTIBANDING_50HZ }, // Equatorial Guinea
    { 631, CAMERA_ANTIBANDING_50HZ }, // Angola
    { 633, CAMERA_ANTIBANDING_50HZ }, // Seychelles
    { 634, CAMERA_ANTIBANDING_50HZ }, // Sudan
    { 636, CAMERA_ANTIBANDING_50HZ }, // Ethiopia
    { 637, CAMERA_ANTIBANDING_50HZ }, // Somalia
    { 638, CAMERA_ANTIBANDING_50HZ }, // Djibouti
    { 639, CAMERA_ANTIBANDING_50HZ }, // Kenya
    { 640, CAMERA_ANTIBANDING_50HZ }, // Tanzania
    { 641, CAMERA_ANTIBANDING_50HZ }, // Uganda
    { 642, CAMERA_ANTIBANDING_50HZ }, // Burundi
    { 643, CAMERA_ANTIBANDING_50HZ }, // Mozambique
    { 645, CAMERA_ANTIBANDING_50HZ }, // Zambia
    { 646, CAMERA_ANTIBANDING_50HZ }, // Madagascar
    { 647, CAMERA_ANTIBANDING_50HZ }, // France
    { 648, CAMERA_ANTIBANDING_50HZ }, // Zimbabwe
    { 649, CAMERA_ANTIBANDING_50HZ }, // Namibia
    { 650, CAMERA_ANTIBANDING_50HZ }, // Malawi
    { 651, CAMERA_ANTIBANDING_50HZ }, // Lesotho
    { 652, CAMERA_ANTIBANDING_50HZ }, // Botswana
    { 653, CAMERA_ANTIBANDING_50HZ }, // Swaziland
    { 654, CAMERA_ANTIBANDING_50HZ }, // Comoros
    { 655, CAMERA_ANTIBANDING_50HZ }, // South Africa
    { 657, CAMERA_ANTIBANDING_50HZ }, // Eritrea
    { 702, CAMERA_ANTIBANDING_60HZ }, // Belize
    { 704, CAMERA_ANTIBANDING_60HZ }, // Guatemala
    { 706, CAMERA_ANTIBANDING_60HZ }, // El Salvador
    { 708, CAMERA_ANTIBANDING_60HZ }, // Honduras
    { 710, CAMERA_ANTIBANDING_60HZ }, // Nicaragua
    { 712, CAMERA_ANTIBANDING_60HZ }, // Costa Rica
    { 714, CAMERA_ANTIBANDING_60HZ }, // Panama
    { 722, CAMERA_ANTIBANDING_50HZ }, // Argentina
    { 724, CAMERA_ANTIBANDING_60HZ }, // Brazil
    { 730, CAMERA_ANTIBANDING_50HZ }, // Chile
    { 732, CAMERA_ANTIBANDING_60HZ }, // Colombia
    { 734, CAMERA_ANTIBANDING_60HZ }, // Venezuela
    { 736, CAMERA_ANTIBANDING_50HZ }, // Bolivia
    { 738, CAMERA_ANTIBANDING_60HZ }, // Guyana
    { 740, CAMERA_ANTIBANDING_60HZ }, // Ecuador
    { 742, CAMERA_ANTIBANDING_50HZ }, // French Guiana
    { 744, CAMERA_ANTIBANDING_50HZ }, // Paraguay
    { 746, CAMERA_ANTIBANDING_60HZ }, // Suriname
    { 748, CAMERA_ANTIBANDING_50HZ }, // Uruguay
    { 750, CAMERA_ANTIBANDING_50HZ }, // Falkland Islands
};

static const str_map scenemode[] = {
    { QCameraParameters::SCENE_MODE_AUTO,           CAMERA_BESTSHOT_OFF },
    { QCameraParameters::SCENE_MODE_ACTION,         CAMERA_BESTSHOT_ACTION },
    { QCameraParameters::SCENE_MODE_PORTRAIT,       CAMERA_BESTSHOT_PORTRAIT },
    { QCameraParameters::SCENE_MODE_LANDSCAPE,      CAMERA_BESTSHOT_LANDSCAPE },
    { QCameraParameters::SCENE_MODE_NIGHT,          CAMERA_BESTSHOT_NIGHT },
    { QCameraParameters::SCENE_MODE_NIGHT_PORTRAIT, CAMERA_BESTSHOT_NIGHT_PORTRAIT },
    { QCameraParameters::SCENE_MODE_THEATRE,        CAMERA_BESTSHOT_THEATRE },
    { QCameraParameters::SCENE_MODE_BEACH,          CAMERA_BESTSHOT_BEACH },
    { QCameraParameters::SCENE_MODE_SNOW,           CAMERA_BESTSHOT_SNOW },
    { QCameraParameters::SCENE_MODE_SUNSET,         CAMERA_BESTSHOT_SUNSET },
    { QCameraParameters::SCENE_MODE_STEADYPHOTO,    CAMERA_BESTSHOT_ANTISHAKE },
    { QCameraParameters::SCENE_MODE_FIREWORKS ,     CAMERA_BESTSHOT_FIREWORKS },
    { QCameraParameters::SCENE_MODE_SPORTS ,        CAMERA_BESTSHOT_SPORTS },
    { QCameraParameters::SCENE_MODE_PARTY,          CAMERA_BESTSHOT_PARTY },
    { QCameraParameters::SCENE_MODE_CANDLELIGHT,    CAMERA_BESTSHOT_CANDLELIGHT },
    { QCameraParameters::SCENE_MODE_BACKLIGHT,      CAMERA_BESTSHOT_BACKLIGHT },
    { QCameraParameters::SCENE_MODE_FLOWERS,        CAMERA_BESTSHOT_FLOWERS },
    { QCameraParameters::SCENE_MODE_AR,             CAMERA_BESTSHOT_AR },
};

static const str_map scenedetect[] = {
    { QCameraParameters::SCENE_DETECT_OFF, FALSE  },
    { QCameraParameters::SCENE_DETECT_ON, TRUE },
};


#define country_number (sizeof(country_numeric) / sizeof(country_map))
/* TODO : setting dummy values as of now, need to query for correct
 * values from sensor in future
 */
#define CAMERA_FOCAL_LENGTH_DEFAULT 4.31
#define CAMERA_HORIZONTAL_VIEW_ANGLE_DEFAULT 54.8
#define CAMERA_VERTICAL_VIEW_ANGLE_DEFAULT  42.5

/* Look up pre-sorted antibanding_type table by current MCC. */
static camera_antibanding_type camera_get_location(void) {
    char value[PROP_VALUE_MAX];
    char country_value[PROP_VALUE_MAX];
    uint32_t country_code, count;
    memset(value, 0x00, sizeof(value));
    memset(country_value, 0x00, sizeof(country_value));
    if (!__system_property_get("gsm.operator.numeric", value)) {
        return CAMERA_ANTIBANDING_60HZ;
    }
    memcpy(country_value, value, 3);
    country_code = atoi(country_value);
    ALOGD("value:%s, country value:%s, country code:%d\n",
        value, country_value, country_code);
    int left = 0;
    int right = country_number - 1;
    while (left <= right) {
        int index = (left + right) >> 1;
        if (country_numeric[index].country_code == country_code)
            return country_numeric[index].type;
        else if (country_numeric[index].country_code > country_code)
            right = index - 1;
        else
            left = index + 1;
    }
    return CAMERA_ANTIBANDING_60HZ;
}

// from camera.h, led_mode_t
static const str_map flash[] = {
    { QCameraParameters::FLASH_MODE_OFF,  LED_MODE_OFF },
    { QCameraParameters::FLASH_MODE_AUTO, LED_MODE_AUTO },
    { QCameraParameters::FLASH_MODE_ON, LED_MODE_ON },
    { QCameraParameters::FLASH_MODE_TORCH, LED_MODE_TORCH }
};

// from mm-camera/common/camera.h.
static const str_map iso[] = {
    { QCameraParameters::ISO_AUTO,  CAMERA_ISO_AUTO},
    { QCameraParameters::ISO_HJR,   CAMERA_ISO_DEBLUR},
    { QCameraParameters::ISO_100,   CAMERA_ISO_100},
    { QCameraParameters::ISO_200,   CAMERA_ISO_200},
    { QCameraParameters::ISO_400,   CAMERA_ISO_400},
    { QCameraParameters::ISO_800,   CAMERA_ISO_800 },
    { QCameraParameters::ISO_1600,  CAMERA_ISO_1600 }
};

#define DONT_CARE 0
static const str_map focus_modes[] = {
    { QCameraParameters::FOCUS_MODE_AUTO,     AF_MODE_AUTO},
    { QCameraParameters::FOCUS_MODE_INFINITY, DONT_CARE },
    { QCameraParameters::FOCUS_MODE_NORMAL,   AF_MODE_NORMAL },
    { QCameraParameters::FOCUS_MODE_MACRO,    AF_MODE_MACRO },
    { QCameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO, DONT_CARE }
};

static const str_map lensshade[] = {
    { QCameraParameters::LENSSHADE_ENABLE, TRUE },
    { QCameraParameters::LENSSHADE_DISABLE, FALSE }
};

static const str_map histogram[] = {
    { QCameraParameters::HISTOGRAM_ENABLE, TRUE },
    { QCameraParameters::HISTOGRAM_DISABLE, FALSE }
};

static const str_map skinToneEnhancement[] = {
    { QCameraParameters::SKIN_TONE_ENHANCEMENT_ENABLE, TRUE },
    { QCameraParameters::SKIN_TONE_ENHANCEMENT_DISABLE, FALSE }
};

static const str_map continuous_af[] = {
    { "caf-off", FALSE },
    { "caf-on", TRUE }
};

static const str_map selectable_zone_af[] = {
    { QCameraParameters::SELECTABLE_ZONE_AF_AUTO,  AUTO },
    { QCameraParameters::SELECTABLE_ZONE_AF_SPOT_METERING, SPOT },
    { QCameraParameters::SELECTABLE_ZONE_AF_CENTER_WEIGHTED, CENTER_WEIGHTED },
    { QCameraParameters::SELECTABLE_ZONE_AF_FRAME_AVERAGE, AVERAGE }
};

static const str_map facedetection[] = {
    { QCameraParameters::FACE_DETECTION_OFF, FALSE },
    { QCameraParameters::FACE_DETECTION_ON, TRUE }
};

#define DONT_CARE_COORDINATE -1
static const str_map touchafaec[] = {
    { QCameraParameters::TOUCH_AF_AEC_OFF, FALSE },
    { QCameraParameters::TOUCH_AF_AEC_ON, TRUE }
};

/*
 * Values based on aec.c
 */
#define CAMERA_HISTOGRAM_ENABLE 1
#define CAMERA_HISTOGRAM_DISABLE 0
#define HISTOGRAM_STATS_SIZE 257

#define EXPOSURE_COMPENSATION_MAXIMUM_NUMERATOR 12
#define EXPOSURE_COMPENSATION_MINIMUM_NUMERATOR -12
#define EXPOSURE_COMPENSATION_DEFAULT_NUMERATOR 0
#define EXPOSURE_COMPENSATION_DENOMINATOR 6
#define EXPOSURE_COMPENSATION_STEP ((float (1))/EXPOSURE_COMPENSATION_DENOMINATOR)

static const str_map picture_formats[] = {
        {QCameraParameters::PIXEL_FORMAT_JPEG, PICTURE_FORMAT_JPEG},
        {QCameraParameters::PIXEL_FORMAT_RAW, PICTURE_FORMAT_RAW}
};

static const str_map frame_rate_modes[] = {
        {QCameraParameters::KEY_PREVIEW_FRAME_RATE_AUTO_MODE, FPS_MODE_AUTO},
        {QCameraParameters::KEY_PREVIEW_FRAME_RATE_FIXED_MODE, FPS_MODE_FIXED}
};

static int mPreviewFormat;
static const str_map preview_formats[] = {
        {QCameraParameters::PIXEL_FORMAT_YUV420SP,   CAMERA_YUV_420_NV21},
        {QCameraParameters::PIXEL_FORMAT_YUV420SP_ADRENO, CAMERA_YUV_420_NV21_ADRENO}
};

static const str_map app_preview_formats[] = {
        {QCameraParameters::PIXEL_FORMAT_YUV420SP,   HAL_PIXEL_FORMAT_YCrCb_420_SP}, //nv21
        {QCameraParameters::PIXEL_FORMAT_YUV420SP_ADRENO, HAL_PIXEL_FORMAT_YCrCb_420_SP_ADRENO}, //nv21_adreno
        {QCameraParameters::PIXEL_FORMAT_YUV420P, HAL_PIXEL_FORMAT_YV12}, //YV12
};

static bool parameter_string_initialized = false;
static String8 preview_size_values;
static String8 picture_size_values;
static String8 fps_ranges_supported_values;
static String8 jpeg_thumbnail_size_values;
static String8 antibanding_values;
static String8 effect_values;
static String8 autoexposure_values;
static String8 whitebalance_values;
static String8 flash_values;
static String8 focus_mode_values;
static String8 iso_values;
static String8 lensshade_values;
static String8 histogram_values;
static String8 skinToneEnhancement_values;
static String8 touchafaec_values;
static String8 picture_format_values;
static String8 scenemode_values;
static String8 continuous_af_values;
static String8 zoom_ratio_values;
static String8 preview_frame_rate_values;
static String8 frame_rate_mode_values;
static String8 scenedetect_values;
static String8 preview_format_values;
static String8 selectable_zone_af_values;
static String8 facedetection_values;
mm_camera_notify mCamNotify;
mm_camera_ops mCamOps;

static String8 create_sizes_str(const camera_size_type *sizes, int len) {
    String8 str;
    char buffer[32];

    if (len > 0) {
        snprintf(buffer, sizeof(buffer), "%dx%d", sizes[0].width, sizes[0].height);
        str.append(buffer);
    }
    for (int i = 1; i < len; i++) {
        snprintf(buffer, sizeof(buffer), ",%dx%d", sizes[i].width, sizes[i].height);
        str.append(buffer);
    }
    return str;
}

static String8 create_fps_str(const android:: FPSRange* fps, int len) {
    String8 str;
    char buffer[32];

    if (len > 0) {
        snprintf(buffer, sizeof(buffer), "(%d,%d)", fps[0].minFPS, fps[0].maxFPS);
        str.append(buffer);
    }
    for (int i = 1; i < len; i++) {
        snprintf(buffer, sizeof(buffer), ",(%d,%d)", fps[i].minFPS, fps[i].maxFPS);
        str.append(buffer);
    }
    return str;
}

static String8 create_values_str(const str_map *values, int len) {
    String8 str;

    if (len > 0) {
        str.append(values[0].desc);
    }
    for (int i = 1; i < len; i++) {
        str.append(",");
        str.append(values[i].desc);
    }
    return str;
}

static String8 create_str(int16_t *arr, int length){
    String8 str;
    char buffer[32];

    if(length > 0){
        snprintf(buffer, sizeof(buffer), "%d", arr[0]);
        str.append(buffer);
    }

    for (int i =1;i<length;i++){
        snprintf(buffer, sizeof(buffer), ",%d",arr[i]);
        str.append(buffer);
    }
    return str;
}

static String8 create_values_range_str(int min, int max){
    String8 str;
    char buffer[32];

    if(min <= max){
        snprintf(buffer, sizeof(buffer), "%d", min);
        str.append(buffer);

        for (int i = min + 1; i <= max; i++) {
            snprintf(buffer, sizeof(buffer), ",%d", i);
            str.append(buffer);
        }
    }
    return str;
}


extern "C" {
//------------------------------------------------------------------------
//   : 720p busyQ funcitons
//   --------------------------------------------------------------------
static struct fifo_queue g_busy_frame_queue =
    {0, 0, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, (char *)"frame_queue"};
};
/*===========================================================================
 * FUNCTION      cam_frame_wait_video
 *
 * DESCRIPTION    this function waits a video in the busy queue
 * ===========================================================================*/

static void cam_frame_wait_video (void)
{
    ALOGV("cam_frame_wait_video E ");
    if ((g_busy_frame_queue.num_of_frames) <=0){
        pthread_cond_wait(&(g_busy_frame_queue.wait), &(g_busy_frame_queue.mut));
    }
    ALOGV("cam_frame_wait_video X");
    return;
}

/*===========================================================================
 * FUNCTION      cam_frame_flush_video
 *
 * DESCRIPTION    this function deletes all the buffers in  busy queue
 * ===========================================================================*/
void cam_frame_flush_video (void)
{
    ALOGV("cam_frame_flush_video: in n = %d\n", g_busy_frame_queue.num_of_frames);
    pthread_mutex_lock(&(g_busy_frame_queue.mut));

    while (g_busy_frame_queue.front)
    {
       //dequeue from the busy queue
       struct fifo_node *node  = dequeue (&g_busy_frame_queue);
       if(node)
           free(node);

       ALOGV("cam_frame_flush_video: node \n");
    }
    pthread_mutex_unlock(&(g_busy_frame_queue.mut));
    ALOGV("cam_frame_flush_video: out n = %d\n", g_busy_frame_queue.num_of_frames);
    return ;
}
/*===========================================================================
 * FUNCTION      cam_frame_get_video
 *
 * DESCRIPTION    this function returns a video frame from the head
 * ===========================================================================*/
static struct msm_frame * cam_frame_get_video()
{
    struct msm_frame *p = NULL;
    ALOGV("cam_frame_get_video... in\n");
    ALOGV("cam_frame_get_video... got lock\n");
    if (g_busy_frame_queue.front)
    {
        //dequeue
       struct fifo_node *node  = dequeue (&g_busy_frame_queue);
       if (node)
       {
           p = (struct msm_frame *)node->f;
           free (node);
       }
       ALOGV("cam_frame_get_video... out = %lx\n", p->buffer);
    }
    return p;
}

/*===========================================================================
 * FUNCTION      cam_frame_post_video
 *
 * DESCRIPTION    this function add a busy video frame to the busy queue tails
 * ===========================================================================*/
static void cam_frame_post_video (struct msm_frame *p)
{
    if (!p)
    {
        ALOGE("post video , buffer is null");
        return;
    }
    ALOGV("cam_frame_post_video... in = %x\n", (unsigned int)(p->buffer));
    pthread_mutex_lock(&(g_busy_frame_queue.mut));
    ALOGV("post_video got lock. q count before enQ %d", g_busy_frame_queue.num_of_frames);
    //enqueue to busy queue
    struct fifo_node *node = (struct fifo_node *)malloc (sizeof (struct fifo_node));
    if (node)
    {
        ALOGV(" post video , enqueing in busy queue");
        node->f = p;
        node->next = NULL;
        enqueue (&g_busy_frame_queue, node);
        ALOGV("post_video got lock. q count after enQ %d", g_busy_frame_queue.num_of_frames);
    }
    else
    {
        ALOGE("cam_frame_post_video error... out of memory\n");
    }

    pthread_mutex_unlock(&(g_busy_frame_queue.mut));
    pthread_cond_signal(&(g_busy_frame_queue.wait));

    ALOGV("cam_frame_post_video... out = %lx\n", p->buffer);

    return;
}

void QualcommCameraHardware::storeTargetType(void) {
    char mDeviceName[PROPERTY_VALUE_MAX];
    property_get("ro.board.platform",mDeviceName," ");
    mCurrentTarget = TARGET_MAX;
    for( int i = 0; i < TARGET_MAX ; i++) {
        if( !strncmp(mDeviceName, targetList[i].targetStr, 7)) {
            mCurrentTarget = targetList[i].targetEnum;
            break;
        }
    }
    mCurrentTarget=TARGET_MSM8660;
    ALOGV("Storing the current target type as %d ", mCurrentTarget );
    return;
}

void *openCamera(void *data) {
    ALOGV("openCamera: E");
    mCameraOpen = false;

    if (!libmmcamera) {
        ALOGE("FATAL ERROR: could not dlopen liboemcamera.so: %s", dlerror());
        return NULL;
    }

    *(void **)&LINK_mm_camera_init =
        ::dlsym(libmmcamera, "mm_camera_init");

    *(void **)&LINK_mm_camera_exec =
        ::dlsym(libmmcamera, "mm_camera_exec");

    // Hard coding it to 0 for MSM_CAMERA. Will change with 3D camera support
    if (MM_CAMERA_SUCCESS != LINK_mm_camera_init(&mCfgControl, &mCamNotify, &mCamOps, 0)) {
        ALOGE("startCamera: mm_camera_init failed");
        return NULL;
    }

    if (MM_CAMERA_SUCCESS != LINK_mm_camera_exec()) {
        ALOGE("startCamera: mm_camera_exec failed:");
        return NULL;
    }

    mCameraOpen = true;
    ALOGV("openCamera: X");

    return NULL;
}
//-------------------------------------------------------------------------------------
static Mutex singleton_lock;
static bool singleton_releasing;
static nsecs_t singleton_releasing_start_time;
static const nsecs_t SINGLETON_RELEASING_WAIT_TIME = seconds_to_nanoseconds(5);
static const nsecs_t SINGLETON_RELEASING_RECHECK_TIMEOUT = seconds_to_nanoseconds(1);
static Condition singleton_wait;

static void receive_camframe_callback(struct msm_frame *frame);
static void receive_liveshot_callback(liveshot_status status, uint32_t jpeg_size);
static void receive_camstats_callback(camstats_type stype, camera_preview_histogram_info* histinfo);
static void receive_camframe_video_callback(struct msm_frame *frame); // 720p
static void receive_jpeg_fragment_callback(uint8_t *buff_ptr, uint32_t buff_size);
static void receive_jpeg_callback(jpeg_event_t status);
static void receive_shutter_callback(common_crop_t *crop);
static void receive_camframe_error_callback(camera_error_type err);
static int fb_fd = -1;
static int32_t mMaxZoom = 0;
static bool zoomSupported = false;
static int dstOffset = 0;

static int16_t * zoomRatios;
static int camerafd = -1;

/* When using MDP zoom, double the preview buffers. The usage of these
 * buffers is as follows:
 * 1. As all the buffers comes under a single FD, and at initial registration,
 * this FD will be passed to surface flinger, surface flinger can have access
 * to all the buffers when needed.
 * 2. Only "kPreviewBufferCount" buffers (SrcSet) will be registered with the
 * camera driver to receive preview frames. The remaining buffers (DstSet),
 * will be used at HAL and by surface flinger only when crop information
 * is present in the frame.
 * 3. When there is no crop information, there will be no call to MDP zoom,
 * and the buffers in SrcSet will be passed to surface flinger to display.
 * 4. With crop information present, MDP zoom will be called, and the final
 * data will be placed in a buffer from DstSet, and this buffer will be given
 * to surface flinger to display.
 */
#define NUM_MORE_BUFS 2

QualcommCameraHardware::QualcommCameraHardware()
    : mParameters(),
      mCameraRunning(false),
      mPreviewInitialized(false),
      mFrameThreadRunning(false),
      mVideoThreadRunning(false),
      mSnapshotThreadRunning(false),
      mJpegThreadRunning(false),
      mInSnapshotMode(false),
      mEncodePending(false),
      mBuffersInitialized(false),
      mSnapshotFormat(0),
      mFirstFrame(true),
      mReleasedRecordingFrame(false),
      mPreviewFrameSize(0),
      mRawSize(0),
      mCbCrOffsetRaw(0),
      mYOffset(0),
      mAutoFocusThreadRunning(false),
      mInitialized(false),
      mBrightness(0),
      mSkinToneEnhancement(0),
      mHJR(0),
      mRawMapped (NULL),
      mJpegMapped(NULL),
      mInPreviewCallback(false),
      mPreviewWindow(NULL),
      mMsgEnabled(0),
      mNotifyCallback(0),
      mDataCallback(0),
      mDataCallbackTimestamp(0),
      mCallbackCookie(0),
      mDebugFps(0),
      mSnapshotDone(0),
      maxSnapshotWidth(0),
      maxSnapshotHeight(0),
      mHasAutoFocusSupport(0),
      mDisEnabled(0),
      mRotation(0),
      mResetWindowCrop(false),
      mThumbnailWidth(0),
      mThumbnailHeight(0),
      mTotalPreviewBufferCount(0),
      strTexturesOn(false),
      mPrevHeapDeallocRunning(false),
      mSnapshotCancel(false)
{
    ALOGI("QualcommCameraHardware constructor E");
    mMMCameraDLRef = MMCameraDL::getInstance();
    libmmcamera = mMMCameraDLRef->pointer();
    ALOGV("%s, libmmcamera: %p\n", __FUNCTION__, libmmcamera);
    char value[PROPERTY_VALUE_MAX];

    mCameraOpen = false;

    storeTargetType();

    mRawMapped = NULL;
    mJpegMapped = NULL;
    mThumbnailMapped = 0;
    mJpegCopyMapped = NULL;
    for(int i=0; i< RECORD_BUFFERS; i++) {
        mRecordMapped[i] = NULL;
    }

    if( (pthread_create(&mDeviceOpenThread, NULL, openCamera, NULL)) != 0) {
        ALOGE(" openCamera thread creation failed ");
    }

    memset(&mDimension, 0, sizeof(mDimension));
    memset(&mCrop, 0, sizeof(mCrop));
    memset(&zoomCropInfo, 0, sizeof(android_native_rect_t));
    property_get("persist.debug.sf.showfps", value, "0");
    mDebugFps = atoi(value);
    if( mCurrentTarget == TARGET_MSM7630 || mCurrentTarget == TARGET_MSM8660 ) {
        kPreviewBufferCountActual = kPreviewBufferCount;
        kRecordBufferCount = RECORD_BUFFERS;
        recordframes = new msm_frame[kRecordBufferCount];
        record_buffers_tracking_flag = new bool[kRecordBufferCount];
    }
    else {
        kPreviewBufferCountActual = kPreviewBufferCount + NUM_MORE_BUFS;
        if( mCurrentTarget == TARGET_QSD8250 ) {
            kRecordBufferCount = RECORD_BUFFERS_8x50;
            recordframes = new msm_frame[kRecordBufferCount];
            record_buffers_tracking_flag = new bool[kRecordBufferCount];
        }
    }

    mTotalPreviewBufferCount = kTotalPreviewBufferCount;
    switch(mCurrentTarget){
        case TARGET_MSM7627:
            jpegPadding = 0; // to be checked.
            break;
        case TARGET_QSD8250:
        case TARGET_MSM7630:
        case TARGET_MSM8660:
            jpegPadding = 0;
            break;
        default:
            jpegPadding = 0;
            break;
    }
    // Initialize with default format values. The format values can be
    // overriden when application requests.
    mDimension.prev_format     = CAMERA_YUV_420_NV21;
    mPreviewFormat             = CAMERA_YUV_420_NV21;
    mDimension.enc_format      = CAMERA_YUV_420_NV21;
    if ((mCurrentTarget == TARGET_MSM7630) || (mCurrentTarget == TARGET_MSM8660))
        mDimension.enc_format  = CAMERA_YUV_420_NV12;

    mDimension.main_img_format = CAMERA_YUV_420_NV21;
    mDimension.thumb_format    = CAMERA_YUV_420_NV21;

    if ((mCurrentTarget == TARGET_MSM7630) || (mCurrentTarget == TARGET_MSM8660)) {
        /* DIS is enabled all the time in VPE support targets.
         * No provision for the user to control this.
         */
        mDisEnabled = 1;
        /* Get the DIS value from properties, to check whether
         * DIS is disabled or not
         */
        property_get("persist.camera.hal.dis", value, "1");
        mDisEnabled = atoi(value);
        mVpeEnabled = 1;
    }

    ALOGV("constructor EX");
}

void QualcommCameraHardware::hasAutoFocusSupport(){
    if( !mCamOps.mm_camera_is_supported(CAMERA_OPS_FOCUS)){
        ALOGE("AutoFocus is not supported");
        mHasAutoFocusSupport = false;
    }else {
        mHasAutoFocusSupport = true;
    }
}

//filter Picture sizes based on max width and height
void QualcommCameraHardware::filterPictureSizes(){
    ALOGV("%s E", __FUNCTION__);
    unsigned int i;
    if(PICTURE_SIZE_COUNT <= 0)
        return;
    maxSnapshotWidth = picture_sizes[0].width;
    maxSnapshotHeight = picture_sizes[0].height;
    // Iterate through all the width and height to find the max value
    for(i=0; i<PICTURE_SIZE_COUNT;i++){
        if (((maxSnapshotWidth < picture_sizes[i].width) &&
            (maxSnapshotHeight <= picture_sizes[i].height))){
            maxSnapshotWidth = picture_sizes[i].width;
            maxSnapshotHeight = picture_sizes[i].height;
        }
    }
    picture_sizes_ptr = picture_sizes;
    supportedPictureSizesCount = PICTURE_SIZE_COUNT;
}

bool QualcommCameraHardware::supportsSceneDetection() {
   unsigned int prop = 0;
   for(prop=0; prop<sizeof(boardProperties)/sizeof(board_property); prop++) {
       if((mCurrentTarget == boardProperties[prop].target)
          && boardProperties[prop].hasSceneDetect == true) {
           return true;
           break;
       }
   }
   return false;
}

bool QualcommCameraHardware::supportsSelectableZoneAf() {
   unsigned int prop = 0;
   for (prop=0; prop<sizeof(boardProperties)/sizeof(board_property); prop++) {
       if((mCurrentTarget == boardProperties[prop].target)
          && boardProperties[prop].hasSelectableZoneAf == true) {
           return true;
           break;
       }
   }
   return false;
}

bool QualcommCameraHardware::supportsFaceDetection() {
   unsigned int prop = 0;
   for (prop=0; prop<sizeof(boardProperties)/sizeof(board_property); prop++) {
       if((mCurrentTarget == boardProperties[prop].target)
          && boardProperties[prop].hasFaceDetect == true) {
           return true;
           break;
       }
   }
   return false;
}

void QualcommCameraHardware::initDefaultParameters()
{
    ALOGI("initDefaultParameters E");

    /* Set the default dimensions */
    mDimension.picture_width = DEFAULT_PICTURE_WIDTH;
    mDimension.picture_height = DEFAULT_PICTURE_HEIGHT;
    mDimension.ui_thumbnail_width =
        thumbnail_sizes[DEFAULT_THUMBNAIL_SETTING].width;
    mDimension.ui_thumbnail_height =
        thumbnail_sizes[DEFAULT_THUMBNAIL_SETTING].height;

    /* Update dimensions. Otherwise CAMERA_PARM_ZOOM_RATIO will fail */
    bool ret = native_set_parms(CAMERA_PARM_DIMENSION,
                               sizeof(cam_ctrl_dimension_t), &mDimension);

    hasAutoFocusSupport();
    //Disable DIS for Web Camera
    if (!mCfgControl.mm_camera_is_supported(CAMERA_PARM_VIDEO_DIS)) {
        ALOGV("DISABLE DIS");
        mDisEnabled = 0;
    } else {
        ALOGV("Enable DIS");
    }
    // Initialize constant parameter strings. This will happen only once in the
    // lifetime of the mediaserver process.
    if (!parameter_string_initialized) {
        antibanding_values = create_values_str(
            antibanding, sizeof(antibanding) / sizeof(str_map));
        effect_values = create_values_str(
            effects, sizeof(effects) / sizeof(str_map));
        autoexposure_values = create_values_str(
            autoexposure, sizeof(autoexposure) / sizeof(str_map));
        whitebalance_values = create_values_str(
            whitebalance, sizeof(whitebalance) / sizeof(str_map));

        //filter picture sizes
        filterPictureSizes();
        picture_size_values = create_sizes_str(
                picture_sizes_ptr, supportedPictureSizesCount);
        preview_size_values = create_sizes_str(
                preview_sizes,  PREVIEW_SIZE_COUNT);

        fps_ranges_supported_values = create_fps_str(
            FpsRangesSupported,FPS_RANGES_SUPPORTED_COUNT );
        mParameters.set(
            QCameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE,
            fps_ranges_supported_values);
        mParameters.setPreviewFpsRange(MINIMUM_FPS*1000,MAXIMUM_FPS*1000);

        flash_values = create_values_str(
            flash, sizeof(flash) / sizeof(str_map));
        if(mHasAutoFocusSupport){
            focus_mode_values = create_values_str(
                    focus_modes, sizeof(focus_modes) / sizeof(str_map));
        }
        iso_values = create_values_str(
            iso,sizeof(iso)/sizeof(str_map));
        lensshade_values = create_values_str(
            lensshade,sizeof(lensshade)/sizeof(str_map));

        //Currently Enabling Histogram for 8x60
        if(mCurrentTarget == TARGET_MSM8660) {
            histogram_values = create_values_str(
                histogram,sizeof(histogram)/sizeof(str_map));
        }

        //Currently Enabling Skin Tone Enhancement for 8x60 and 7630
        if((mCurrentTarget == TARGET_MSM8660)||(mCurrentTarget == TARGET_MSM7630)) {
            skinToneEnhancement_values = create_values_str(
                skinToneEnhancement,sizeof(skinToneEnhancement)/sizeof(str_map));
        }
        if(mHasAutoFocusSupport){
            touchafaec_values = create_values_str(
                touchafaec,sizeof(touchafaec)/sizeof(str_map));
        }

        picture_format_values = create_values_str(
            picture_formats, sizeof(picture_formats)/sizeof(str_map));

        if(mHasAutoFocusSupport) {
            continuous_af_values = create_values_str(
                continuous_af, sizeof(continuous_af) / sizeof(str_map));
        }

        if (mCfgControl.mm_camera_query_parms(CAMERA_PARM_ZOOM_RATIO, (void **)&zoomRatios, (uint32_t *) &mMaxZoom) == MM_CAMERA_SUCCESS) {
            zoomSupported = true;
            if (mMaxZoom >0) {
                ALOGE("Maximum zoom value is %d", mMaxZoom);
                if(zoomRatios != NULL) {
                    zoom_ratio_values =  create_str(zoomRatios, mMaxZoom);
                } else {
                     ALOGE("Failed to get zoomratios ..");
                }
           } else {
               zoomSupported = false;
           }
        } else {
            zoomSupported = false;
            ALOGE("Failed to get maximum zoom value...setting max "
                    "zoom to zero");
            mMaxZoom = 0;
        }

        preview_frame_rate_values = create_values_range_str(
            MINIMUM_FPS, MAXIMUM_FPS);

        scenemode_values = create_values_str(
            scenemode, sizeof(scenemode) / sizeof(str_map));

        if (supportsSceneDetection()) {
            scenedetect_values = create_values_str(
                scenedetect, sizeof(scenedetect) / sizeof(str_map));
        }

        if (mHasAutoFocusSupport && supportsSelectableZoneAf()) {
            selectable_zone_af_values = create_values_str(
                selectable_zone_af, sizeof(selectable_zone_af) / sizeof(str_map));
        }

        if (mHasAutoFocusSupport && supportsFaceDetection()) {
            facedetection_values = create_values_str(
                facedetection, sizeof(facedetection) / sizeof(str_map));
        }
        parameter_string_initialized = true;
    }

    mParameters.setVideoSize(DEFAULT_VIDEO_WIDTH, DEFAULT_VIDEO_HEIGHT);
    mParameters.setPreviewSize(DEFAULT_PREVIEW_WIDTH, DEFAULT_PREVIEW_HEIGHT);
    mDimension.display_width = DEFAULT_PREVIEW_WIDTH;
    mDimension.display_height = DEFAULT_PREVIEW_HEIGHT;

    mParameters.setPreviewFrameRate(DEFAULT_FPS);
    mParameters.setPreviewFpsRange(MINIMUM_FPS*1000, MAXIMUM_FPS*1000);
    if (mCfgControl.mm_camera_is_supported(CAMERA_PARM_FPS)){
        mParameters.set(
            QCameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES,
            preview_frame_rate_values.string());
    } else {
        mParameters.setPreviewFrameRate(DEFAULT_FPS);
        mParameters.set(
            QCameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES,
            DEFAULT_FPS);
    }
    mParameters.setPreviewFrameRateMode("frame-rate-auto");
    mParameters.setPreviewFormat(QCameraParameters::PIXEL_FORMAT_YUV420SP); // informative
    mParameters.set("overlay-format", QCameraParameters::PIXEL_FORMAT_YUV420SP);

    mParameters.setPictureSize(DEFAULT_PICTURE_WIDTH, DEFAULT_PICTURE_HEIGHT);
    mParameters.setPictureFormat("jpeg"); // informative
    mParameters.set(QCameraParameters::KEY_VIDEO_FRAME_FORMAT, QCameraParameters::PIXEL_FORMAT_YUV420SP);

    mParameters.set("power-mode-supported", "false");

    mParameters.set(QCameraParameters::KEY_JPEG_QUALITY, "85"); // max quality
    mParameters.set(QCameraParameters::KEY_JPEG_THUMBNAIL_WIDTH,
                    THUMBNAIL_WIDTH_STR); // informative
    mParameters.set(QCameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT,
                    THUMBNAIL_HEIGHT_STR); // informative
    mParameters.set(QCameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, "90");

    String8 valuesStr = create_sizes_str(jpeg_thumbnail_sizes, JPEG_THUMBNAIL_SIZE_COUNT);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES,
                valuesStr.string());

    if (zoomSupported) {
        mParameters.set(QCameraParameters::KEY_ZOOM_SUPPORTED, "true");
        ALOGV("max zoom is %d", mMaxZoom-1);
        /* mMaxZoom value that the query interface returns is the size
         * of zoom table. So the actual max zoom value will be one
         * less than that value.
         */
        mParameters.set("max-zoom",mMaxZoom-1);
        mParameters.set(QCameraParameters::KEY_ZOOM_RATIOS,
                            zoom_ratio_values);
    } else {
        mParameters.set(QCameraParameters::KEY_ZOOM_SUPPORTED, "false");
    }
    /* Enable zoom support for video application if VPE enabled */
    if (zoomSupported && mVpeEnabled) {
        mParameters.set("video-zoom-support", "true");
    } else {
        mParameters.set("video-zoom-support", "false");
    }

    mParameters.set(QCameraParameters::KEY_CAMERA_MODE,0);

    mParameters.set(QCameraParameters::KEY_ANTIBANDING,
                    QCameraParameters::ANTIBANDING_OFF);
    mParameters.set(QCameraParameters::KEY_EFFECT,
                    QCameraParameters::EFFECT_NONE);
    mParameters.set(QCameraParameters::KEY_AUTO_EXPOSURE,
                    QCameraParameters::AUTO_EXPOSURE_FRAME_AVG);
    mParameters.set(QCameraParameters::KEY_WHITE_BALANCE,
                    QCameraParameters::WHITE_BALANCE_AUTO);
    if ((mCurrentTarget != TARGET_MSM7630) && (mCurrentTarget != TARGET_QSD8250)
        && (mCurrentTarget != TARGET_MSM8660)) {
        mParameters.set(QCameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS,
                    QCameraParameters::PIXEL_FORMAT_YUV420SP);
    } else {
        preview_format_values = create_values_str(
            preview_formats, sizeof(preview_formats) / sizeof(str_map));
        mParameters.set(QCameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS,
                preview_format_values.string());
    }

    frame_rate_mode_values = create_values_str(
            frame_rate_modes, sizeof(frame_rate_modes) / sizeof(str_map));
    if (mCfgControl.mm_camera_is_supported(CAMERA_PARM_FPS_MODE)){
        mParameters.set(QCameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATE_MODES,
                    frame_rate_mode_values.string());
    }

    mParameters.set(QCameraParameters::KEY_SUPPORTED_PREVIEW_SIZES,
                    preview_size_values.string());

    mParameters.set(QCameraParameters::KEY_SUPPORTED_VIDEO_SIZES,
                    preview_size_values.string());

    mParameters.set(QCameraParameters::KEY_SUPPORTED_PICTURE_SIZES,
                    picture_size_values.string());
    mParameters.set(QCameraParameters::KEY_SUPPORTED_ANTIBANDING,
                    antibanding_values);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_EFFECTS, effect_values);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_AUTO_EXPOSURE, autoexposure_values);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_WHITE_BALANCE,
                    whitebalance_values);
    if (mHasAutoFocusSupport) {
       mParameters.set(QCameraParameters::KEY_SUPPORTED_FOCUS_MODES,
                    focus_mode_values);
       mParameters.set(QCameraParameters::KEY_FOCUS_MODE,
                    QCameraParameters::FOCUS_MODE_AUTO);
    } else {
       mParameters.set(QCameraParameters::KEY_SUPPORTED_FOCUS_MODES,
                   QCameraParameters::FOCUS_MODE_INFINITY);
       mParameters.set(QCameraParameters::KEY_FOCUS_MODE,
                   QCameraParameters::FOCUS_MODE_INFINITY);
    }

    mParameters.set(QCameraParameters::KEY_SUPPORTED_PICTURE_FORMATS,
                    picture_format_values);

    if (mCfgControl.mm_camera_is_supported(CAMERA_PARM_LED_MODE)) {
        mParameters.set(QCameraParameters::KEY_FLASH_MODE,
                        QCameraParameters::FLASH_MODE_OFF);
        mParameters.set(QCameraParameters::KEY_SUPPORTED_FLASH_MODES,
                        flash_values);
    }

    mParameters.set(QCameraParameters::KEY_MAX_SHARPNESS,
            CAMERA_MAX_SHARPNESS);
    mParameters.set(QCameraParameters::KEY_MAX_CONTRAST,
            CAMERA_MAX_CONTRAST);
    mParameters.set(QCameraParameters::KEY_MAX_SATURATION,
            CAMERA_MAX_SATURATION);

    mParameters.set(
            QCameraParameters::KEY_MAX_EXPOSURE_COMPENSATION,
            EXPOSURE_COMPENSATION_MAXIMUM_NUMERATOR);
    mParameters.set(
            QCameraParameters::KEY_MIN_EXPOSURE_COMPENSATION,
            EXPOSURE_COMPENSATION_MINIMUM_NUMERATOR);
    mParameters.set(
            QCameraParameters::KEY_EXPOSURE_COMPENSATION,
            EXPOSURE_COMPENSATION_DEFAULT_NUMERATOR);
    mParameters.setFloat(
            QCameraParameters::KEY_EXPOSURE_COMPENSATION_STEP,
            EXPOSURE_COMPENSATION_STEP);

    mParameters.set("luma-adaptation", "3");
    mParameters.set("skinToneEnhancement", "0");
    mParameters.set("zoom-supported", "true");
    mParameters.set("zoom", 0);
    mParameters.set(QCameraParameters::KEY_PICTURE_FORMAT,
                    QCameraParameters::PIXEL_FORMAT_JPEG);

    mParameters.set(QCameraParameters::KEY_SHARPNESS,
                    CAMERA_DEF_SHARPNESS);
    mParameters.set(QCameraParameters::KEY_CONTRAST,
                    CAMERA_DEF_CONTRAST);
    mParameters.set(QCameraParameters::KEY_SATURATION,
                    CAMERA_DEF_SATURATION);

    mParameters.set(QCameraParameters::KEY_ISO_MODE,
                    QCameraParameters::ISO_AUTO);
    mParameters.set(QCameraParameters::KEY_LENSSHADE,
                    QCameraParameters::LENSSHADE_ENABLE);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_ISO_MODES,
                    iso_values);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_LENSSHADE_MODES,
                    lensshade_values);
    mParameters.set(QCameraParameters::KEY_HISTOGRAM,
                    QCameraParameters::HISTOGRAM_DISABLE);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_HISTOGRAM_MODES,
                    histogram_values);
    mParameters.set(QCameraParameters::KEY_SKIN_TONE_ENHANCEMENT,
                    QCameraParameters::SKIN_TONE_ENHANCEMENT_DISABLE);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_SKIN_TONE_ENHANCEMENT_MODES,
                    skinToneEnhancement_values);
    mParameters.set(QCameraParameters::KEY_SCENE_MODE,
                    QCameraParameters::SCENE_MODE_AUTO);
    mParameters.set("strtextures", "OFF");

    mParameters.set(QCameraParameters::KEY_SUPPORTED_SCENE_MODES,
                    scenemode_values);
    mParameters.set("continuous-af", "caf-off");
    mParameters.set("continuous-af-values",
                    continuous_af_values);
    mParameters.set(QCameraParameters::KEY_TOUCH_AF_AEC,
                    QCameraParameters::TOUCH_AF_AEC_OFF);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_TOUCH_AF_AEC,
                    touchafaec_values);
    mParameters.setTouchIndexAec(-1, -1);
    mParameters.setTouchIndexAf(-1, -1);
    mParameters.set("touchAfAec-dx","100");
    mParameters.set("touchAfAec-dy","100");
    mParameters.set(QCameraParameters::KEY_MAX_NUM_FOCUS_AREAS, "1");
    mParameters.set(QCameraParameters::KEY_MAX_NUM_METERING_AREAS, "1");
    mParameters.set(QCameraParameters::KEY_SCENE_DETECT,
                    QCameraParameters::SCENE_DETECT_OFF);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_SCENE_DETECT,
                    scenedetect_values);
    mParameters.setFloat(QCameraParameters::KEY_FOCAL_LENGTH,
                    CAMERA_FOCAL_LENGTH_DEFAULT);
    mParameters.setFloat(QCameraParameters::KEY_HORIZONTAL_VIEW_ANGLE,
                    CAMERA_HORIZONTAL_VIEW_ANGLE_DEFAULT);
    mParameters.setFloat(QCameraParameters::KEY_VERTICAL_VIEW_ANGLE,
                    CAMERA_VERTICAL_VIEW_ANGLE_DEFAULT);
    mParameters.set(QCameraParameters::KEY_SELECTABLE_ZONE_AF,
                    QCameraParameters::SELECTABLE_ZONE_AF_AUTO);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_SELECTABLE_ZONE_AF,
                    selectable_zone_af_values);
    mParameters.set(QCameraParameters::KEY_FACE_DETECTION,
                    QCameraParameters::FACE_DETECTION_OFF);
    mParameters.set(QCameraParameters::KEY_SUPPORTED_FACE_DETECTION,
                    facedetection_values);
    mParameters.set(QCameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO,
                    "640x480");
    if (setParameters(mParameters) != NO_ERROR) {
        ALOGE("Failed to set default parameters?!");
    }

    /* Initialize the camframe_timeout_flag*/
    Mutex::Autolock l(&mCamframeTimeoutLock);
    camframe_timeout_flag = FALSE;

    /* Initialize heaps */
    mStatHeap = NULL;
    mMetaDataHeap = NULL;

    mInitialized = true;
    strTexturesOn = false;

    ALOGI("initDefaultParameters X");
}

#define ROUND_TO_PAGE(x)  (((x)+0xfff)&~0xfff)

bool QualcommCameraHardware::startCamera()
{
    ALOGV("startCamera E");
    if (mCurrentTarget == TARGET_MAX) {
        ALOGE(" Unable to determine the target type. Camera will not work ");
        return false;
    }
    ALOGV("%s, libmmcamera: %p\n", __FUNCTION__, libmmcamera);
#if DLOPEN_LIBMMCAMERA
    if (!libmmcamera) {
        ALOGE("FATAL ERROR: could not dlopen liboemcamera.so: %s", dlerror());
        return false;
    }

    *(void **)&LINK_cam_frame =
        ::dlsym(libmmcamera, "cam_frame");
    *(void **)&LINK_camframe_terminate =
        ::dlsym(libmmcamera, "camframe_terminate");

    *(void **)&LINK_jpeg_encoder_init =
        ::dlsym(libmmcamera, "jpeg_encoder_init");

    *(void **)&LINK_jpeg_encoder_encode =
        ::dlsym(libmmcamera, "jpeg_encoder_encode");

    *(void **)&LINK_jpeg_encoder_join =
        ::dlsym(libmmcamera, "jpeg_encoder_join");

    mCamNotify.preview_frame_cb = &receive_camframe_callback;

    mCamNotify.camstats_cb = &receive_camstats_callback;

    mCamNotify.jpegfragment_cb = &receive_jpeg_fragment_callback;

    mCamNotify.on_jpeg_event =  &receive_jpeg_callback;

    mCamNotify.on_error_event = &receive_camframe_error_callback;

    // 720 p new recording functions
    *(void **)&LINK_cam_frame_flush_free_video = ::dlsym(libmmcamera, "cam_frame_flush_free_video");

    *(void **)&LINK_camframe_free_video = ::dlsym(libmmcamera, "cam_frame_add_free_video");

    mCamNotify.video_frame_cb = &receive_camframe_video_callback;

    *(void **)&LINK_mmcamera_shutter_callback =
        ::dlsym(libmmcamera, "mmcamera_shutter_callback");

    *LINK_mmcamera_shutter_callback = receive_shutter_callback;

    *(void**)&LINK_jpeg_encoder_setMainImageQuality =
        ::dlsym(libmmcamera, "jpeg_encoder_setMainImageQuality");

    *(void**)&LINK_jpeg_encoder_setThumbnailQuality =
        ::dlsym(libmmcamera, "jpeg_encoder_setThumbnailQuality");

    *(void**)&LINK_jpeg_encoder_setRotation =
        ::dlsym(libmmcamera, "jpeg_encoder_setRotation");

    *(void**)&LINK_jpeg_encoder_get_buffer_offset =
        ::dlsym(libmmcamera, "jpeg_encoder_get_buffer_offset");

/* Disabling until support is available.
    *(void**)&LINK_jpeg_encoder_setLocation =
        ::dlsym(libmmcamera, "jpeg_encoder_setLocation");
*/
    *(void **)&LINK_cam_conf =
        ::dlsym(libmmcamera, "cam_conf");

/* Disabling until support is available.
    *(void **)&LINK_default_sensor_get_snapshot_sizes =
        ::dlsym(libmmcamera, "default_sensor_get_snapshot_sizes");
*/
    *(void **)&LINK_launch_cam_conf_thread =
        ::dlsym(libmmcamera, "launch_cam_conf_thread");

    *(void **)&LINK_release_cam_conf_thread =
        ::dlsym(libmmcamera, "release_cam_conf_thread");

    mCamNotify.on_liveshot_event = &receive_liveshot_callback;

    *(void **)&LINK_cancel_liveshot =
        ::dlsym(libmmcamera, "cancel_liveshot");

    *(void **)&LINK_set_liveshot_params =
        ::dlsym(libmmcamera, "set_liveshot_params");

    *(void **)&LINK_mm_camera_deinit =
        ::dlsym(libmmcamera, "mm_camera_deinit");

    *(void **)&LINK_mm_camera_destroy =
        ::dlsym(libmmcamera, "mm_camera_destroy");

/* Disabling until support is available.
    *(void **)&LINK_zoom_crop_upscale =
        ::dlsym(libmmcamera, "zoom_crop_upscale");
*/

#else
    mCamNotify.preview_frame_cb = &receive_camframe_callback;
    mCamNotify.camstats_cb = &receive_camstats_callback;
    mCamNotify.jpegfragment_cb = &receive_jpeg_fragment_callback;
    mCamNotify.on_jpeg_event =  &receive_jpeg_callback;

    mmcamera_shutter_callback = receive_shutter_callback;
    mCamNotify.on_liveshot_event = &receive_liveshot_callback;
    mCamNotify.video_frame_cb = &receive_camframe_video_callback;
#endif // DLOPEN_LIBMMCAMERA

    /* The control thread is in libcamera itself. */
    int ret_val;
    if (pthread_join(mDeviceOpenThread, (void**)&ret_val) != 0) {
         ALOGE("openCamera thread exit failed");
         return false;
    }

    if (!mCameraOpen) {
        ALOGE("openCamera() failed");
        return false;
    }

    mCfgControl.mm_camera_query_parms(CAMERA_PARM_PICT_SIZE, (void **)&picture_sizes, &PICTURE_SIZE_COUNT);
    if ((picture_sizes == NULL) || (!PICTURE_SIZE_COUNT)) {
        ALOGE("startCamera X: could not get snapshot sizes");
        return false;
    }
    ALOGV("startCamera picture_sizes %p PICTURE_SIZE_COUNT %d", picture_sizes, PICTURE_SIZE_COUNT);

    mCfgControl.mm_camera_query_parms(CAMERA_PARM_PREVIEW_SIZE, (void **)&preview_sizes, &PREVIEW_SIZE_COUNT);
    if ((preview_sizes == NULL) || (!PREVIEW_SIZE_COUNT)) {
        ALOGE("startCamera X: could not get preview sizes");
        return false;
    }
    ALOGV("startCamera preview_sizes %p previewSizeCount %d", preview_sizes, PREVIEW_SIZE_COUNT);

    ALOGV("startCamera X");
    return true;
}

status_t QualcommCameraHardware::dump(int fd,
                                      const Vector<String16>& args) const
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;

    // Dump internal primitives.
    result.append("QualcommCameraHardware::dump");
    snprintf(buffer, 255, "mMsgEnabled (%d)\n", mMsgEnabled);
    result.append(buffer);
    int width, height;
    mParameters.getPreviewSize(&width, &height);
    snprintf(buffer, 255, "preview width(%d) x height (%d)\n", width, height);
    result.append(buffer);
    mParameters.getPictureSize(&width, &height);
    snprintf(buffer, 255, "raw width(%d) x height (%d)\n", width, height);
    result.append(buffer);
    snprintf(buffer, 255,
             "preview frame size(%d), raw size (%d), jpeg size (%d) "
             "and jpeg max size (%d)\n", mPreviewFrameSize, mRawSize,
             mJpegSize, mJpegMaxSize);
    result.append(buffer);
    write(fd, result.string(), result.size());

    // Dump internal objects.
    mParameters.dump(fd, args);
    return NO_ERROR;
}

/* Issue ioctl calls related to starting Camera Operations*/
bool static native_start_ops(mm_camera_ops_type_t  type, void* value)
{
    if (mCamOps.mm_camera_start(type, value,NULL) != MM_CAMERA_SUCCESS) {
        ALOGE("native_start_ops: type %d error %s",
            type,strerror(errno));
        return false;
    }
    return true;
}

/* Issue ioctl calls related to stopping Camera Operations*/
bool static native_stop_ops(mm_camera_ops_type_t  type, void* value)
{
     if (mCamOps.mm_camera_stop(type, value,NULL) != MM_CAMERA_SUCCESS) {
        ALOGE("native_stop_ops: type %d error %s",
            type,strerror(errno));
        return false;
    }
    return true;
}
/*==========================================================================*/

static cam_frame_start_parms frame_parms;
static int recordingState = 0;

#define GPS_PROCESSING_METHOD_SIZE  101
#define FOCAL_LENGTH_DECIMAL_PRECISON 100

static const char ExifAsciiPrefix[] = { 0x41, 0x53, 0x43, 0x49, 0x49, 0x0, 0x0, 0x0 };
#define EXIF_ASCII_PREFIX_SIZE (sizeof(ExifAsciiPrefix))

static rat_t latitude[3];
static rat_t longitude[3];
static char lonref[2];
static char latref[2];
static rat_t altitude;
static rat_t gpsTimestamp[3];
static char gpsDatestamp[20];
static char dateTime[20];
static rat_t focalLength;
static char gpsProcessingMethod[EXIF_ASCII_PREFIX_SIZE + GPS_PROCESSING_METHOD_SIZE];

static void addExifTag(exif_tag_id_t tagid, exif_tag_type_t type,
                        uint32_t count, uint8_t copy, void *data) {
    ALOGV("%s E", __FUNCTION__);

    if (exif_table_numEntries == MAX_EXIF_TABLE_ENTRIES) {
        ALOGE("Number of entries exceeded limit");
        return;
    }

    int index = exif_table_numEntries;
    exif_data[index].tag_id = tagid;
    exif_data[index].tag_entry.type = type;
    exif_data[index].tag_entry.count = count;
    exif_data[index].tag_entry.copy = copy;
    if((type == EXIF_RATIONAL) && (count > 1))
        exif_data[index].tag_entry.data._rats = (rat_t *)data;
    if((type == EXIF_RATIONAL) && (count == 1))
        exif_data[index].tag_entry.data._rat = *(rat_t *)data;
    else if(type == EXIF_ASCII)
        exif_data[index].tag_entry.data._ascii = (char *)data;
    else if(type == EXIF_BYTE)
        exif_data[index].tag_entry.data._byte = *(uint8_t *)data;

    // Increase number of entries
    exif_table_numEntries++;
    return;
}

static void parseLatLong(const char *latlonString, int *pDegrees,
                           int *pMinutes, int *pSeconds ) {
    ALOGV("%s E", __FUNCTION__);

    double value = atof(latlonString);
    value = fabs(value);
    int degrees = (int) value;

    double remainder = value - degrees;
    int minutes = (int) (remainder * 60);
    int seconds = (int) (((remainder * 60) - minutes) * 60 * 1000);

    *pDegrees = degrees;
    *pMinutes = minutes;
    *pSeconds = seconds;
}

static void setLatLon(exif_tag_id_t tag, const char *latlonString) {

    int degrees, minutes, seconds;

    parseLatLong(latlonString, &degrees, &minutes, &seconds);

    rat_t value[3] = { {(unsigned)degrees, 1},
                       {(unsigned)minutes, 1},
                       {(unsigned)seconds, 1000} };

    if (tag == EXIFTAGID_GPS_LATITUDE) {
        memcpy(latitude, value, sizeof(latitude));
        addExifTag(EXIFTAGID_GPS_LATITUDE, EXIF_RATIONAL, 3,
                    1, (void *)latitude);
    } else {
        memcpy(longitude, value, sizeof(longitude));
        addExifTag(EXIFTAGID_GPS_LONGITUDE, EXIF_RATIONAL, 3,
                    1, (void *)longitude);
    }
}

void QualcommCameraHardware::setGpsParameters() {
    const char *str = NULL;
    ALOGV("%s E", __FUNCTION__);

    str = mParameters.get(QCameraParameters::KEY_GPS_PROCESSING_METHOD);
    if (str!=NULL) {
       memcpy(gpsProcessingMethod, ExifAsciiPrefix, EXIF_ASCII_PREFIX_SIZE);
       strlcpy(gpsProcessingMethod + EXIF_ASCII_PREFIX_SIZE, str,
           GPS_PROCESSING_METHOD_SIZE-1);
       gpsProcessingMethod[EXIF_ASCII_PREFIX_SIZE + GPS_PROCESSING_METHOD_SIZE-1] = '\0';
       addExifTag(EXIFTAGID_GPS_PROCESSINGMETHOD, EXIF_ASCII,
           EXIF_ASCII_PREFIX_SIZE + strlen(gpsProcessingMethod + EXIF_ASCII_PREFIX_SIZE) + 1,
           1, (void *)gpsProcessingMethod);
    }

    //Set Latitude
    str = NULL;
    str = mParameters.get(QCameraParameters::KEY_GPS_LATITUDE);
    if (str != NULL) {
        setLatLon(EXIFTAGID_GPS_LATITUDE, str);
        float latitudeValue = mParameters.getFloat(QCameraParameters::KEY_GPS_LATITUDE);
        latref[0] = 'N';
        if (latitudeValue < 0 ){
            latref[0] = 'S';
        }
        latref[1] = '\0';
        mParameters.set(QCameraParameters::KEY_GPS_LATITUDE_REF, latref);
        addExifTag(EXIFTAGID_GPS_LATITUDE_REF, EXIF_ASCII, 2,
                                1, (void *)latref);
    }

    //set Longitude
    str = NULL;
    str = mParameters.get(QCameraParameters::KEY_GPS_LONGITUDE);
    if (str != NULL) {
        setLatLon(EXIFTAGID_GPS_LONGITUDE, str);
        //set Longitude Ref
        float longitudeValue = mParameters.getFloat(QCameraParameters::KEY_GPS_LONGITUDE);
        lonref[0] = 'E';
        if (longitudeValue < 0){
            lonref[0] = 'W';
        }
        lonref[1] = '\0';
        mParameters.set(QCameraParameters::KEY_GPS_LONGITUDE_REF, lonref);
        addExifTag(EXIFTAGID_GPS_LONGITUDE_REF, EXIF_ASCII, 2,
                                1, (void *)lonref);
    }

    //set Altitude
    str = NULL;
    str = mParameters.get(QCameraParameters::KEY_GPS_ALTITUDE);
    if (str != NULL) {
        double value = atof(str);
        int ref = 0;
        if (value < 0){
            ref = 1;
            value = -value;
        }
        uint32_t value_meter = value * 1000;
        rat_t alt_value = {value_meter, 1000};
        memcpy(&altitude, &alt_value, sizeof(altitude));
        addExifTag(EXIFTAGID_GPS_ALTITUDE, EXIF_RATIONAL, 1,
                    1, (void *)&altitude);
        //set AltitudeRef
        mParameters.set(QCameraParameters::KEY_GPS_ALTITUDE_REF, ref);
        addExifTag(EXIFTAGID_GPS_ALTITUDE_REF, EXIF_BYTE, 1,
                    1, (void *)&ref);
    }

    //set Gps TimeStamp
    str = NULL;
    str = mParameters.get(QCameraParameters::KEY_GPS_TIMESTAMP);
    if (str != NULL) {
      long value = atol(str);
      time_t unixTime;
      struct tm *UTCTimestamp;

      unixTime = (time_t)value;
      UTCTimestamp = gmtime(&unixTime);

      strftime(gpsDatestamp, sizeof(gpsDatestamp), "%Y:%m:%d", UTCTimestamp);
      addExifTag(EXIFTAGID_GPS_DATESTAMP, EXIF_ASCII,
                          strlen(gpsDatestamp)+1 , 1, (void *)&gpsDatestamp);

      rat_t time_value[3] = { {(unsigned)UTCTimestamp->tm_hour, 1},
                              {(unsigned)UTCTimestamp->tm_min, 1},
                              {(unsigned)UTCTimestamp->tm_sec, 1} };


      memcpy(&gpsTimestamp, &time_value, sizeof(gpsTimestamp));
      addExifTag(EXIFTAGID_GPS_TIMESTAMP, EXIF_RATIONAL,
                  3, 1, (void *)&gpsTimestamp);
    }
}

bool QualcommCameraHardware::native_jpeg_encode(void)
{
    ALOGV("%s E", __FUNCTION__);
    int jpeg_quality = mParameters.getInt("jpeg-quality");
    if (jpeg_quality >= 0) {
        //Application can pass quality of zero
        //when there is no back sensor connected.
        //as jpeg quality of zero is not accepted at
        //camera stack, pass default value.
        if (jpeg_quality == 0) jpeg_quality = 85;
        ALOGV("native_jpeg_encode, current jpeg main img quality =%d",
             jpeg_quality);
        if (!LINK_jpeg_encoder_setMainImageQuality(jpeg_quality)) {
            ALOGE("native_jpeg_encode set jpeg-quality failed");
            return false;
        }
    }

    int thumbnail_quality = mParameters.getInt("jpeg-thumbnail-quality");
    if (thumbnail_quality >= 0) {
        //Application can pass quality of zero
        //when there is no back sensor connected.
        //as quality of zero is not accepted at
        //camera stack, pass default value.
        if (thumbnail_quality == 0) thumbnail_quality = 85;
        ALOGV("native_jpeg_encode, current jpeg thumbnail quality =%d",
             thumbnail_quality);
        if (!LINK_jpeg_encoder_setThumbnailQuality(thumbnail_quality)) {
            ALOGE("native_jpeg_encode set thumbnail-quality failed");
            return false;
        }
    }

    if ((mCurrentTarget != TARGET_MSM7630) && (mCurrentTarget != TARGET_MSM7627) && (mCurrentTarget != TARGET_MSM8660)) {
        int rotation = mParameters.getInt("rotation");
        if (rotation >= 0) {
            ALOGV("native_jpeg_encode, rotation = %d", rotation);
            if (!LINK_jpeg_encoder_setRotation(rotation)) {
                ALOGE("native_jpeg_encode set rotation failed");
                return false;
            }
        }
    }

    jpeg_set_location();

    //set TimeStamp
    const char *str = mParameters.get(QCameraParameters::KEY_EXIF_DATETIME);
    if (str != NULL) {
      strlcpy(dateTime, str, 20);
      addExifTag(EXIFTAGID_EXIF_DATE_TIME_ORIGINAL, EXIF_ASCII,
                  20, 1, (void *)dateTime);
    }

    unsigned focalLengthValue = (unsigned) (mParameters.getFloat(
                QCameraParameters::KEY_FOCAL_LENGTH) * FOCAL_LENGTH_DECIMAL_PRECISON);
    rat_t focalLengthRational = {focalLengthValue, FOCAL_LENGTH_DECIMAL_PRECISON};
    memcpy(&focalLength, &focalLengthRational, sizeof(focalLengthRational));
    addExifTag(EXIFTAGID_FOCAL_LENGTH, EXIF_RATIONAL, 1,
                1, (void *)&focalLength);

    if ((mCurrentTarget == TARGET_MSM7630) ||
        (mCurrentTarget == TARGET_MSM8660) ||
        (mCurrentTarget == TARGET_MSM7627) ||
        (strTexturesOn == true) ) {
        // Pass the main image as thumbnail buffer, so that jpeg encoder will
        // generate thumbnail based on main image.
        // Set the input and output dimensions for thumbnail generation to main
        // image dimensions and required thumbanail size repectively, for the
        // encoder to do downscaling of the main image accordingly.
        mCrop.in1_w  = mDimension.orig_picture_dx;
        mCrop.in1_h  = mDimension.orig_picture_dy;
        /* For Adreno format on targets that don't use VFE other output
         * for postView, thumbnail_width and thumbnail_height has the
         * actual thumbnail dimensions.
         */
        mCrop.out1_w = mDimension.thumbnail_width;
        mCrop.out1_h = mDimension.thumbnail_height;
        /* For targets, that uses VFE other output for postview,
         * thumbnail_width and thumbnail_height has values based on postView
         * dimensions(mostly previewWidth X previewHeight), but not based on
         * required thumbnail dimensions. So, while downscaling, we need to
         * pass the actual thumbnail dimensions, not the postview dimensions.
         * mThumbnailWidth/Height has the required thumbnail dimensions, so
         * use them here.
         */
        if ((mCurrentTarget == TARGET_MSM7630)||
            (mCurrentTarget == TARGET_MSM7627) ||
            (mCurrentTarget == TARGET_MSM8660)) {
            mCrop.out1_w = mThumbnailWidth;
            mCrop.out1_h = mThumbnailHeight;
        }
        mDimension.thumbnail_width = mDimension.orig_picture_dx;
        mDimension.thumbnail_height = mDimension.orig_picture_dy;
        ALOGV("mCrop.in1_w = %d, mCrop.in1_h = %d", mCrop.in1_w, mCrop.in1_h);
        ALOGV("mCrop.out1_w = %d, mCrop.out1_h = %d", mCrop.out1_w, mCrop.out1_h);
        ALOGV("mDimension.thumbnail_width = %d, mDimension.thumbnail_height = %d", mDimension.thumbnail_width, mDimension.thumbnail_height);
        int CbCrOffset = -1;
        if (mPreviewFormat == CAMERA_YUV_420_NV21_ADRENO)
            CbCrOffset = mCbCrOffsetRaw;
        mCrop.in1_w = mDimension.orig_picture_dx - jpegPadding; // when cropping is enabled
        mCrop.in1_h = mDimension.orig_picture_dy - jpegPadding; // when cropping is enabled

        if (!LINK_jpeg_encoder_encode(&mDimension,
                                      (uint8_t *)mThumbnailMapped,
                                      mThumbnailfd,
                                      (uint8_t *)mRawMapped->data,
                                      mRawfd,
                                      &mCrop, exif_data, exif_table_numEntries,
                                      jpegPadding/2, CbCrOffset)) {
            ALOGE("native_jpeg_encode: jpeg_encoder_encode failed.");
            return false;
        }
    } else {
        if (!LINK_jpeg_encoder_encode(&mDimension,
                                     (uint8_t *)mThumbnailMapped,
                                     mThumbnailfd,
                                     (uint8_t *)mRawMapped->data,
                                     mRawfd,
                                     &mCrop, exif_data, exif_table_numEntries,
                                     jpegPadding/2, -1)) {
            ALOGE("native_jpeg_encode: jpeg_encoder_encode failed.");
            return false;
        }
    }
    return true;
}

bool QualcommCameraHardware::native_set_parms(
    mm_camera_parm_type_t type, uint16_t length, void *value)
{
    if (mCfgControl.mm_camera_set_parm(type,value) != MM_CAMERA_SUCCESS) {
        ALOGE("native_set_parms failed: type %d error %s",
            type,strerror(errno));
        return false;
    }
    return true;

}
bool QualcommCameraHardware::native_set_parms(
    mm_camera_parm_type_t type, uint16_t length, void *value, int *result)
{
    mm_camera_status_t status;
    status = mCfgControl.mm_camera_set_parm(type,value);
    ALOGV("native_set_parms status = %d", status);
    if (status == MM_CAMERA_SUCCESS || status == MM_CAMERA_ERR_INVALID_OPERATION) {
        *result = status ;
        return true;
    }
    ALOGE("%s: type %d, status %d", __FUNCTION__, type, status);
    *result = status;
    return false;
}

void QualcommCameraHardware::jpeg_set_location()
{
    bool encode_location = true;
    camera_position_type pt;

    ALOGV("%s E", __FUNCTION__);
#define PARSE_LOCATION(what,type,fmt,desc) do {                                \
        pt.what = 0;                                                           \
        const char *what##_str = mParameters.get("gps-"#what);                 \
        ALOGV("GPS PARM %s --> [%s]", "gps-"#what, what##_str);                 \
        if (what##_str) {                                                      \
            type what = 0;                                                     \
            if (sscanf(what##_str, fmt, &what) == 1)                           \
                pt.what = what;                                                \
            else {                                                             \
                ALOGE("GPS " #what " %s could not"                              \
                     " be parsed as a " #desc, what##_str);                    \
                encode_location = false;                                       \
            }                                                                  \
        }                                                                      \
        else {                                                                 \
            ALOGV("GPS " #what " not specified: "                               \
                 "defaulting to zero in EXIF header.");                        \
            encode_location = false;                                           \
       }                                                                       \
    } while(0)

    PARSE_LOCATION(timestamp, long, "%ld", "long");
    if (!pt.timestamp) pt.timestamp = time(NULL);
    PARSE_LOCATION(altitude, short, "%hd", "short");
    PARSE_LOCATION(latitude, double, "%lf", "double float");
    PARSE_LOCATION(longitude, double, "%lf", "double float");

#undef PARSE_LOCATION

    if (encode_location) {
        ALOGD("setting image location ALT %d LAT %lf LON %lf",
             pt.altitude, pt.latitude, pt.longitude);

        setGpsParameters();
        /* Disabling until support is available.
        if (!LINK_jpeg_encoder_setLocation(&pt)) {
            ALOGE("jpeg_set_location: LINK_jpeg_encoder_setLocation failed.");
        }
        */
    }
    else ALOGV("not setting image location");
}

static bool register_buf(int size,
                         int frame_size,
                         int cbcr_offset,
                         int yoffset,
                         int pmempreviewfd,
                         uint32_t offset,
                         uint8_t *buf,
                         int pmem_type,
                         bool vfe_can_write,
                         bool register_buffer = true);

void QualcommCameraHardware::runFrameThread(void *data)
{
    ALOGV("runFrameThread E");
    int type;
    int cnt;

    if(libmmcamera) {
        LINK_cam_frame(data);
    }
    ALOGV("runFrameThread: clearing preview buffers");
    // Cancelling previewBuffers and returning them to display before stopping preview
    // This will ensure that all preview buffers are available for dequeing when
    //startPreview is called again with the same ANativeWindow object (snapshot case). If the
    //ANativeWindow is a new one(camera-camcorder switch case) because the app passed a new
    //surface then buffers will be re-allocated and not returned from the old pool.
    relinquishBuffers();

    {
        int mBufferSize = previewWidth * previewHeight * 3/2;
        int mCbCrOffset = PAD_TO_WORD(previewWidth * previewHeight);
        ALOGI("unregistering all preview buffers");
        //unregister preview buffers. we are not deallocating here.
        for (int cnt = 0; cnt < mTotalPreviewBufferCount; ++cnt) {
            register_buf(mBufferSize,
                         mBufferSize,
                         mCbCrOffset,
                         0,
                         frames[cnt].fd,
                         0,
                         (uint8_t *)frames[cnt].buffer,
                         MSM_PMEM_PREVIEW,
                         false,
                         false );
        }
    }

    if ((mCurrentTarget == TARGET_MSM7630) || (mCurrentTarget == TARGET_QSD8250) || (mCurrentTarget == TARGET_MSM8660)) {
        ALOGV("runFrameThread: clearing mRecordHeap");
        int CbCrOffset = PAD_TO_2K(mDimension.video_width  * mDimension.video_height);
        for (int cnt = 0; cnt < kRecordBufferCount; cnt++) {
            type = MSM_PMEM_VIDEO;
            ALOGI("%s: unregister record buffer %d", __FUNCTION__, cnt);
            if (recordframes) {
                register_buf(mRecordFrameSize,
                    mRecordFrameSize, CbCrOffset, 0,
                    recordframes[cnt].fd,
                    0,
                    (uint8_t *)recordframes[cnt].buffer,
                    type,
                    false,false);
                if (mRecordMapped[cnt]) {
                    mRecordMapped[cnt]->release(mRecordMapped[cnt]);
                    close(mRecordfd[cnt]);
#ifdef USE_ION
                    deallocate_ion_memory(&record_main_ion_fd[cnt], &record_ion_info_fd[cnt]);
#endif
                }
            }
        }
    }

    mFrameThreadWaitLock.lock();
    mFrameThreadRunning = false;
    mFrameThreadWait.signal();
    mFrameThreadWaitLock.unlock();

    ALOGV("runFrameThread X");
}

int QualcommCameraHardware::mapBuffer(struct msm_frame *frame) {
    int ret = -1;
    for (int cnt = 0; cnt < mTotalPreviewBufferCount; cnt++) {
        if (frame_buffer[cnt].frame->buffer == frame->buffer) {
            ret = cnt;
            break;
        }
    }
    return ret;
}

int QualcommCameraHardware::mapvideoBuffer(struct msm_frame *frame)
{
    int ret = -1;
    for (int cnt = 0; cnt < kRecordBufferCount; cnt++) {
        if ((unsigned int)mRecordMapped[cnt]->data == (unsigned int)frame->buffer) {
            ret = cnt;
            break;
        }
    }
    return ret;
}

int QualcommCameraHardware::mapFrame(buffer_handle_t *buffer) {
    int ret = -1;
    for (int cnt = 0; cnt < mTotalPreviewBufferCount; cnt++) {
        if (frame_buffer[cnt].buffer == buffer) {
            ret = cnt;
            break;
        }
    }
    return ret;
}

void QualcommCameraHardware::runVideoThread(void *data)
{
    ALOGD("runVideoThread E");
    msm_frame* vframe = NULL;

    while(true) {
        pthread_mutex_lock(&(g_busy_frame_queue.mut));

        // Exit the thread , in case of stop recording..
        mVideoThreadWaitLock.lock();
        if (mVideoThreadExit) {
            ALOGV("Exiting video thread..");
            mVideoThreadWaitLock.unlock();
            pthread_mutex_unlock(&(g_busy_frame_queue.mut));
            break;
        }
        mVideoThreadWaitLock.unlock();

        ALOGV("in video_thread : wait for video frame ");
        // check if any frames are available in busyQ and give callback to
        // services/video encoder
        cam_frame_wait_video();
        ALOGV("video_thread, wait over..");

        // Exit the thread , in case of stop recording..
        mVideoThreadWaitLock.lock();
        if (mVideoThreadExit) {
            ALOGV("Exiting video thread..");
            mVideoThreadWaitLock.unlock();
            pthread_mutex_unlock(&(g_busy_frame_queue.mut));
            break;
        }
        mVideoThreadWaitLock.unlock();

        // Get the video frame to be encoded
        vframe = cam_frame_get_video ();
        pthread_mutex_unlock(&(g_busy_frame_queue.mut));
        ALOGV("in video_thread : got video frame");

        if (UNLIKELY(mDebugFps)) {
            debugShowVideoFPS();
        }

        if (vframe != NULL) {
            /* Extract the timestamp of this frame */
            nsecs_t timeStamp = nsecs_t(vframe->ts.tv_sec)*1000000000LL + vframe->ts.tv_nsec;

            // dump frames for test purpose
#ifdef DUMP_VIDEO_FRAMES
            static int frameCnt = 0;
            if (frameCnt >= 11 && frameCnt <= 13 ) {
                char buf[128];
                snprintf(buffer, sizeof(buf),  "/data/%d_v.yuv", frameCnt);
                int file_fd = open(buf, O_RDWR | O_CREAT, 0777);
                ALOGV("dumping video frame %d", frameCnt);
                if (file_fd < 0) {
                    ALOGE("cannot open file\n");
                }
                else {
                    write(file_fd, (const void *)vframe->buffer,
                        vframe->cbcr_off * 3 / 2);
                }
                close(file_fd);
            }
            frameCnt++;
#endif
            // Enable IF block to give frames to encoder , ELSE block for just simulation
            // FIXME: the video thread stops receiving frames after 5th so until it gets solved using preview frames for recording
#if 0
            ALOGV("in video_thread : got video frame, before if check giving frame to services/encoder");
            mCallbackLock.lock();
            int msgEnabled = mMsgEnabled;
            camera_data_timestamp_callback rcb = mDataCallbackTimestamp;
            void *rdata = mCallbackCookie;
            mCallbackLock.unlock();

            int index = mapvideoBuffer(vframe);
            record_buffers_tracking_flag[index] = true;
            if (rcb != NULL && (msgEnabled & CAMERA_MSG_VIDEO_FRAME)) {
                ALOGV("in video_thread : got video frame, giving frame to services/encoder index = %d", index);
                rcb(timeStamp, CAMERA_MSG_VIDEO_FRAME, mRecordMapped[index],0,rdata);
            } else {
                LINK_camframe_free_video(vframe);
            }
#else
            // 720p output2  : simulate release frame here:
            ALOGV("in video_thread simulation , releasing the video frame");
            LINK_camframe_free_video(vframe);
#endif
        } else ALOGE("in video_thread get frame returned null");
    } // end of while loop

    mVideoThreadWaitLock.lock();
    mVideoThreadRunning = false;
    mVideoThreadWait.signal();
    mVideoThreadWaitLock.unlock();

    ALOGV("runVideoThread X");
}

void *video_thread(void *user)
{
    ALOGV("video_thread E");
    QualcommCameraHardware *obj = QualcommCameraHardware::getInstance();
    if (obj != 0) {
        obj->runVideoThread(user);
    }
    else ALOGE("not starting video thread: the object went away!");
    ALOGV("video_thread X");
    return NULL;
}

void *frame_thread(void *user)
{
    ALOGD("frame_thread E");
    QualcommCameraHardware *obj = QualcommCameraHardware::getInstance();
    if (obj != 0) {
        obj->runFrameThread(user);
    }
    else ALOGW("not starting frame thread: the object went away!");
    ALOGD("frame_thread X");
    return NULL;
}

static int parse_size(const char *str, int &width, int &height)
{
    ALOGV("%s E", __FUNCTION__);
    // Find the width.
    char *end;
    int w = (int)strtol(str, &end, 10);
    // If an 'x' or 'X' does not immediately follow, give up.
    if ( (*end != 'x') && (*end != 'X') )
        return -1;

    // Find the height, immediately after the 'x'.
    int h = (int)strtol(end+1, 0, 10);

    width = w;
    height = h;

    return 0;
}

QualcommCameraHardware* current_cam;

int QualcommCameraHardware::allocate_ion_memory(int *main_ion_fd, struct ion_allocation_data* alloc,
     struct ion_fd_data* ion_info_fd, int ion_type, int size, int *memfd)
{
    int rc = 0;
    struct ion_handle_data handle_data;

    *main_ion_fd = open("/dev/ion", O_RDONLY | O_SYNC);
    if (*main_ion_fd < 0) {
        ALOGE("Ion dev open failed\n");
        ALOGE("Error is %s\n", strerror(errno));
        goto ION_OPEN_FAILED;
    }
    alloc->len = size;
    /* to make it page size aligned */
    alloc->len = (alloc->len + 4095) & (~4095);
    alloc->align = 4096;
    alloc->flags = ION_FLAG_CACHED;
    alloc->heap_mask = (1 << ion_type);

    ALOGV("ION allocation, len: %d, align: %d, flags: %d, mask: 0x%08x\n", alloc->len, alloc->align, alloc->flags, alloc->heap_mask);
    rc = ioctl(*main_ion_fd, ION_IOC_ALLOC, alloc);
    if (rc < 0) {
        ALOGE("ION allocation failed, rc: %d, errno: %d, error: %s\n", rc, errno, strerror(errno));
        goto ION_ALLOC_FAILED;
    }

    ion_info_fd->handle = alloc->handle;
    rc = ioctl(*main_ion_fd, ION_IOC_SHARE, ion_info_fd);
    if (rc < 0) {
        ALOGE("ION map failed %s\n", strerror(errno));
        goto ION_MAP_FAILED;
    }
    *memfd = ion_info_fd->fd; 
    return 0;

ION_MAP_FAILED:
    handle_data.handle = ion_info_fd->handle;
    ioctl(*main_ion_fd, ION_IOC_FREE, &handle_data);
ION_ALLOC_FAILED:
    close(*main_ion_fd);
ION_OPEN_FAILED:
    return -1;
}

int QualcommCameraHardware::deallocate_ion_memory(int *main_ion_fd, struct ion_fd_data* ion_info_fd)
{
    struct ion_handle_data handle_data;
    int rc = 0;

    handle_data.handle = ion_info_fd->handle;
    ioctl(*main_ion_fd, ION_IOC_FREE, &handle_data);
    close(*main_ion_fd);
    return rc;
}

bool QualcommCameraHardware::initPreview()
{
    ALOGV("%s E", __FUNCTION__);
    const char * pmem_region = "/dev/pmem_adsp";
    int ion_heap = ION_CAMERA_HEAP_ID;

    mParameters.getPreviewSize(&previewWidth, &previewHeight);
    ALOGV("initPreview: Got preview dimension as %d x %d ", previewWidth, previewHeight);
    const char *recordSize = NULL;
    recordSize = mParameters.get("record-size");

    if (!recordSize) {
        //If application didn't set this parameter string, use the values from
        //getPreviewSize() as video dimensions.
        ALOGV("No Record Size requested, use the preview dimensions");
        videoWidth = previewWidth;
        videoHeight = previewHeight;
    } else {
        //Extract the record witdh and height that application requested.
        if (!parse_size(recordSize, videoWidth, videoHeight)) {
            //VFE output1 shouldn't be greater than VFE output2.
            if ((previewWidth > videoWidth) || (previewHeight > videoHeight)) {
                //Set preview sizes as record sizes.
                ALOGI("Preview size %dx%d is greater than record size %dx%d,\
                    resetting preview size to record size",previewWidth,\
                    previewHeight, videoWidth, videoHeight);
                previewWidth = videoWidth;
                previewHeight = videoHeight;
                mParameters.setPreviewSize(previewWidth, previewHeight);
             }
             if ((mCurrentTarget != TARGET_MSM7630)
                && (mCurrentTarget != TARGET_QSD8250)
                && (mCurrentTarget != TARGET_MSM8660) ) {
                //For Single VFE output targets, use record dimensions as preview dimensions.
                previewWidth = videoWidth;
                previewHeight = videoHeight;
                mParameters.setPreviewSize(previewWidth, previewHeight);
            }
        } else {
            ALOGE("initPreview X: failed to parse parameter record-size (%s)", recordSize);
            return false;
        }
    }

    mDimension.display_width = previewWidth;
    mDimension.display_height = previewHeight;
    mDimension.ui_thumbnail_width =
        thumbnail_sizes[DEFAULT_THUMBNAIL_SETTING].width;
    mDimension.ui_thumbnail_height =
        thumbnail_sizes[DEFAULT_THUMBNAIL_SETTING].height;

    ALOGV("initPreview E: preview size=%d x %d videosize = %d x %d", previewWidth, previewHeight, videoWidth, videoHeight );

    if ((mCurrentTarget == TARGET_MSM7630) || (mCurrentTarget == TARGET_QSD8250) || (mCurrentTarget == TARGET_MSM8660)) {
        mDimension.video_width = CEILING16(videoWidth);
        /* Backup the video dimensions, as video dimensions in mDimension
         * will be modified when DIS is supported. Need the actual values
         * to pass ap part of VPE config
         */
        videoWidth = mDimension.video_width;
        mDimension.video_height = videoHeight;
        ALOGI("initPreview : preview size=%d x %d videosize = %d x %d", previewWidth, previewHeight,
            videoWidth, videoHeight);
    }

    // See comments in deinitPreview() for why we have to wait for the frame
    // thread here, and why we can't use pthread_join().
    mFrameThreadWaitLock.lock();
    while (mFrameThreadRunning) {
        ALOGI("initPreview: waiting for old frame thread to complete.");
        mFrameThreadWait.wait(mFrameThreadWaitLock);
        ALOGI("initPreview: old frame thread completed.");
    }
    mFrameThreadWaitLock.unlock();

    mInSnapshotModeWaitLock.lock();
    while (mInSnapshotMode) {
        ALOGI("initPreview: waiting for snapshot mode to complete.");
        mInSnapshotModeWait.wait(mInSnapshotModeWaitLock);
        ALOGI("initPreview: snapshot mode completed.");
    }
    mInSnapshotModeWaitLock.unlock();

    mPreviewFrameSize = previewWidth * previewHeight * 3/2;
    ALOGV("mPreviewFrameSize = %d, width = %d, height = %d \n",
        mPreviewFrameSize, previewWidth, previewHeight);
    int CbCrOffset = PAD_TO_WORD(previewWidth * previewHeight);

    //Pass the yuv formats, display dimensions,
    //so that vfe will be initialized accordingly.
    mDimension.display_luma_width = previewWidth;
    mDimension.display_luma_height = previewHeight;
    mDimension.display_chroma_width = previewWidth;
    mDimension.display_chroma_height = previewHeight;
    if (mPreviewFormat == CAMERA_YUV_420_NV21_ADRENO) {
        mPreviewFrameSize = PAD_TO_4K(CEILING32(previewWidth) * CEILING32(previewHeight)) +
                                     2 * (CEILING32(previewWidth/2) * CEILING32(previewHeight/2));
        CbCrOffset = PAD_TO_4K(CEILING32(previewWidth) * CEILING32(previewHeight));
        mDimension.prev_format = CAMERA_YUV_420_NV21_ADRENO;
        mDimension.display_luma_width = CEILING32(previewWidth);
        mDimension.display_luma_height = CEILING32(previewHeight);
        mDimension.display_chroma_width = 2 * CEILING32(previewWidth/2);
        //Chroma Height is not needed as of now. Just sending with other dimensions.
        mDimension.display_chroma_height = CEILING32(previewHeight/2);
    }
    ALOGV("mDimension.prev_format = %d", mDimension.prev_format);
    ALOGV("mDimension.display_luma_width = %d", mDimension.display_luma_width);
    ALOGV("mDimension.display_luma_height = %d", mDimension.display_luma_height);
    ALOGV("mDimension.display_chroma_width = %d", mDimension.display_chroma_width);
    ALOGV("mDimension.display_chroma_height = %d", mDimension.display_chroma_height);

    dstOffset = 0;
    //set DIS value to get the updated video width and height to calculate
    //the required record buffer size
    if (mVpeEnabled) {
        bool status = setDIS();
        if (status) {
            ALOGE("Failed to set DIS");
            return false;
        }
    }

    //Pass the original video width and height and get the required width
    //and height for record buffer allocation
    mDimension.orig_video_width = videoWidth;
    mDimension.orig_video_height = videoHeight;

    // mDimension will be filled with thumbnail_width, thumbnail_height,
    // orig_picture_dx, and orig_picture_dy after this function call. We need to
    // keep it for jpeg_encoder_encode.
    bool ret = native_set_parms(CAMERA_PARM_DIMENSION,
                               sizeof(cam_ctrl_dimension_t), &mDimension);

    //Restore video_width and video_height that might have been zeroed
    if (mDimension.video_width == 0 && mDimension.video_height == 0) {
        mDimension.video_width = videoWidth;
        mDimension.video_height = videoHeight;
    }

    if ((mCurrentTarget == TARGET_MSM7630 ) || (mCurrentTarget == TARGET_QSD8250) || (mCurrentTarget == TARGET_MSM8660)) {
        // Allocate video buffers after allocating preview buffers.
        bool status = initRecord();
        if (status != true) {
            ALOGE("Failed to allocate video bufers");
            return false;
        }
    }

    if (ret) {
        mFrameThreadWaitLock.lock();
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        frame_parms.frame = frames[kPreviewBufferCount - 1];
        if (mCurrentTarget == TARGET_MSM7630 || mCurrentTarget == TARGET_QSD8250 || mCurrentTarget == TARGET_MSM8660)
            frame_parms.video_frame =  recordframes[kPreviewBufferCount - 1];
        else
            frame_parms.video_frame =  frames[kPreviewBufferCount - 1];

        ALOGV("initpreview before cam_frame thread carete, video frame buffer=%lu fd=%d y_off=%d cbcr_off=%d \n",
            (unsigned long)frame_parms.video_frame.buffer, frame_parms.video_frame.fd, frame_parms.video_frame.y_off,
            frame_parms.video_frame.cbcr_off);
        mFrameThreadRunning = !pthread_create(&mFrameThread,
                                              &attr,
                                              frame_thread,
                                              (void*)&(frame_parms));
        ret = mFrameThreadRunning;
        mFrameThreadWaitLock.unlock();
    }
    mFirstFrame = true;

    ALOGV("initPreview X: %d", ret);
    return ret;
}

void QualcommCameraHardware::deinitPreview(void)
{
    ALOGI("deinitPreview E");

    // When we call deinitPreview(), we signal to the frame thread that it
    // needs to exit, but we DO NOT WAIT for it to complete here.  The problem
    // is that deinitPreview is sometimes called from the frame-thread's
    // callback, when the refcount on the Camera client reaches zero.  If we
    // called pthread_join(), we would deadlock.  So, we just call
    // LINK_camframe_terminate() in deinitPreview(), which makes sure that
    // after the preview callback returns, the camframe thread will exit.  We
    // could call pthread_join() in initPreview() to join the last frame
    // thread.  However, we would also have to call pthread_join() in release
    // as well, shortly before we destroy the object; this would cause the same
    // deadlock, since release(), like deinitPreview(), may also be called from
    // the frame-thread's callback.  This we have to make the frame thread
    // detached, and use a separate mechanism to wait for it to complete.

    LINK_camframe_terminate();
    ALOGI("deinitPreview X");
}

bool QualcommCameraHardware::initRawSnapshot()
{
    ALOGV("initRawSnapshot E");
    const char * pmem_region = "/dev/pmem_adsp";
    int ion_heap = ION_CP_MM_HEAP_ID;

    //get width and height from Dimension Object
    bool ret = native_set_parms(CAMERA_PARM_DIMENSION,
                               sizeof(cam_ctrl_dimension_t), &mDimension);
    if (!ret){
        ALOGE("initRawSnapshot X: failed to set dimension");
        return false;
    }

    int rawSnapshotSize = mDimension.raw_picture_height *
                           mDimension.raw_picture_width;

    ALOGV("raw_snapshot_buffer_size = %d, raw_picture_height = %d, "\
         "raw_picture_width = %d",
          rawSnapshotSize, mDimension.raw_picture_height,
          mDimension.raw_picture_width);

    ALOGV("initRawSnapshot X");
    return true;
}

bool QualcommCameraHardware::initRaw(bool initJpegHeap)
{
    int rawWidth, rawHeight;
    const char * pmem_region = "/dev/pmem_adsp";
    int ion_heap = ION_CP_MM_HEAP_ID;

    ALOGV("%s E", __FUNCTION__);
    mParameters.getPictureSize(&rawWidth, &rawHeight);
    ALOGV("initRaw E: picture size=%dx%d", rawWidth, rawHeight);

    int thumbnailBufferSize;
    //Thumbnail height should be smaller than Picture height
    if (rawHeight > (int)thumbnail_sizes[DEFAULT_THUMBNAIL_SETTING].height){
        mDimension.ui_thumbnail_width =
                thumbnail_sizes[DEFAULT_THUMBNAIL_SETTING].width;
        mDimension.ui_thumbnail_height =
                thumbnail_sizes[DEFAULT_THUMBNAIL_SETTING].height;
        uint32_t pictureAspectRatio = (uint32_t)((rawWidth * Q12) / rawHeight);
        uint32_t i;
        for (i = 0; i < THUMBNAIL_SIZE_COUNT; i++ ) {
            if (thumbnail_sizes[i].aspect_ratio == pictureAspectRatio) {
                mDimension.ui_thumbnail_width = thumbnail_sizes[i].width;
                mDimension.ui_thumbnail_height = thumbnail_sizes[i].height;
                break;
            }
        }
    }
    else {
        mDimension.ui_thumbnail_height = THUMBNAIL_SMALL_HEIGHT;
        mDimension.ui_thumbnail_width =
                (THUMBNAIL_SMALL_HEIGHT * rawWidth)/ rawHeight;
    }

    if ((mCurrentTarget == TARGET_MSM7630) ||
        (mCurrentTarget == TARGET_MSM7627) ||
        (mCurrentTarget == TARGET_MSM8660)) {
        if(rawHeight < previewHeight) {
            mDimension.ui_thumbnail_height = THUMBNAIL_SMALL_HEIGHT;
            mDimension.ui_thumbnail_width =
                    (THUMBNAIL_SMALL_HEIGHT * rawWidth)/ rawHeight;
        }
        /* store the thumbanil dimensions which are needed
         * by the jpeg downscaler to generate thumbnails from
         * main YUV image.
         */
        mThumbnailWidth = mDimension.ui_thumbnail_width;
        mThumbnailHeight = mDimension.ui_thumbnail_height;
        /* As thumbnail is generated from main YUV image,
         * configure and use the VFE other output to get
         * an image of preview dimensions for postView use.
         * So, mThumbnailHeap will be used for postview rather than
         * as thumbnail(Not changing the terminology to keep changes minimum).
         */
        if ((rawHeight >= previewHeight) &&
            (mCurrentTarget != TARGET_MSM7627)) {
            mDimension.ui_thumbnail_height = previewHeight;
            mDimension.ui_thumbnail_width =
                        (previewHeight * rawWidth) / rawHeight;
        }
    }

    ALOGV("Thumbnail Size Width %d Height %d",
            mDimension.ui_thumbnail_width,
            mDimension.ui_thumbnail_height);

    if (mPreviewFormat == CAMERA_YUV_420_NV21_ADRENO) {
        mDimension.main_img_format = CAMERA_YUV_420_NV21_ADRENO;
        mDimension.thumb_format = CAMERA_YUV_420_NV21_ADRENO;
    }

    // mDimension will be filled with thumbnail_width, thumbnail_height,
    // orig_picture_dx, and orig_picture_dy after this function call. We need to
    // keep it for jpeg_encoder_encode.
    bool ret = native_set_parms(CAMERA_PARM_DIMENSION,
                               sizeof(cam_ctrl_dimension_t), &mDimension);

    if (!ret) {
        ALOGE("initRaw X: failed to set dimension");
        return false;
    }

    thumbnailBufferSize = mDimension.ui_thumbnail_width *
                          mDimension.ui_thumbnail_height * 3 / 2;
    int CbCrOffsetThumb = PAD_TO_WORD(mDimension.ui_thumbnail_width *
                          mDimension.ui_thumbnail_height);
    if (mPreviewFormat == CAMERA_YUV_420_NV21_ADRENO) {
        thumbnailBufferSize = PAD_TO_4K(CEILING32(mDimension.ui_thumbnail_width) *
                              CEILING32(mDimension.ui_thumbnail_height)) +
                              2 * (CEILING32(mDimension.ui_thumbnail_width/2) *
                                CEILING32(mDimension.ui_thumbnail_height/2));
        CbCrOffsetThumb = PAD_TO_4K(CEILING32(mDimension.ui_thumbnail_width) *
                              CEILING32(mDimension.ui_thumbnail_height));
    }

    // Snapshot
    mRawSize = rawWidth * rawHeight * 3 / 2;
    mCbCrOffsetRaw = PAD_TO_WORD(rawWidth * rawHeight);

    if (mPreviewFormat == CAMERA_YUV_420_NV21_ADRENO) {
        mRawSize = PAD_TO_4K(CEILING32(rawWidth) * CEILING32(rawHeight)) +
                            2 * (CEILING32(rawWidth/2) * CEILING32(rawHeight/2));
        mCbCrOffsetRaw = PAD_TO_4K(CEILING32(rawWidth) * CEILING32(rawHeight));
    }

    if (mCurrentTarget == TARGET_MSM7627)
        mJpegMaxSize = CEILING16(rawWidth) * CEILING16(rawHeight) * 3 / 2;
    else {
        mJpegMaxSize = rawWidth * rawHeight * 3 / 2;
        if (mPreviewFormat == CAMERA_YUV_420_NV21_ADRENO) {
            mJpegMaxSize =
               PAD_TO_4K(CEILING32(rawWidth) * CEILING32(rawHeight)) +
                    2 * (CEILING32(rawWidth/2) * CEILING32(rawHeight/2));
        }
    }

    //For offline jpeg hw encoder, jpeg encoder will provide us the
    //required offsets and buffer size depending on the rotation.
    if ((mCurrentTarget == TARGET_MSM7630) || (mCurrentTarget == TARGET_MSM7627) || (mCurrentTarget == TARGET_MSM8660)) {
        int rotation = mParameters.getInt("rotation");
        if (rotation >= 0) {
            ALOGV("initRaw, jpeg_rotation = %d", rotation);
            if(!LINK_jpeg_encoder_setRotation(rotation)) {
                ALOGE("native_jpeg_encode set rotation failed");
                return false;
            }
        }
        //Don't call the get_buffer_offset() for ADRENO, as the width and height
        //for Adreno format will be of CEILING32.
        if (mPreviewFormat != CAMERA_YUV_420_NV21_ADRENO) {
            LINK_jpeg_encoder_get_buffer_offset(rawWidth, rawHeight, (uint32_t *)&mYOffset,
                                            (uint32_t *)&mCbCrOffsetRaw, (uint32_t *)&mRawSize);
            mJpegMaxSize = mRawSize;
        }
        ALOGV("initRaw: yOffset = %d, mCbCrOffsetRaw = %d, mRawSize = %d",
                     mYOffset, mCbCrOffsetRaw, mRawSize);
    }
    if ((mPreviewWindow != NULL) && (mThumbnailBuffer != NULL)) {
        mDisplayLock.lock();
        /* Lock the postview buffer before use */
        ALOGV(" Locking postview buffer ");

        if (mPreviewWindow->lock_buffer(mPreviewWindow,
                        mThumbnailBuffer) != NO_ERROR) {

            ALOGE(" Error locking postview buffer. Error = %d ", ret);
            mDisplayLock.unlock();
            return false;
        }
        if (GENLOCK_NO_ERROR != genlock_lock_buffer((native_handle_t*)(*mThumbnailBuffer),
                                                   GENLOCK_WRITE_LOCK, GENLOCK_MAX_TIMEOUT)) {
            ALOGE("%s: genlock_lock_buffer(WRITE) failed", __FUNCTION__);
            mDisplayLock.unlock();
            return -EINVAL;
        } else {
            mThumbnailLockState = BUFFER_LOCKED;
        }
        mDisplayLock.unlock();
    }

    mParameters.getPreviewSize(&previewWidth, &previewHeight);
    int mBufferSize = previewWidth * previewHeight * 3/2;
    int CbCrOffset = PAD_TO_WORD(previewWidth * previewHeight);
    private_handle_t *thumbnailHandle;
    if (mThumbnailBuffer) {
        thumbnailHandle = (private_handle_t *)(*mThumbnailBuffer);
        mThumbnailfd = thumbnailHandle->fd;
        ALOGV("fd thumbnailhandle fd %d size %d", thumbnailHandle->fd, thumbnailHandle->size);
        mThumbnailMapped = (unsigned int)mmap(0, thumbnailHandle->size, PROT_READ|PROT_WRITE,
            MAP_SHARED, thumbnailHandle->fd, 0);
        if ((void *)mThumbnailMapped == MAP_FAILED){
            ALOGE(" Couldnt map Thumbnail buffer %d", errno);
        }
        ALOGI("register thumbnail buffer");
        register_buf(mBufferSize,
                mBufferSize, CbCrOffset, 0,
                thumbnailHandle->fd,
                0,
                (uint8_t *)mThumbnailMapped,
                MSM_PMEM_THUMBNAIL,
                true, true);
    }

    ALOGV("initRaw: initializing mRawHeap.");
#ifdef USE_ION
    unsigned cnt = 0;
    if (allocate_ion_memory(&raw_main_ion_fd[cnt], &raw_alloc[cnt], &raw_ion_info_fd[cnt],
                            ion_heap, mJpegMaxSize, &mRawfd) < 0) {
        ALOGE("%s: ion allocation failed, heap=%d!\n", __func__, ion_heap);
        return NULL;
    }
#else
    mRawfd = open(pmem_region, O_RDWR|O_SYNC);
    if (mRawfd <= 0) {
        ALOGE("do_mmap: Open device %s failed!\n",pmem_region);
        return NULL;
    }
#endif
    ALOGV("%s Raw memory index: %d, fd is %d ", __func__, cnt, mRawfd);
    mRawMapped = mGetMemory(mRawfd,mJpegMaxSize,1,mCallbackCookie);
    if (mRawMapped==NULL) {
        ALOGE("Failed to get camera memory for Raw heap");
    } else {
        ALOGV("Received following info for raw mapped data:%p,handle:%p, size:%d,release:%p",
            mRawMapped->data, mRawMapped->handle, mRawMapped->size, mRawMapped->release);
    }
    // Register Raw frames
    ALOGI("Registering MAIN_IMG buffer %d with fd: %d", cnt, mRawfd);
    register_buf(mJpegMaxSize,
                mRawSize,
                mCbCrOffsetRaw,
                mYOffset,
                mRawfd,0,
                (uint8_t *)mRawMapped->data,
                MSM_PMEM_MAINIMG,
                true, true);

    //This is kind of workaround for the GPU limitation, as it can't
    //output in line to correct NV21 adreno formula for some snapshot
    //sizes (like 3264x2448). This change of cbcr offset will ensure that
    //chroma plane always starts at the beginning of a row.
    if (mPreviewFormat != CAMERA_YUV_420_NV21_ADRENO)
        mCbCrOffsetRaw = CEILING32(rawWidth) * CEILING32(rawHeight);

    // Jpeg
    int yOffsetThumb = 0;
    if (initJpegHeap) {
        ALOGV("%s Jpeg memory index: %d, fd is %d ", __func__, cnt, mJpegfd);
        mJpegMapped=mGetMemory(-1,mJpegMaxSize,kJpegBufferCount,mCallbackCookie);
        if (mJpegMapped==NULL) {
            ALOGE("Failed to get camera memory for jpeg heap");
            return false;
        } else {
            ALOGV("Received following info for jpeg mapped data:%p,handle:%p, size:%d,release:%p",
                mJpegMapped->data, mJpegMapped->handle, mJpegMapped->size,
                mJpegMapped->release);
        }
        // Thumbnails
        /* With the recent jpeg encoder downscaling changes for thumbnail padding,
         *  HAL needs to call this API to get the offsets and buffer size.
         */
        if ((mPreviewFormat != CAMERA_YUV_420_NV21_ADRENO)
            && (mCurrentTarget != TARGET_MSM7630)
            && (mCurrentTarget != TARGET_MSM8660)) {
            LINK_jpeg_encoder_get_buffer_offset(mDimension.thumbnail_width,
                                                mDimension.thumbnail_height,
                                                (uint32_t *)&yOffsetThumb,
                                                (uint32_t *)&CbCrOffsetThumb,
                                                (uint32_t *)&thumbnailBufferSize);
        }
    }

    ALOGV("initRaw X");
    return true;
}


void QualcommCameraHardware::deinitRawSnapshot()
{
    ALOGV("deinitRawSnapshot");
}

void QualcommCameraHardware::deinitRaw()
{
    ALOGV("deinitRaw E");
    if (NULL != mRawMapped) {
        ALOGI("Unregister MAIN_IMG");
        register_buf(mJpegMaxSize,
                mRawSize,
                mCbCrOffsetRaw,
                mYOffset,
                mRawfd,0,
                (uint8_t *)mRawMapped->data,
                MSM_PMEM_MAINIMG,
                false, false);
        mRawMapped->release(mRawMapped);
        close(mRawfd);
        mRawMapped = NULL;
#ifdef USE_ION
        int cnt = 0;
        deallocate_ion_memory(&raw_main_ion_fd[cnt], &raw_ion_info_fd[cnt]);
#endif
    }
    if (mPreviewWindow != NULL ) {
        ALOGV("deinitRaw, clearing/cancelling thumbnail buffers:");
        private_handle_t *handle;
        if (mPreviewWindow != NULL && mThumbnailBuffer != NULL) {
            handle = (private_handle_t *)(*mThumbnailBuffer);
            ALOGI("%s:  Cancelling postview buffer %d ", __FUNCTION__, handle->fd);
            mDisplayLock.lock();
            if (BUFFER_LOCKED == mThumbnailLockState) {
                if (GENLOCK_FAILURE == genlock_unlock_buffer(handle)) {
                   ALOGE("%s: genlock_unlock_buffer failed", __FUNCTION__);
                } else {
                   mThumbnailLockState = BUFFER_UNLOCKED;
                }
            }
            status_t retVal = mPreviewWindow->cancel_buffer(mPreviewWindow,
                                                              mThumbnailBuffer);
            if (retVal != NO_ERROR)
                ALOGE("%s: cancelBuffer failed for postview buffer %d",
                                                     __FUNCTION__, handle->fd);
            int mBufferSize = previewWidth * previewHeight * 3/2;
            int mCbCrOffset = PAD_TO_WORD(previewWidth * previewHeight);
            if (mThumbnailMapped) {
                ALOGI("%s: Unregistering Thumbnail Buffer %d ", __FUNCTION__, handle->fd);
                register_buf(mBufferSize,
                    mBufferSize, mCbCrOffset, 0,
                    handle->fd,
                    0,
                    (uint8_t *)mThumbnailMapped,
                    MSM_PMEM_THUMBNAIL,
                    false, false);
                 if (munmap((void *)(mThumbnailMapped),handle->size ) == -1) {
                    ALOGE("deinitraw : Error un-mmapping the thumbnail buffer");
                 }
                 mThumbnailBuffer = NULL;
                 mThumbnailMapped = 0;
            }
            ALOGV("deinitraw : display unlock");
            mDisplayLock.unlock();
        }
    }
    ALOGV("deinitRaw X");
}

void QualcommCameraHardware::relinquishBuffers() {
    status_t retVal;
    ALOGV("%s: E ", __FUNCTION__);
    mDisplayLock.lock();
    if (mPreviewWindow != NULL) {
        for (int cnt = 0; cnt < mTotalPreviewBufferCount; cnt++) {
         if (BUFFER_LOCKED == frame_buffer[cnt].lockState) {
            ALOGI("%s: Cancelling preview buffers %d", __FUNCTION__, frames[cnt].fd);
            if (GENLOCK_FAILURE == genlock_unlock_buffer((native_handle_t *)
                                              (*(frame_buffer[cnt].buffer)))) {
                    ALOGE("%s: genlock_unlock_buffer failed", __FUNCTION__);
                } else {
                    frame_buffer[cnt].lockState = BUFFER_UNLOCKED;
                }
            }
            retVal = mPreviewWindow->cancel_buffer(mPreviewWindow,
                                frame_buffer[cnt].buffer);
            mPreviewMapped[cnt]->release(mPreviewMapped[cnt]);
            if (retVal != NO_ERROR)
               ALOGE("%s: cancelBuffer failed for preview buffer %d ",
                 __FUNCTION__, frames[cnt].fd);
        }
    } else {
        ALOGV(" PreviewWindow is null, will not cancelBuffers ");
    }
    mDisplayLock.unlock();
    ALOGV("%s: X ", __FUNCTION__);
}

status_t QualcommCameraHardware::set_PreviewWindow(void* param)
{
    preview_stream_ops_t* window = (preview_stream_ops_t*)param;
    return setPreviewWindow(window);
}

status_t QualcommCameraHardware::setPreviewWindow(preview_stream_ops_t* window)
{
    status_t retVal = NO_ERROR;
    ALOGV("%s: E ", __FUNCTION__);
    if (window == NULL) {
        ALOGW("Setting NULL preview window");
        /* Current preview window will be invalidated.
         * Release all the buffers back */
        if (mPreviewWindow != NULL)
            relinquishBuffers();
    }
    ALOGI("Set preview window");
    mDisplayLock.lock();
    mPreviewWindow = window;
    mDisplayLock.unlock();

    if ((mPreviewWindow != NULL) && mCameraRunning) {
        /* Initial preview in progress. Stop it and start
         * the actual preview */
        stopInitialPreview();
        retVal = getBuffersAndStartPreview();
    }
    ALOGV(" %s : X ", __FUNCTION__ );
    return retVal;
}

status_t QualcommCameraHardware::getBuffersAndStartPreview() {
    status_t retVal = NO_ERROR;
    int stride;
    ALOGI("%s : E ", __FUNCTION__);
    mFrameThreadWaitLock.lock();
    while (mFrameThreadRunning) {
        ALOGV("%s: waiting for old frame thread to complete.", __FUNCTION__);
        mFrameThreadWait.wait(mFrameThreadWaitLock);
        ALOGV("%s: old frame thread completed.",__FUNCTION__);
    }
    mFrameThreadWaitLock.unlock();

    if (mPreviewWindow!= NULL) {
        ALOGV("%s: Calling native_window_set_buffer", __FUNCTION__);

        android_native_buffer_t *mPreviewBuffer;
        int32_t previewFormat;
        const char *str = mParameters.getPreviewFormat();
        int numMinUndequeuedBufs = 0;

        int err = mPreviewWindow->get_min_undequeued_buffer_count(mPreviewWindow,
           &numMinUndequeuedBufs);
        if (err != 0) {
            ALOGW("NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS query failed: %s (%d)",
                    strerror(-err), -err);
            return err;
        }
        mTotalPreviewBufferCount = kPreviewBufferCount + numMinUndequeuedBufs;

        previewFormat = attr_lookup(app_preview_formats,
            sizeof(app_preview_formats) / sizeof(str_map), str);
        if (previewFormat ==  NOT_FOUND) {
            previewFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        }

        retVal = mPreviewWindow->set_buffer_count(mPreviewWindow,
                            mTotalPreviewBufferCount + 1);
        if (retVal != NO_ERROR) {
            ALOGE("%s: Error while setting buffer count to %d ", __FUNCTION__, kPreviewBufferCount + 1);
            return retVal;
        }
        mParameters.getPreviewSize(&previewWidth, &previewHeight);

        retVal = mPreviewWindow->set_buffers_geometry(mPreviewWindow,
                     previewWidth, previewHeight, previewFormat);

        if (retVal != NO_ERROR) {
            ALOGE("%s: Error while setting buffer geometry ", __FUNCTION__);
            return retVal;
        }

#ifdef USE_ION
        mPreviewWindow->set_usage (mPreviewWindow,
             GRALLOC_USAGE_PRIVATE_CAMERA_HEAP |
             GRALLOC_USAGE_PRIVATE_UNCACHED);
#else
        mPreviewWindow->set_usage (mPreviewWindow,
            GRALLOC_USAGE_PRIVATE_ADSP_HEAP |
            GRALLOC_USAGE_PRIVATE_UNCACHED);
#endif

        int CbCrOffset = PAD_TO_WORD(previewWidth * previewHeight);
        int cnt = 0, active = 1;
        int mBufferSize = previewWidth * previewHeight * 3/2;
        for (cnt = 0; cnt < mTotalPreviewBufferCount; cnt++) {
            buffer_handle_t *bhandle =NULL;
            retVal = mPreviewWindow->dequeue_buffer(mPreviewWindow,
                                                   &(bhandle),
                                                   &(stride));
            if ((retVal == NO_ERROR)) {
                /* Acquire lock on the buffer if it was successfully
                 * dequeued from gralloc */
                ALOGV("Locking buffer %d ", cnt);
                retVal = mPreviewWindow->lock_buffer(mPreviewWindow,
                                            bhandle);
                // lock the buffer using genlock
                if (GENLOCK_NO_ERROR != genlock_lock_buffer((native_handle_t *)(*bhandle),
                                                      GENLOCK_WRITE_LOCK, GENLOCK_MAX_TIMEOUT)) {
                    ALOGE("%s: genlock_lock_buffer(WRITE) failed", __FUNCTION__);
                    return -EINVAL;
                }
                ALOGV(" Locked buffer %d successfully", cnt);
            } else {
                ALOGI("%s: dequeueBuffer failed for preview buffer. Error = %d",
                      __FUNCTION__, retVal);
                return retVal;
            }
            if (retVal == NO_ERROR) {
                private_handle_t *handle = (private_handle_t *)(*bhandle);//(private_handle_t *)mPreviewBuffer->handle;
                ALOGV("Handle %p, Fd passed:%d, Base:%d, Size %d",
                    handle, handle->fd, handle->base, handle->size);

                if (handle) {
                    mPreviewMapped[cnt] = mGetMemory(handle->fd,handle->size,1,mCallbackCookie);
                    if ((void *)mPreviewMapped[cnt] == NULL){
                        ALOGE("%s: Failed to get camera memory for Preview buffer %d", __FUNCTION__, cnt);
                        return UNKNOWN_ERROR;
                    }

                    ALOGV("Got the following from get_mem data: %p, handle :%p, release : %p, size: %d",
                        mPreviewMapped[cnt]->data,
                        mPreviewMapped[cnt]->handle,
                        mPreviewMapped[cnt]->release,
                        mPreviewMapped[cnt]->size);

                    if (((void *)mPreviewMapped[cnt]->data == MAP_FAILED)
                         || (mPreviewMapped[cnt]->data == 0)) {
                         ALOGE("%s: Couldnt map preview buffers", __FUNCTION__);
                         return UNKNOWN_ERROR;
                    }

                    frames[cnt].fd = handle->fd;
                    frames[cnt].buffer = (unsigned int)mPreviewMapped[cnt]->data;
                    frames[cnt].y_off = 0;
                    frames[cnt].cbcr_off= CbCrOffset;
                    frames[cnt].path = OUTPUT_TYPE_P; // MSM_FRAME_ENC;

                    frame_buffer[cnt].frame = &frames[cnt];
                    frame_buffer[cnt].buffer = bhandle;
                    frame_buffer[cnt].size = handle->size;
                    frame_buffer[cnt].lockState = BUFFER_LOCKED;
                    active = (cnt < ACTIVE_PREVIEW_BUFFERS);

                    ALOGI("Registering preview buffer %d with fd: %d with kernel",cnt,handle->fd);
                    register_buf(mBufferSize,
                             mBufferSize, CbCrOffset, 0,
                             handle->fd,
                             0,
                             (uint8_t *)frames[cnt].buffer,
                             MSM_PMEM_PREVIEW,
                             active,true);
                    ALOGV("Came back from register call to kernel");
                } else
                    ALOGE("%s: setPreviewWindow: Could not get buffer handle", __FUNCTION__);
            } else {
                ALOGE("%s: lockBuffer failed for preview buffer. Error = %d",
                         __FUNCTION__, retVal);
                return retVal;
            }
        }
        //DeQueue Thumbnail here
        retVal = mPreviewWindow->dequeue_buffer(mPreviewWindow,
                     &mThumbnailBuffer, &(stride));
        private_handle_t* handle = (private_handle_t *)(*mThumbnailBuffer);
        ALOGV("%s: dequeing thumbnail buffer fd %d", __FUNCTION__, handle->fd);
        if (retVal != NO_ERROR) {
            ALOGE("%s: dequeueBuffer failed for postview buffer. Error = %d ",
                   __FUNCTION__, retVal);
            return retVal;
        }

        // Cancel minUndequeuedBufs.
        for (cnt = kPreviewBufferCount; cnt < mTotalPreviewBufferCount; cnt++) {
            if (GENLOCK_FAILURE == genlock_unlock_buffer((native_handle_t *)
                                              (*(frame_buffer[cnt].buffer)))) {
                ALOGE("%s: genlock_unlock_buffer failed", __FUNCTION__);
                return -EINVAL;
            }
            frame_buffer[cnt].lockState = BUFFER_UNLOCKED;
            status_t retVal = mPreviewWindow->cancel_buffer(mPreviewWindow,
                                frame_buffer[cnt].buffer);
            ALOGV("Cancelling preview buffers %d ", frame_buffer[cnt].frame->fd);
        }
    } else {
        ALOGE("%s: Could not get Buffer from Surface", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    LINK_cam_frame_flush_free_video();
    for(int i=ACTIVE_PREVIEW_BUFFERS; i < kPreviewBufferCount; i++)
        LINK_camframe_free_video(&frames[i]);

    mBuffersInitialized = true;

    //Starting preview now as the preview buffers are allocated
    ALOGV("setPreviewWindow: Starting preview after buffer allocation, initialized: %d, running: %d, run: %d", mPreviewInitialized, mCameraRunning, (!mPreviewInitialized && !mCameraRunning));
    startPreviewInternal();

    ALOGI(" %s : X ",__FUNCTION__);
    return NO_ERROR;
}


void QualcommCameraHardware::release()
{
    ALOGI("release E");
    Mutex::Autolock l(&mLock);
    int cnt, rc;
    struct msm_ctrl_cmd ctrlCmd;
    ALOGV("release: mCameraRunning = %d", mCameraRunning);
    if (mCameraRunning) {
        if (mDataCallbackTimestamp && (mMsgEnabled & CAMERA_MSG_VIDEO_FRAME)) {
            mRecordFrameLock.lock();
            mReleasedRecordingFrame = true;
            mRecordWait.signal();
            mRecordFrameLock.unlock();
        }
        stopPreviewInternal();
        ALOGV("release: stopPreviewInternal done.");
    }
    LINK_jpeg_encoder_join();

    //Signal the snapshot thread
    mJpegThreadWaitLock.lock();
    mJpegThreadRunning = false;
    mJpegThreadWait.signal();
    mJpegThreadWaitLock.unlock();

    // Wait for snapshot thread to complete before clearing the
    // resources.
    mSnapshotThreadWaitLock.lock();
    while (mSnapshotThreadRunning) {
        ALOGV("takePicture: waiting for old snapshot thread to complete.");
        mSnapshotThreadWait.wait(mSnapshotThreadWaitLock);
        ALOGV("takePicture: old snapshot thread completed.");
    }
    mSnapshotThreadWaitLock.unlock();

    {
        Mutex::Autolock l (&mRawPictureHeapLock);
        deinitRaw();
    }

    deinitRawSnapshot();
    ALOGI("release: clearing resources done.");

    if (mStatHeap != NULL) {
       ALOGV("release: clearing mStatHeap");
       mStatHeap.clear();
       mStatHeap = NULL;
    }
    if (mMetaDataHeap != NULL) {
       ALOGV("release: clearing mMetaDataHeap");
       mMetaDataHeap.clear();
       mMetaDataHeap = NULL;
    }

    LINK_mm_camera_deinit();
    if (fb_fd >= 0) {
        close(fb_fd);
        fb_fd = -1;
    }

    ALOGV("release X: mCameraRunning = %d, mFrameThreadRunning = %d", mCameraRunning, mFrameThreadRunning);
    ALOGV("mVideoThreadRunning = %d, mSnapshotThreadRunning = %d, mJpegThreadRunning = %d", mVideoThreadRunning, mSnapshotThreadRunning, mJpegThreadRunning);
    ALOGV("camframe_timeout_flag = %d, mAutoFocusThreadRunning = %d", camframe_timeout_flag, mAutoFocusThreadRunning);
}

QualcommCameraHardware::~QualcommCameraHardware()
{
    ALOGI("~QualcommCameraHardware E");

    if (mCurrentTarget == TARGET_MSM7630 || mCurrentTarget == TARGET_QSD8250 || mCurrentTarget == TARGET_MSM8660) {
        delete [] recordframes;
        recordframes = NULL;
        delete [] record_buffers_tracking_flag;
        record_buffers_tracking_flag = NULL;
    }

    libmmcamera = NULL;
    mMMCameraDLRef.clear();

    ALOGI("~QualcommCameraHardware X");
}

status_t QualcommCameraHardware::startPreviewInternal()
{
    ALOGV("in startPreviewInternal : E");
    if (!mBuffersInitialized) {
        ALOGE("startPreviewInternal: Buffers not allocated. Cannot start preview");
        return NO_ERROR;
    }
    if (mCameraRunning) {
        ALOGV("startPreview X: preview already running.");
        return NO_ERROR;
    }

    if (!mPreviewInitialized) {
        mLastQueuedFrame = NULL;
        mPreviewInitialized = initPreview();
        if (!mPreviewInitialized) {
            ALOGE("startPreview X initPreview failed.  Not starting preview.");
            return UNKNOWN_ERROR;
        }
    }

    {
        Mutex::Autolock cameraRunningLock(&mCameraRunningLock);
        if(( mCurrentTarget != TARGET_MSM7630 ) &&
                (mCurrentTarget != TARGET_QSD8250) && (mCurrentTarget != TARGET_MSM8660))
            mCameraRunning = native_start_ops(CAMERA_OPS_STREAMING_PREVIEW, NULL);
        else
            mCameraRunning = native_start_ops(CAMERA_OPS_STREAMING_VIDEO, NULL);
    }

    if (!mCameraRunning) {
        deinitPreview();
        mPreviewInitialized = false;
        ALOGE("startPreview X: native_start_ops: CAMERA_OPS_STREAMING_PREVIEW ioctl failed!");
        return UNKNOWN_ERROR;
    }

    //Reset the Gps Information
    exif_table_numEntries = 0;

    ALOGV("startPreviewInternal X");
    return NO_ERROR;
}

status_t QualcommCameraHardware::startInitialPreview()
{
    mCameraRunning = DUMMY_CAMERA_STARTED;
    return NO_ERROR;
}

status_t QualcommCameraHardware::startPreview()
{
    status_t result;
    ALOGV("startPreview E");
    Mutex::Autolock l(&mLock);
    if (mPreviewWindow == NULL) {
        /* startPreview has been called before setting the preview
         * window. Start the camera with initial buffers because the
         * CameraService expects the preview to be enabled while
         * setting a valid preview window */
        ALOGV("%s: Starting preview with initial buffers ", __FUNCTION__);
        result = startInitialPreview();
    } else {
        /* startPreview has been issued after a valid preview window
         * is set. Get the preview buffers from gralloc and start
         * preview normally */
        ALOGV("%s: Starting normal preview ", __FUNCTION__);
        result = getBuffersAndStartPreview();
    }
    ALOGV("startPreview X");
    return result;
}

void QualcommCameraHardware::stopInitialPreview()
{
    mCameraRunning = 0;
}

void QualcommCameraHardware::stopPreviewInternal()
{
    ALOGI("stopPreviewInternal E mCameraRunning: %d", mCameraRunning);
    if (mCameraRunning && mPreviewWindow!=NULL) {
        // Cancel auto focus.
        {
            if (mNotifyCallback && (mMsgEnabled & CAMERA_MSG_FOCUS)) {
                cancelAutoFocusInternal();
            }
        }

        Mutex::Autolock l(&mCamframeTimeoutLock);
        {
            Mutex::Autolock cameraRunningLock(&mCameraRunningLock);
            if(!camframe_timeout_flag) {
                if (( mCurrentTarget != TARGET_MSM7630 ) &&
                         (mCurrentTarget != TARGET_QSD8250) && (mCurrentTarget != TARGET_MSM8660))
                    mCameraRunning = !native_stop_ops(CAMERA_OPS_STREAMING_PREVIEW, NULL);
                else
                    mCameraRunning = !native_stop_ops(CAMERA_OPS_STREAMING_VIDEO, NULL);
            } else {
                /* This means that the camframetimeout was issued.
                 * But we did not issue native_stop_preview(), so we
                 * need to update mCameraRunning to indicate that
                 * Camera is no longer running. */
                mCameraRunning = 0;
            }
        }
    }

    ALOGV("%s: mCameraRunning = %d", __FUNCTION__, mCameraRunning);
    if (!mCameraRunning) {
        ALOGV("%s: before calling deinitpre mPreviewInitialized = %d", __FUNCTION__, mPreviewInitialized);
        if (mPreviewInitialized) {
            ALOGV("before calling deinitpreview");
            deinitPreview();
            if ((mCurrentTarget == TARGET_MSM7630 ) ||
                (mCurrentTarget == TARGET_QSD8250) ||
                (mCurrentTarget == TARGET_MSM8660)) {
                mVideoThreadWaitLock.lock();
                ALOGV("in stopPreviewInternal: making mVideoThreadExit 1");
                mVideoThreadExit = 1;
                mVideoThreadWaitLock.unlock();
                //  720p : signal the video thread , and check in video thread if stop is called, if so exit video thread.
                pthread_mutex_lock(&(g_busy_frame_queue.mut));
                pthread_cond_signal(&(g_busy_frame_queue.wait));
                pthread_mutex_unlock(&(g_busy_frame_queue.mut));

                ALOGI(" flush video and release all frames");
                /* Flush the Busy Q */
                cam_frame_flush_video();
                /* Flush the Free Q */
                LINK_cam_frame_flush_free_video();
            }
            mPreviewInitialized = false;
        }
    }
    else ALOGE("stopPreviewInternal: failed to stop preview");

    ALOGI("stopPreviewInternal X: %d", mCameraRunning);
}

void QualcommCameraHardware::stopPreview()
{
    ALOGV("stopPreview: E");
    Mutex::Autolock l(&mLock);

    if (mDataCallbackTimestamp && (mMsgEnabled & CAMERA_MSG_VIDEO_FRAME))
        return;

    // wait if snapshot is in progress
    mSnapshotThreadWaitLock.lock();
    while (mSnapshotThreadRunning) {
        ALOGI("stoppreview: waiting for old snapshot thread to complete.");
        mSnapshotThreadWait.wait(mSnapshotThreadWaitLock);
        ALOGI("stoppreview: old snapshot thread completed. proceed stoppreview");
    }
    mSnapshotThreadWaitLock.unlock();

    if (mPreviewWindow != NULL) {
        private_handle_t *handle;
        if (mPreviewWindow != NULL && mThumbnailBuffer != NULL) {
            handle = (private_handle_t *)(*mThumbnailBuffer);
            ALOGI("%s:  Cancelling postview buffer %d ", __FUNCTION__, handle->fd);
            mDisplayLock.lock();
            if (BUFFER_LOCKED == mThumbnailLockState) {
                if (GENLOCK_FAILURE == genlock_unlock_buffer(handle)) {
                    ALOGE("%s: genlock_unlock_buffer failed", __FUNCTION__);
                } else {
                    mThumbnailLockState = BUFFER_UNLOCKED;
                }
            }
            status_t retVal = mPreviewWindow->cancel_buffer(mPreviewWindow,
                                mThumbnailBuffer);
            if (retVal != NO_ERROR)
                ALOGE("%s: cancelBuffer failed for postview buffer %d",
                    __FUNCTION__, handle->fd);
            mDisplayLock.unlock();
            mThumbnailBuffer = NULL;
            mThumbnailMapped = 0;
        }
    }
    stopPreviewInternal();
    ALOGV("stopPreview: X");
}

void QualcommCameraHardware::runAutoFocus()
{
    bool status = true;
    void *libhandle = NULL;
    isp3a_af_mode_t afMode = AF_MODE_AUTO;

    ALOGV("%s E", __FUNCTION__);
    mAutoFocusThreadLock.lock();
    // Skip autofocus if focus mode is infinity.

    const char * focusMode = mParameters.get(QCameraParameters::KEY_FOCUS_MODE);
    if ((mParameters.get(QCameraParameters::KEY_FOCUS_MODE) == 0)
           || (strcmp(focusMode, QCameraParameters::FOCUS_MODE_INFINITY) == 0)
           || (strcmp(focusMode, QCameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO) == 0)) {
        goto done;
    }

    ALOGV("%s, libmmcamera: %p\n", __FUNCTION__, libmmcamera);
    if (!libmmcamera){
        ALOGE("FATAL ERROR: could not dlopen liboemcamera.so: %s", dlerror());
        mAutoFocusThreadRunning = false;
        mAutoFocusThreadLock.unlock();
        return;
    }

    afMode = (isp3a_af_mode_t)attr_lookup(focus_modes,
                                sizeof(focus_modes) / sizeof(str_map),
                                mParameters.get(QCameraParameters::KEY_FOCUS_MODE));

    /* This will block until either AF completes or is cancelled. */
    ALOGV("af start (mode %d)", afMode);
    status_t err;
    err = mAfLock.tryLock();
    if(err == NO_ERROR) {
        {
            Mutex::Autolock cameraRunningLock(&mCameraRunningLock);
            if (mCameraRunning){
                ALOGV("Start AF");
                status =  native_start_ops(CAMERA_OPS_FOCUS ,(void *)&afMode);
            } else{
                ALOGV("As Camera preview is not running, AF not issued");
                status = false;
            }
        }
        mAfLock.unlock();
    }
    else {
        //AF Cancel would have acquired the lock,
        //so, no need to perform any AF
        ALOGV("As Cancel auto focus is in progress, auto focus request "
                "is ignored");
        status = FALSE;
    }

    ALOGV("af done: %d", (int)status);

done:
    mAutoFocusThreadRunning = false;
    mAutoFocusThreadLock.unlock();

    mCallbackLock.lock();
    bool autoFocusEnabled = mNotifyCallback && (mMsgEnabled & CAMERA_MSG_FOCUS);
    camera_notify_callback cb = mNotifyCallback;
    void *data = mCallbackCookie;
    mCallbackLock.unlock();
    if (autoFocusEnabled)
        cb(CAMERA_MSG_FOCUS, status, 0, data);
}

status_t QualcommCameraHardware::cancelAutoFocusInternal()
{
    ALOGV("cancelAutoFocusInternal E");

    if (!mHasAutoFocusSupport){
        ALOGV("cancelAutoFocusInternal X");
        return NO_ERROR;
    }

    status_t rc = NO_ERROR;
    status_t err;
    err = mAfLock.tryLock();
    if (err == NO_ERROR) {
        //Got Lock, means either AF hasn't started or
        // AF is done. So no need to cancel it, just change the state
        ALOGV("As Auto Focus is not in progress, Cancel Auto Focus "
                "is ignored");
        mAfLock.unlock();
    } else {
        //AF is in Progess, So cancel it
        ALOGV("Lock busy...cancel AF");
        rc = native_stop_ops(CAMERA_OPS_FOCUS, NULL) ?
                NO_ERROR :
                UNKNOWN_ERROR;
    }

    ALOGV("cancelAutoFocusInternal X: %d", rc);
    return rc;
}

void *auto_focus_thread(void *user)
{
    ALOGV("auto_focus_thread E");
    QualcommCameraHardware *obj = QualcommCameraHardware::getInstance();
    if (obj != 0) {
        obj->runAutoFocus();
    }
    else ALOGW("not starting autofocus: the object went away!");
    ALOGV("auto_focus_thread X");
    return NULL;
}

status_t QualcommCameraHardware::autoFocus()
{
    ALOGV("autoFocus E");
    Mutex::Autolock l(&mLock);

    if(!mHasAutoFocusSupport){
       bool status = false;
        mCallbackLock.lock();
        bool autoFocusEnabled = mNotifyCallback && (mMsgEnabled & CAMERA_MSG_FOCUS);
        camera_notify_callback cb = mNotifyCallback;
        void *data = mCallbackCookie;
        mCallbackLock.unlock();
        if (autoFocusEnabled)
            cb(CAMERA_MSG_FOCUS, status, 0, data);
        ALOGV("autoFocus X");
        return NO_ERROR;
    }

    {
        mAutoFocusThreadLock.lock();
        if (!mAutoFocusThreadRunning) {
            if (native_start_ops(CAMERA_OPS_PREPARE_SNAPSHOT, NULL) == FALSE) {
                ALOGE("Prepare_snapshot: CAMERA_OPS_PREPARE_SNAPSHOT ioctl failed!\n");
                mAutoFocusThreadLock.unlock();
                return UNKNOWN_ERROR;
            } else {
                mSnapshotPrepare = TRUE;
            }

            // Create a detached thread here so that we don't have to wait
            // for it when we cancel AF.
            pthread_t thr;
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            mAutoFocusThreadRunning =
                !pthread_create(&thr, &attr,
                                auto_focus_thread, NULL);
            if (!mAutoFocusThreadRunning) {
                ALOGE("failed to start autofocus thread");
                mAutoFocusThreadLock.unlock();
                return UNKNOWN_ERROR;
            }
        }
        mAutoFocusThreadLock.unlock();
    }

    ALOGV("autoFocus X");
    return NO_ERROR;
}

status_t QualcommCameraHardware::cancelAutoFocus()
{
    ALOGV("cancelAutoFocus E");
    Mutex::Autolock l(&mLock);

    int rc = NO_ERROR;
    if (mCameraRunning && mNotifyCallback && (mMsgEnabled & CAMERA_MSG_FOCUS)) {
        rc = cancelAutoFocusInternal();
    }

    ALOGV("cancelAutoFocus X");
    return rc;
}

void QualcommCameraHardware::runSnapshotThread(void *data)
{
    bool ret = true;
    CAMERA_HAL_UNUSED(data);
    ALOGV("runSnapshotThread E");

    if(!libmmcamera){
        ALOGE("FATAL ERROR: could not dlopen liboemcamera.so: %s", dlerror());
    }

    ALOGV("%s, libmmcamera: %p\n", __FUNCTION__, libmmcamera);

    mSnapshotCancelLock.lock();
    if (mSnapshotCancel == true) {
        mSnapshotCancel = false;
        mSnapshotCancelLock.unlock();
        ALOGV("%s: cancelpicture has been called..so abort taking snapshot", __FUNCTION__);
        deinitRaw();
        mInSnapshotModeWaitLock.lock();
        mInSnapshotMode = false;
        mInSnapshotModeWait.signal();
        mInSnapshotModeWaitLock.unlock();
        mSnapshotThreadWaitLock.lock();
        mSnapshotThreadRunning = false;
        mSnapshotThreadWait.signal();
        mSnapshotThreadWaitLock.unlock();
        return;
    }
    mSnapshotCancelLock.unlock();

    if(mSnapshotFormat == PICTURE_FORMAT_JPEG){
        if (native_start_ops(CAMERA_OPS_SNAPSHOT, NULL))
            ret = receiveRawPicture();
        else {
            ALOGE("main: snapshot failed! [CAMERA_OPS_SNAPSHOT]");
            ret = false;
        }
    } else if(mSnapshotFormat == PICTURE_FORMAT_RAW){
        if (native_start_ops(CAMERA_OPS_RAW_SNAPSHOT, NULL)) {
            ret = receiveRawSnapshot();
        } else {
            ALOGE("main: raw_snapshot failed! [CAMERA_OPS_RAW_SNAPSHOT]");
            ret = false;
        }
    }
    mInSnapshotModeWaitLock.lock();
    mInSnapshotMode = false;
    mInSnapshotModeWait.signal();
    mInSnapshotModeWaitLock.unlock();

    mSnapshotFormat = 0;
    if (ret != false) {
        if (strTexturesOn != true ) {
            mJpegThreadWaitLock.lock();
            while (mJpegThreadRunning) {
                ALOGI("runSnapshotThread: waiting for jpeg thread to complete.");
                mJpegThreadWait.wait(mJpegThreadWaitLock);
                ALOGI("runSnapshotThread: jpeg thread completed.");
            }
            mJpegThreadWaitLock.unlock();
            //clear the resources
            ALOGV("%s, libmmcamera: %p\n", __FUNCTION__, libmmcamera);
            if (libmmcamera != NULL) {
                LINK_jpeg_encoder_join();
            }
        }
    } else {
        if (mDataCallback
            && (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE)) {
            /* get picture failed. Give jpeg callback with NULL data
             * to the application to restore to preview mode
             */
            ALOGE("get picture failed, giving jpeg callback with NULL data");
            mDataCallback(CAMERA_MSG_COMPRESSED_IMAGE, NULL, data_counter, NULL, mCallbackCookie);
        }
    }
    deinitRaw();

    mSnapshotThreadWaitLock.lock();
    mSnapshotThreadRunning = false;
    mSnapshotThreadWait.signal();
    mSnapshotThreadWaitLock.unlock();

    ALOGV("runSnapshotThread X");
}

void *snapshot_thread(void *user)
{
    ALOGD("snapshot_thread E");
    QualcommCameraHardware *obj = QualcommCameraHardware::getInstance();
    if (obj != 0) {
        obj->runSnapshotThread(user);
    }
    else ALOGW("not starting snapshot thread: the object went away!");
    ALOGD("snapshot_thread X");
    return NULL;
}

status_t QualcommCameraHardware::takePicture()
{
    ALOGV("takePicture(%d)", mMsgEnabled);
    Mutex::Autolock l(&mLock);

    if (strTexturesOn == true) {
        mEncodePendingWaitLock.lock();
        while(mEncodePending) {
            ALOGV("takePicture: Frame given to application, waiting for encode call");
            mEncodePendingWait.wait(mEncodePendingWaitLock);
            ALOGV("takePicture: Encode of the application data is done");
        }
        mEncodePendingWaitLock.unlock();
    }

    // Wait for old snapshot thread to complete.
    mSnapshotThreadWaitLock.lock();
    while (mSnapshotThreadRunning) {
        ALOGV("takePicture: waiting for old snapshot thread to complete.");
        mSnapshotThreadWait.wait(mSnapshotThreadWaitLock);
        ALOGV("takePicture: old snapshot thread completed.");
    }
    //mSnapshotFormat is protected by mSnapshotThreadWaitLock
    if (mParameters.getPictureFormat() != 0 &&
            !strcmp(mParameters.getPictureFormat(),
                    QCameraParameters::PIXEL_FORMAT_RAW))
        mSnapshotFormat = PICTURE_FORMAT_RAW;
    else
        mSnapshotFormat = PICTURE_FORMAT_JPEG;

    if (mSnapshotFormat == PICTURE_FORMAT_JPEG){
        if (!native_start_ops(CAMERA_OPS_PREPARE_SNAPSHOT, NULL)) {
            mSnapshotThreadWaitLock.unlock();
            ALOGE("PREPARE SNAPSHOT: CAMERA_OPS_PREPARE_SNAPSHOT ioctl Failed");
            return UNKNOWN_ERROR;
        }
    }

    if (mCurrentTarget == TARGET_MSM8660) {
       /* Store the last frame queued for preview. This
        * shall be used as postview */
        if (!(storePreviewFrameForPostview()))
        return UNKNOWN_ERROR;
    }
    stopPreviewInternal();

    if (mSnapshotFormat == PICTURE_FORMAT_JPEG){
        if (!initRaw(mDataCallback && (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE))) {
            ALOGE("initRaw failed.  Not taking picture.");
            mSnapshotThreadWaitLock.unlock();
            return UNKNOWN_ERROR;
        }
    } else if (mSnapshotFormat == PICTURE_FORMAT_RAW ){
        if (!initRawSnapshot()){
            ALOGE("initRawSnapshot failed. Not taking picture.");
            mSnapshotThreadWaitLock.unlock();
            return UNKNOWN_ERROR;
        }
    }

    mShutterLock.lock();
    mShutterPending = true;
    mShutterLock.unlock();

    mSnapshotCancelLock.lock();
    mSnapshotCancel = false;
    mSnapshotCancelLock.unlock();

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    mSnapshotThreadRunning = !pthread_create(&mSnapshotThread,
                                             &attr,
                                             snapshot_thread,
                                             NULL);
    mSnapshotThreadWaitLock.unlock();

    mInSnapshotModeWaitLock.lock();
    mInSnapshotMode = true;
    mInSnapshotModeWaitLock.unlock();

    ALOGV("takePicture: X");
    return mSnapshotThreadRunning ? NO_ERROR : UNKNOWN_ERROR;
}

void QualcommCameraHardware::set_liveshot_exifinfo()
{
    setGpsParameters();
    //set TimeStamp
    const char *str = mParameters.get(QCameraParameters::KEY_EXIF_DATETIME);
    if(str != NULL) {
        strlcpy(dateTime, str, 20);
        addExifTag(EXIFTAGID_EXIF_DATE_TIME_ORIGINAL, EXIF_ASCII,
                   20, 1, (void *)dateTime);
    }
}

status_t QualcommCameraHardware::takeLiveSnapshot()
{
    ALOGV("takeLiveSnapshot: E ");
    Mutex::Autolock l(&mLock);

    if (liveshot_state == LIVESHOT_IN_PROGRESS || !recordingState) {
        return NO_ERROR;
    }

    if ((mCurrentTarget != TARGET_MSM7630) && (mCurrentTarget != TARGET_MSM8660)) {
        ALOGI("LiveSnapshot not supported on this target");
        liveshot_state = LIVESHOT_STOPPED;
        return NO_ERROR;
    }

    liveshot_state = LIVESHOT_IN_PROGRESS;

    if (!initLiveSnapshot(videoWidth, videoHeight)) {
        ALOGE("takeLiveSnapshot: Jpeg Heap Memory allocation failed.  Not taking Live Snapshot.");
        liveshot_state = LIVESHOT_STOPPED;
        return UNKNOWN_ERROR;
    }

    uint32_t maxjpegsize = videoWidth * videoHeight *1.5;
    set_liveshot_exifinfo();
    if (!LINK_set_liveshot_params(videoWidth, videoHeight,
                                exif_data, exif_table_numEntries,
                                (uint8_t *)mJpegMapped->data, maxjpegsize)) {
        ALOGE("Link_set_liveshot_params failed.");
        return NO_ERROR;
    }

    if (!native_start_ops(CAMERA_OPS_LIVESHOT, NULL)) {
        ALOGE("start_liveshot ioctl failed");
        liveshot_state = LIVESHOT_STOPPED;
        return UNKNOWN_ERROR;
    }

    ALOGV("takeLiveSnapshot: X");
    return NO_ERROR;
}

bool QualcommCameraHardware::initLiveSnapshot(int videowidth, int videoheight)
{
    ALOGV("initLiveSnapshot E");
    return true;
}


status_t QualcommCameraHardware::cancelPicture()
{
    status_t rc;
    ALOGV("cancelPicture: E");

    mSnapshotCancelLock.lock();
    mSnapshotCancel = true;
    mSnapshotCancelLock.unlock();

    if (mCurrentTarget == TARGET_MSM7627) {
        mSnapshotDone = TRUE;
        mSnapshotThreadWaitLock.lock();
        while (mSnapshotThreadRunning) {
            ALOGV("cancelPicture: waiting for snapshot thread to complete.");
            mSnapshotThreadWait.wait(mSnapshotThreadWaitLock);
            ALOGV("cancelPicture: snapshot thread completed.");
        }
        mSnapshotThreadWaitLock.unlock();
    }
    rc = native_stop_ops(CAMERA_OPS_SNAPSHOT, NULL) ? NO_ERROR : UNKNOWN_ERROR;
    mSnapshotDone = FALSE;
    ALOGV("cancelPicture: X: %d", rc);
    return rc;
}

status_t QualcommCameraHardware::setParameters(const QCameraParameters& params)
{
    ALOGV("setParameters: E params = %p", &params);

    Mutex::Autolock l(&mLock);
    status_t rc, final_rc = NO_ERROR;

    if (mSnapshotThreadRunning) {
        if ((rc = setPreviewSize(params)))  final_rc = rc;
        if ((rc = setRecordSize(params)))  final_rc = rc;
        if ((rc = setPictureSize(params)))  final_rc = rc;
        if ((rc = setJpegThumbnailSize(params))) final_rc = rc;
        if ((rc = setJpegQuality(params)))  final_rc = rc;
        return final_rc;
    }

    if ((rc = setPreviewSize(params))) final_rc = rc;
    if ((rc = setRecordSize(params)))  final_rc = rc;
    if ((rc = setPictureSize(params)))  final_rc = rc;
    if ((rc = setJpegThumbnailSize(params))) final_rc = rc;
    if ((rc = setJpegQuality(params)))  final_rc = rc;
    if ((rc = setPictureFormat(params))) final_rc = rc;
    if ((rc = setRecordSize(params)))  final_rc = rc;
    if ((rc = setPreviewFormat(params)))   final_rc = rc;
    if ((rc = setEffect(params)))       final_rc = rc;
    if ((rc = setGpsLocation(params)))  final_rc = rc;
    if ((rc = setRotation(params)))     final_rc = rc;
    if ((rc = setZoom(params)))         final_rc = rc;
    if ((rc = setOrientation(params)))  final_rc = rc;
    if ((rc = setLensshadeValue(params)))  final_rc = rc;
    if ((rc = setPictureFormat(params))) final_rc = rc;
    if ((rc = setSharpness(params)))    final_rc = rc;
    if ((rc = setSaturation(params)))   final_rc = rc;
    if ((rc = setContinuousAf(params)))  final_rc = rc;
    if ((rc = setSelectableZoneAf(params)))   final_rc = rc;
    if ((rc = setTouchAfAec(params)))   final_rc = rc;
    if ((rc = setSceneMode(params)))    final_rc = rc;
    if ((rc = setContrast(params)))     final_rc = rc;
    if ((rc = setRecordSize(params)))  final_rc = rc;
    if ((rc = setSceneDetect(params)))  final_rc = rc;
    if ((rc = setStrTextures(params)))   final_rc = rc;
    if ((rc = setPreviewFormat(params)))   final_rc = rc;
    if ((rc = setSkinToneEnhancement(params)))   final_rc = rc;
    if ((rc = setAntibanding(params)))  final_rc = rc;
    if ((rc = setPreviewFpsRange(params)))  final_rc = rc;

    const char *str = params.get(QCameraParameters::KEY_SCENE_MODE);
    int32_t value = attr_lookup(scenemode, sizeof(scenemode) / sizeof(str_map), str);

    if ((value != NOT_FOUND) && (value == CAMERA_BESTSHOT_OFF)) {
        if ((rc = setPreviewFrameRate(params))) final_rc = rc;
        if ((rc = setPreviewFrameRateMode(params))) final_rc = rc;
        if ((rc = setAutoExposure(params))) final_rc = rc;
        if ((rc = setExposureCompensation(params))) final_rc = rc;
        if ((rc = setWhiteBalance(params))) final_rc = rc;
        if ((rc = setFlash(params)))        final_rc = rc;
        if ((rc = setFocusMode(params)))    final_rc = rc;
        if ((rc = setBrightness(params)))   final_rc = rc;
        if ((rc = setISOValue(params)))  final_rc = rc;
    }
    //selectableZoneAF needs to be invoked after continuous AF
    if ((rc = setSelectableZoneAf(params)))   final_rc = rc;

    ALOGV("setParameters: X");
    return final_rc;
}

QCameraParameters QualcommCameraHardware::getParameters() const
{
    ALOGV("getParameters: EX");
    return mParameters;
}

status_t QualcommCameraHardware::setHistogramOn()
{
    ALOGV("setHistogramOn: EX");

    mStatsWaitLock.lock();
    mSendData = true;
    if (mStatsOn == CAMERA_HISTOGRAM_ENABLE) {
        mStatsWaitLock.unlock();
        return NO_ERROR;
    }

    if (mStatHeap != NULL) {
        ALOGV("setHistogram on: clearing old mStatHeap.");
        mStatHeap.clear();
    }

    mStatSize = sizeof(uint32_t)* HISTOGRAM_STATS_SIZE;
    mCurrent = -1;
    /*Currently the Ashmem is multiplying the buffer size with total number
    of buffers and page aligning. This causes a crash in JNI as each buffer
    individually expected to be page aligned  */
    int page_size_minus_1 = getpagesize() - 1;
    int32_t mAlignedStatSize = ((mStatSize + page_size_minus_1) & (~page_size_minus_1));

    mStatHeap =
            new AshmemPool(mAlignedStatSize,
                           3,
                           mStatSize,
                           "stat");
    if (!mStatHeap->initialized()) {
        ALOGE("Stat Heap X failed ");
        mStatHeap.clear();
        mStatHeap = NULL;
        ALOGE("setHistogramOn X: error initializing mStatHeap");
        mStatsWaitLock.unlock();
        return UNKNOWN_ERROR;
    }
    mStatsOn = CAMERA_HISTOGRAM_ENABLE;

    mStatsWaitLock.unlock();
    mCfgControl.mm_camera_set_parm(CAMERA_PARM_HISTOGRAM, &mStatsOn);
    return NO_ERROR;

}

status_t QualcommCameraHardware::setHistogramOff()
{
    ALOGV("setHistogramOff: EX");
    mStatsWaitLock.lock();
    if (mStatsOn == CAMERA_HISTOGRAM_DISABLE) {
        mStatsWaitLock.unlock();
        return NO_ERROR;
    }
    mStatsOn = CAMERA_HISTOGRAM_DISABLE;
    mStatsWaitLock.unlock();

    mCfgControl.mm_camera_set_parm(CAMERA_PARM_HISTOGRAM, &mStatsOn);

    mStatsWaitLock.lock();
    mStatHeap.clear();
    mStatHeap = NULL;
    mStatsWaitLock.unlock();

    return NO_ERROR;
}

status_t QualcommCameraHardware::runFaceDetection()
{
    bool ret = true;

    const char *str = mParameters.get(QCameraParameters::KEY_FACE_DETECTION);
    if (str != NULL) {
        int value = attr_lookup(facedetection,
                sizeof(facedetection) / sizeof(str_map), str);

        mMetaDataWaitLock.lock();
        if (value == true) {
            if(mMetaDataHeap != NULL)
                mMetaDataHeap.clear();

            mMetaDataHeap =
                new AshmemPool((sizeof(int)*(MAX_ROI*4+1)),
                        1,
                        (sizeof(int)*(MAX_ROI*4+1)),
                        "metadata");
            if (!mMetaDataHeap->initialized()) {
                ALOGE("Meta Data Heap allocation failed ");
                mMetaDataHeap.clear();
                mMetaDataHeap = NULL;
                ALOGE("runFaceDetection X: error initializing mMetaDataHeap");
                mMetaDataWaitLock.unlock();
                return UNKNOWN_ERROR;
            }
            mSendMetaData = true;
        } else {
            if (mMetaDataHeap != NULL) {
                mMetaDataHeap.clear();
                mMetaDataHeap = NULL;
            }
        }
        mMetaDataWaitLock.unlock();
        ret = native_set_parms(CAMERA_PARM_FD, sizeof(int8_t), (void *)&value);
        return ret ? NO_ERROR : UNKNOWN_ERROR;
    }
    ALOGE("Invalid Face Detection value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QualcommCameraHardware::sendCommand(int32_t command, int32_t arg1,
                                             int32_t arg2)
{
    ALOGV("sendCommand: EX");
    switch (command) {
        case CAMERA_CMD_SET_DISPLAY_ORIENTATION:
                                   ALOGV("Display orientation is not supported yet");
                                   return NO_ERROR;
        case CAMERA_CMD_START_FACE_DETECTION:
                                   if(supportsFaceDetection() == false){
                                        ALOGI("face detection support is not available");
                                        return NO_ERROR;
                                   }
                                   setFaceDetection("on");
                                   return runFaceDetection();
        case CAMERA_CMD_STOP_FACE_DETECTION:
                                   if(supportsFaceDetection() == false){
                                        ALOGI("face detection support is not available");
                                        return NO_ERROR;
                                   }
                                   setFaceDetection("off");
                                   return runFaceDetection();
        /*case CAMERA_CMD_HISTOGRAM_ON:
                                   ALOGV("histogram set to on");
                                   return setHistogramOn();
        case CAMERA_CMD_HISTOGRAM_OFF:
                                   ALOGV("histogram set to off");
                                   return setHistogramOff();
        case CAMERA_CMD_HISTOGRAM_SEND_DATA:
                                   mStatsWaitLock.lock();
                                   if(mStatsOn == CAMERA_HISTOGRAM_ENABLE)
                                       mSendData = true;
                                   mStatsWaitLock.unlock();
                                   return NO_ERROR;*/
        case CAMERA_CMD_START_SMOOTH_ZOOM:
        case CAMERA_CMD_STOP_SMOOTH_ZOOM:
                                   ALOGV("Smooth zoom is not supported yet");
                                   return BAD_VALUE;
        default:
                                   ALOGE("The command %i is not supported yet", command);
    }
    return BAD_VALUE;
}

// If the hardware already exists, return a strong pointer to the current
// object. If not, create a new hardware object, put it in the singleton,
// and return it.
QualcommCameraHardware* QualcommCameraHardware::createInstance()
{
    ALOGI("createInstance: E");
    struct stat st;
    int rc = stat("/dev/oncrpc", &st);
    if (rc < 0) {
        ALOGD("createInstance: X failed to create hardware: %s", strerror(errno));
        return NULL;
    }

    QualcommCameraHardware *cam = new QualcommCameraHardware();
    current_cam = cam;

    ALOGI("createInstance: created hardware=%p", &(*current_cam));
    if (!cam->startCamera()) {
        ALOGE("%s: startCamera failed!", __FUNCTION__);
        delete cam;
        return NULL;
    }

    cam->initDefaultParameters();
    ALOGI("createInstance: X");
    return cam;
}

// For internal use only, hence the strong pointer to the derived type.
QualcommCameraHardware* QualcommCameraHardware::getInstance()
{
    ALOGV("%s E", __FUNCTION__);
    if (current_cam != 0) {
        return current_cam;
    } else {
        ALOGV("getInstance: X new instance of hardware");
        return new QualcommCameraHardware();
    }
}
void QualcommCameraHardware::receiveRecordingFrame(struct msm_frame *frame)
{
    ALOGV("receiveRecordingFrame E");
    // post busy frame
    if (frame) {
        cam_frame_post_video(frame);
    }
    else ALOGE("in receiveRecordingFrame frame is NULL");
    ALOGV("receiveRecordingFrame X");
}


bool QualcommCameraHardware::native_zoom_image(int fd, int srcOffset, int dstOffSet, common_crop_t *crop)
{
    int result = 0;
    struct mdp_blit_req *e;
    struct timeval td1, td2;

    /* Initialize yuv structure */
    zoomImage.list.count = 1;

    e = &zoomImage.list.req[0];

    e->src.width = previewWidth;
    e->src.height = previewHeight;
    e->src.format = MDP_Y_CBCR_H2V2;
    e->src.offset = srcOffset;
    e->src.memory_id = fd;

    e->dst.width = previewWidth;
    e->dst.height = previewHeight;
    e->dst.format = MDP_Y_CBCR_H2V2;
    e->dst.offset = dstOffSet;
    e->dst.memory_id = fd;

    e->transp_mask = 0xffffffff;
    e->flags = 0;
    e->alpha = 0xff;
    if (crop->in1_w != 0 || crop->in1_h != 0) {
        e->src_rect.x = (crop->out1_w - crop->in1_w + 1) / 2 - 1;
        e->src_rect.y = (crop->out1_h - crop->in1_h + 1) / 2 - 1;
        e->src_rect.w = crop->in1_w;
        e->src_rect.h = crop->in1_h;
    } else {
        e->src_rect.x = 0;
        e->src_rect.y = 0;
        e->src_rect.w = previewWidth;
        e->src_rect.h = previewHeight;
    }
    //ALOGV(" native_zoom : SRC_RECT : x,y = %d,%d \t w,h = %d, %d",
    //        e->src_rect.x, e->src_rect.y, e->src_rect.w, e->src_rect.h);

    e->dst_rect.x = 0;
    e->dst_rect.y = 0;
    e->dst_rect.w = previewWidth;
    e->dst_rect.h = previewHeight;

    result = ioctl(fb_fd, MSMFB_BLIT, &zoomImage.list);
    if (result < 0) {
        ALOGE("MSMFB_BLIT failed! line=%d\n", __LINE__);
        return FALSE;
    }
    return TRUE;
}

void QualcommCameraHardware::debugShowPreviewFPS() const
{
    static int mFrameCount;
    static int mLastFrameCount = 0;
    static nsecs_t mLastFpsTime = 0;
    static float mFps = 0;
    mFrameCount++;
    nsecs_t now = systemTime();
    nsecs_t diff = now - mLastFpsTime;
    if (diff > ms2ns(250)) {
        mFps = ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
        ALOGI("Preview Frames Per Second: %.4f", mFps);
        mLastFpsTime = now;
        mLastFrameCount = mFrameCount;
    }
}

void QualcommCameraHardware::debugShowVideoFPS() const
{
    static int mFrameCount;
    static int mLastFrameCount = 0;
    static nsecs_t mLastFpsTime = 0;
    static float mFps = 0;
    ALOGV("%s E", __FUNCTION__);
    mFrameCount++;
    nsecs_t now = systemTime();
    nsecs_t diff = now - mLastFpsTime;
    if (diff > ms2ns(250)) {
        mFps =  ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
        ALOGI("Video Frames Per Second: %.4f", mFps);
        mLastFpsTime = now;
        mLastFrameCount = mFrameCount;
    }
}

void QualcommCameraHardware::receiveLiveSnapshot(uint32_t jpeg_size)
{
    ALOGV("receiveLiveSnapshot E");

#ifdef DUMP_LIVESHOT_JPEG_FILE
    int file_fd = open("/data/LiveSnapshot.jpg", O_RDWR | O_CREAT, 0777);
    ALOGV("dumping live shot image in /data/LiveSnapshot.jpg");
    if (file_fd < 0) {
        ALOGE("cannot open file\n");
    } else {
        write(file_fd, (uint8_t *)mJpegMapped->data,jpeg_size);
    }
    close(file_fd);
#endif

    Mutex::Autolock cbLock(&mCallbackLock);
#ifdef MEDIA_RECORDER_MSG_COMPRESSED_IMAGE
    if (mDataCallback && (mMsgEnabled & MEDIA_RECORDER_MSG_COMPRESSED_IMAGE)) {
        mDataCallback(CAMERA_MSG_COMPRESSED_IMAGE, mJpegMapped, data_counter,
                          NULL, mCallbackCookie);
    }
    else ALOGV("JPEG callback was cancelled--not delivering image.");
#endif
    //Reset the Gps Information & relieve memory
    exif_table_numEntries = 0;

    liveshot_state = LIVESHOT_DONE;

    ALOGV("receiveLiveSnapshot X");
}

void QualcommCameraHardware::receivePreviewFrame(struct msm_frame *frame)
{
    status_t retVal = NO_ERROR;
    buffer_handle_t *handle = NULL;
    int bufferIndex = 0;

    ALOGV("receivePreviewFrame E");
    if (!frame) {
        return;
    }
    if (!mCameraRunning) {
        ALOGE("ignoring preview callback--camera has been stopped");
        LINK_camframe_free_video(frame);
        return;
    }
    if (UNLIKELY(mDebugFps)) {
        debugShowPreviewFPS();
    }
    mCallbackLock.lock();
    int msgEnabled = mMsgEnabled;
    camera_data_callback pcb = mDataCallback;
    void *pdata = mCallbackCookie;
    camera_data_timestamp_callback rcb = mDataCallbackTimestamp;
    void *rdata = mCallbackCookie;
    camera_data_callback mcb = mDataCallback;
    void *mdata = mCallbackCookie;
    mCallbackLock.unlock();

    int i=0;
    int *data=(int*)frame;

    // Find the offset within the heap of the current buffer.
    ssize_t offset_addr = 0; // TODO , use proper value
    common_crop_t *crop = (common_crop_t *) (frame->cropinfo);

#ifdef DUMP_PREVIEW_FRAMES
    static int frameCnt = 0;
    int written;
    if (frameCnt >= 0 && frameCnt <= 10) {
        char buf[128];
        sprintf(buf, "/data/%d_preview.yuv", frameCnt);
        int file_fd = open(buf, O_RDWR | O_CREAT, 0777);
        ALOGV("dumping preview frame %d", frameCnt);
        if (file_fd < 0) {
            ALOGE("cannot open file\n");
        } else {
            ALOGV("dumping data");
            written = write(file_fd, (uint8_t *)frame->buffer,
                mPreviewFrameSize );
            if (written < 0)
                ALOGE("error in data write");
        }
        close(file_fd);
    }
    frameCnt++;
#endif
    mInPreviewCallback = true;
    if (crop->in1_w != 0 && crop->in1_h != 0) {
        zoomCropInfo.left = (crop->out1_w - crop->in1_w + 1) / 2 - 1;
        zoomCropInfo.top = (crop->out1_h - crop->in1_h + 1) / 2 - 1;
        /* There can be scenarios where the in1_wXin1_h and
         * out1_wXout1_h are same. In those cases, reset the
         * x and y to zero instead of negative for proper zooming
         */
        if (zoomCropInfo.left < 0) zoomCropInfo.left = 0;
        if (zoomCropInfo.top < 0) zoomCropInfo.top = 0;
        zoomCropInfo.right = zoomCropInfo.left + crop->in1_w;
        zoomCropInfo.bottom = zoomCropInfo.top + crop->in1_h;
        mPreviewWindow->set_crop(mPreviewWindow,
                                  zoomCropInfo.left,
                                  zoomCropInfo.top,
                                  zoomCropInfo.right,
                                  zoomCropInfo.bottom);
        /* Set mResetOverlayCrop to true, so that when there is
         * no crop information, setCrop will be called
         * with zero crop values.
         */
        mResetWindowCrop = true;
    } else {
        // Reset zoomCropInfo variables. This will ensure that
        // stale values wont be used for postview
        zoomCropInfo.left = 0;
        zoomCropInfo.top = 0;
        zoomCropInfo.right = crop->in1_w;
        zoomCropInfo.bottom = crop->in1_h;
        /* This reset is required, if not, overlay driver continues
         * to use the old crop information for these preview
         * frames which is not the correct behavior. To avoid
         * multiple calls, reset once.
         */
        if (mResetWindowCrop == true) {
            mPreviewWindow->set_crop(mPreviewWindow,
                                  zoomCropInfo.left,
                                  zoomCropInfo.top,
                                  zoomCropInfo.right,
                                  zoomCropInfo.bottom);
            mResetWindowCrop = false;
        }
    }
    /* To overcome a timing case where we could be having the overlay refer to deallocated
        mDisplayHeap(and showing corruption), the mDisplayHeap is not deallocated untill the
        first preview frame is queued to the overlay in 8660. Also adding the condition
        to check if snapshot is currently in progress ensures that the resources being
        used by the snapshot thread are not incorrectly deallocated by preview thread*/
    if ((mCurrentTarget == TARGET_MSM8660)&&(mFirstFrame == true)) {
        ALOGD(" receivePreviewFrame : first frame queued, display heap being deallocated");
        mFirstFrame = false;
    }
    mLastQueuedFrame = (void *)frame->buffer;
    bufferIndex = mapBuffer(frame);
    if (bufferIndex >= 0) {
        //Need to encapsulate this in IMemory object and send
        if (pcb != NULL && (msgEnabled & CAMERA_MSG_PREVIEW_FRAME)) {
            int previewBufSize;
            /* for CTS : Forcing preview memory buffer lenth to be
                      'previewWidth * previewHeight * 3/2'. Needed when gralloc allocated extra memory.*/
            if (mPreviewFormat == CAMERA_YUV_420_NV21) {
                previewBufSize = previewWidth * previewHeight * 3/2;
                camera_memory_t *previewMem = mGetMemory(frames[bufferIndex].fd, previewBufSize,
                                                    1, mCallbackCookie);
                if (!previewMem || !previewMem->data) {
                    ALOGE("%s: mGetMemory failed.\n", __func__);
                } else {
                    pcb(CAMERA_MSG_PREVIEW_FRAME,previewMem,0,NULL,pdata);
                    previewMem->release(previewMem);
                }
            } else
                 pcb(CAMERA_MSG_PREVIEW_FRAME,(camera_memory_t *) mPreviewMapped[bufferIndex]->data,0,NULL,pdata);
        }

        mDisplayLock.lock();
        if (mPreviewWindow != NULL) {
            if (BUFFER_LOCKED == frame_buffer[bufferIndex].lockState) {
                if (GENLOCK_FAILURE == genlock_unlock_buffer(
                       (native_handle_t*)(*(frame_buffer[bufferIndex].buffer)))) {
                   ALOGE("%s: genlock_unlock_buffer failed", __FUNCTION__);
                   mDisplayLock.unlock();
                   return;
                } else {
                   frame_buffer[bufferIndex].lockState = BUFFER_UNLOCKED;
                }
            } else {
                ALOGE("%s: buffer to be enqueued is unlocked", __FUNCTION__);
                mDisplayLock.unlock();
                return;
            }
            retVal = mPreviewWindow->enqueue_buffer(mPreviewWindow,
                                            frame_buffer[bufferIndex].buffer);
            ALOGV("enQ preview buffers %d", frame_buffer[bufferIndex].frame->fd);
            if (retVal != NO_ERROR)
                ALOGE("%s: Failed while queueing buffer %d for display."
                    " Error = %d", __FUNCTION__,
                    frame_buffer[bufferIndex].frame->fd, retVal);
            int stride;
            retVal = mPreviewWindow->dequeue_buffer(mPreviewWindow,
                                     &(handle),
                                     &(stride));
            if (retVal != NO_ERROR) {
                ALOGE("%s: Failed while dequeueing buffer from display."
                        " Error = %d", __FUNCTION__, retVal);
            } else {
                    private_handle_t *bhandle = (private_handle_t *)(*handle);
                    ALOGV("deQ preview buffers %d ", bhandle->fd);
                    retVal = mPreviewWindow->lock_buffer(mPreviewWindow,handle);
                    //yyan todo use handle to find out buffer
                    if (retVal != NO_ERROR)
                        ALOGE("%s: Failed while dequeueing buffer from"
                            "display. Error = %d", __FUNCTION__, retVal);
            }
        }
        mDisplayLock.unlock();
    } else
        ALOGE("Could not find the buffer");

    // If output  is NOT enabled (targets otherthan 7x30 , 8x50 and 8x60 currently..)
    nsecs_t timeStamp = nsecs_t(frame->ts.tv_sec)*1000000000LL + frame->ts.tv_nsec;

    //FIXME: the video thread stops receiving frames after 5th so until it gets solved using preview frames for recording
//    if ((mCurrentTarget != TARGET_MSM7630 ) &&  (mCurrentTarget != TARGET_QSD8250) && (mCurrentTarget != TARGET_MSM8660)) {
        if (rcb != NULL && (msgEnabled & CAMERA_MSG_VIDEO_FRAME)) {
            rcb(timeStamp, CAMERA_MSG_VIDEO_FRAME, mPreviewMapped[bufferIndex], 0, rdata);
            Mutex::Autolock rLock(&mRecordFrameLock);
            if (mReleasedRecordingFrame != true) {
                ALOGV("block waiting for frame release");
                mRecordWait.wait(mRecordFrameLock);
                ALOGV("frame released, continuing");
            }
            mReleasedRecordingFrame = false;
        }
//    }
#if 0
    if (mCurrentTarget == TARGET_MSM8660) {
        mMetaDataWaitLock.lock();
        if (mFaceDetectOn == true && mSendMetaData == true) {
            mSendMetaData = false;
            fd_roi_t *fd = (fd_roi_t *)(frame->roi_info.info);
            int faces_detected = fd->rect_num;
            int max_faces_detected = MAX_ROI * 4;
            int array[max_faces_detected + 1];

            array[0] = faces_detected * 4;
            for (int i = 1, j = 0;j < MAX_ROI; j++, i = i + 4) {
                if (j < faces_detected) {
                    array[i]   = fd->faces[j].x;
                    array[i+1] = fd->faces[j].y;
                    array[i+2] = fd->faces[j].dx;
                    array[i+3] = fd->faces[j].dx;
                } else {
                    array[i]   = -1;
                    array[i+1] = -1;
                    array[i+2] = -1;
                    array[i+3] = -1;
                }
            }
            memcpy((uint32_t *)mMetaDataHeap->mHeap->base(), (uint32_t *)array, (sizeof(int)*(MAX_ROI*4+1)));
            if  (mcb != NULL && (msgEnabled & CAMERA_MSG_PREVIEW_METADATA)) {
                mcb(CAMERA_MSG_PREVIEW_METADATA, mMetaDataHeap->mBuffers[0], mdata);
            }
        }
        mMetaDataWaitLock.unlock();
    }
#endif
    bufferIndex = mapFrame(handle);
    if (bufferIndex >= 0) {
        LINK_camframe_free_video(&frames[bufferIndex]);
        private_handle_t *bhandle = (private_handle_t *)(*handle);
        if (GENLOCK_NO_ERROR != genlock_lock_buffer(bhandle, GENLOCK_WRITE_LOCK, GENLOCK_MAX_TIMEOUT)) {
            ALOGE("%s: genlock_lock_buffer(WRITE) failed", __FUNCTION__);
            frame_buffer[bufferIndex].lockState = BUFFER_UNLOCKED;
        } else {
            frame_buffer[bufferIndex].lockState = BUFFER_LOCKED;
        }
    } else {
        ALOGE("Could not find the Frame");

        // Special Case: Stoppreview is issued which causes thumbnail buffer
        // to be cancelled. Frame thread has still not exited. In preview thread
        // dequeue returns incorrect buffer id (previously cancelled thumbnail buffer)
        // This will throw error "Could not find frame". We need to cancel the incorrectly
        // dequeued buffer here to ensure that all buffers are available for the next
        // startPreview call.

        mDisplayLock.lock();
        ALOGV("error Cancelling preview buffers");
        if (handle) {
            retVal = mPreviewWindow->cancel_buffer(mPreviewWindow,
                  handle);
            if (retVal != NO_ERROR)
                ALOGE("%s: cancelBuffer failed for buffer", __FUNCTION__);
        }
        mDisplayLock.unlock();
    }

    ALOGV("receivePreviewFrame X");
}

void QualcommCameraHardware::receiveCameraStats(camstats_type stype, camera_preview_histogram_info* histinfo)
{
  //  ALOGV("receiveCameraStats E");

    if (!mCameraRunning) {
        ALOGE("ignoring stats callback--camera has been stopped");
        return;
    }

    mCallbackLock.lock();
    int msgEnabled = mMsgEnabled;
    camera_data_callback scb = mDataCallback;
    void *sdata = mCallbackCookie;
    mCallbackLock.unlock();
    mStatsWaitLock.lock();
    if (mStatsOn == CAMERA_HISTOGRAM_DISABLE) {
        mStatsWaitLock.unlock();
        return;
    }
    if (!mSendData) {
        mStatsWaitLock.unlock();
    } else {
        mSendData = false;
        mCurrent = (mCurrent+1) % 3;
    // The first element of the array will contain the maximum hist value provided by driver.
        //*(uint32_t *)(mStatHeap->mHeap->base() + (mStatHeap->mBufferSize * mCurrent)) = histinfo->max_value;
        //memcpy((uint32_t *)((unsigned int)mStatHeap->mHeap->base()+ (mStatHeap->mBufferSize * mCurrent)+ sizeof(int32_t)), (uint32_t *)histinfo->buffer,(sizeof(int32_t) * 256));

        mStatsWaitLock.unlock();
/*
        if (scb != NULL && (msgEnabled & CAMERA_MSG_STATS_DATA))
            scb(CAMERA_MSG_STATS_DATA, mStatHeap->mBuffers[mCurrent],
                sdata);
*/
     }
  //  ALOGV("receiveCameraStats X");
}

bool QualcommCameraHardware::initRecord()
{
    const char *pmem_region = "/dev/pmem_adsp";
    int ion_heap = ION_CP_MM_HEAP_ID;
    int CbCrOffset;
    int recordBufferSize;
    int active, type =0;

    ALOGV("initRecord E");

    ALOGI("initRecord: mDimension.video_width = %d mDimension.video_height = %d",
            mDimension.video_width, mDimension.video_height);
    // for 8x60 the Encoder expects the CbCr offset should be aligned to 2K.
    if (mCurrentTarget == TARGET_MSM8660) {
        CbCrOffset = PAD_TO_2K(mDimension.video_width  * mDimension.video_height);
        recordBufferSize = CbCrOffset + PAD_TO_2K((mDimension.video_width * mDimension.video_height)/2);
    } else {
        CbCrOffset = PAD_TO_WORD(mDimension.video_width  * mDimension.video_height);
        recordBufferSize = (mDimension.video_width  * mDimension.video_height *3)/2;
    }

    /* Buffersize and frameSize will be different when DIS is ON.
     * We need to pass the actual framesize with video heap, as the same
     * is used at camera MIO when negotiating with encoder.
     */
    mRecordFrameSize = PAD_TO_4K(recordBufferSize);
    if (mVpeEnabled && mDisEnabled){
        mRecordFrameSize = videoWidth * videoHeight * 3 / 2;
        if (mCurrentTarget == TARGET_MSM8660){
            mRecordFrameSize = PAD_TO_4K(PAD_TO_2K(videoWidth * videoHeight)
                                + PAD_TO_2K((videoWidth * videoHeight)/2));
        }
    }
    ALOGV("mRecordFrameSize = %d", mRecordFrameSize);

    if (mRecordFrameSize <= 0) {
        ALOGE("initRecord X: wrong record frame size.");
        return false;
    }

    ALOGV("initRecord: Allocating Record buffers");
    for (int cnt = 0; cnt < kRecordBufferCount; cnt++) {
#ifdef USE_ION
        if (allocate_ion_memory(&record_main_ion_fd[cnt], &record_alloc[cnt], &record_ion_info_fd[cnt],
                            ion_heap, mRecordFrameSize, &mRecordfd[cnt]) < 0) {
              ALOGE("%s: ion allocation failed, heap=%d!\n", __func__, ion_heap);
              return NULL;
        }
#else
        mRecordfd[cnt] = open(pmem_region, O_RDWR|O_SYNC);
        if (mRecordfd[cnt] <= 0) {
            ALOGE("%s: Open device %s failed!\n", __func__, pmem_region);
            return NULL;
        }
#endif
        ALOGV("%s: Record fd is %d ", __func__, mRecordfd[cnt]);
        mRecordMapped[cnt]=mGetMemory(mRecordfd[cnt], mRecordFrameSize,1,mCallbackCookie);
        if ((void *)mRecordMapped[cnt]==NULL) {
            ALOGE("Failed to get camera memory for mRecordMapped heap");
        } else{
            ALOGV("Received following info for record mapped data:%p, handle:%p, size:%d, release:%p",
                mRecordMapped[cnt]->data ,mRecordMapped[cnt]->handle, mRecordMapped[cnt]->size, mRecordMapped[cnt]->release);
        }

        recordframes[cnt].buffer = (unsigned int)mRecordMapped[cnt]->data;
        recordframes[cnt].fd = mRecordfd[cnt];
        recordframes[cnt].y_off = 0;
        recordframes[cnt].cbcr_off = CbCrOffset;
        recordframes[cnt].path = OUTPUT_TYPE_V;
        record_buffers_tracking_flag[cnt] = false;

        ALOGV("initRecord: record heap, video buffers buffer=%lu fd=%d y_off=%d cbcr_off=%d \n",
            (unsigned long)recordframes[cnt].buffer, recordframes[cnt].fd, recordframes[cnt].y_off,
            recordframes[cnt].cbcr_off);

        active = (cnt < ACTIVE_VIDEO_BUFFERS);
        type = MSM_PMEM_VIDEO;
        if ((mVpeEnabled) && (cnt == kRecordBufferCount-1)) {
            type = MSM_PMEM_VIDEO_VPE;
            active = 1;
        }
        ALOGI("Registering video buffer %d", cnt);
        register_buf(mRecordFrameSize,
                            mRecordFrameSize, CbCrOffset, 0,
                            recordframes[cnt].fd,
                            0,
                            (uint8_t *)recordframes[cnt].buffer,
                            type,
                            active, true);
        ALOGV("Came back from register call to kernel");
    }

    // initial setup : buffers 1,2,3 with kernel , 4 with camframe , 5,6,7,8 in free Q
    // flush the busy Q
    cam_frame_flush_video();

    mVideoThreadWaitLock.lock();
    while (mVideoThreadRunning) {
        ALOGV("initRecord: waiting for old video thread to complete.");
        mVideoThreadWait.wait(mVideoThreadWaitLock);
        ALOGV("initRecord : old video thread completed.");
    }
    mVideoThreadWaitLock.unlock();

    // flush free queue and add 5,6,7,8 buffers.
    LINK_cam_frame_flush_free_video();
    if (mVpeEnabled) {
        //If VPE is enabled, the VPE buffer shouldn't be added to Free Q initally.
        for (int i=ACTIVE_VIDEO_BUFFERS+1;i <kRecordBufferCount-1; i++)
            LINK_camframe_free_video(&recordframes[i]);
    } else {
        for (int i=ACTIVE_VIDEO_BUFFERS+1;i <kRecordBufferCount; i++)
            LINK_camframe_free_video(&recordframes[i]);
    }
    ALOGV("initRecord X");

    return true;
}

status_t QualcommCameraHardware::setDIS() {
    ALOGV("setDIS E");
    video_dis_param_ctrl_t disCtrl;

    bool ret = true;
    ALOGV("mDisEnabled = %d", mDisEnabled);

    int video_frame_cbcroffset;
    video_frame_cbcroffset = PAD_TO_WORD(videoWidth * videoHeight);
    if (mCurrentTarget == TARGET_MSM8660)
        video_frame_cbcroffset = PAD_TO_2K(videoWidth * videoHeight);

    memset(&disCtrl, 0, sizeof(disCtrl));
    disCtrl.dis_enable = mDisEnabled;
    disCtrl.video_rec_width = videoWidth;
    disCtrl.video_rec_height = videoHeight;
    disCtrl.output_cbcr_offset = video_frame_cbcroffset;

    ret = native_set_parms( CAMERA_PARM_VIDEO_DIS,
                       sizeof(disCtrl), &disCtrl);
    ALOGV("setDIS X (%d)", ret);

    return ret ? NO_ERROR : UNKNOWN_ERROR;
}

status_t QualcommCameraHardware::setVpeParameters()
{
    ALOGV("setVpeParameters E");
    video_rotation_param_ctrl_t rotCtrl;

    bool ret = true;

    ALOGV("videoWidth = %d, videoHeight = %d", videoWidth, videoHeight);
    rotCtrl.rotation = (mRotation == 0) ? ROT_NONE :
                       ((mRotation == 90) ? ROT_CLOCKWISE_90 :
                  ((mRotation == 180) ? ROT_CLOCKWISE_180 : ROT_CLOCKWISE_270));

    if (((videoWidth == 1280 && videoHeight == 720) || (videoWidth == 800 && videoHeight == 480))
        && (mRotation == 90 || mRotation == 270) ){
        /* Due to a limitation at video core to support heights greater than 720, adding this check.
         * This is a temporary hack, need to be removed once video core support is available
         */
        ALOGI("video resolution (%dx%d) with rotation (%d) is not supported, setting rotation to NONE",
            videoWidth, videoHeight, mRotation);
        rotCtrl.rotation = ROT_NONE;
    }
    ALOGV("rotCtrl.rotation = %d", rotCtrl.rotation);

    ret = native_set_parms(CAMERA_PARM_VIDEO_ROT,
                           sizeof(rotCtrl), &rotCtrl);

    ALOGV("setVpeParameters X (%d)", ret);
    return ret ? NO_ERROR : UNKNOWN_ERROR;
}

status_t QualcommCameraHardware::startRecording()
{
    ALOGV("startRecording E");
    int ret;
    Mutex::Autolock l(&mLock);
    mReleasedRecordingFrame = false;
    if ((ret=startPreviewInternal())== NO_ERROR) {
        if (mVpeEnabled) {
            ALOGI("startRecording: VPE enabled, setting vpe parameters");
            bool status = setVpeParameters();
            if(status) {
                ALOGE("Failed to set VPE parameters");
                return status;
            }
        }
#if 0
        if( ( mCurrentTarget == TARGET_MSM7630 ) || (mCurrentTarget == TARGET_QSD8250) || (mCurrentTarget == TARGET_MSM8660))  {
            ALOGV(" in startREcording : calling start_recording");
            native_start_ops(CAMERA_OPS_VIDEO_RECORDING, NULL);
            recordingState = 1;
            // Remove the left out frames in busy Q and them in free Q.
            // this should be done before starting video_thread so that,
            // frames in previous recording are flushed out.
            ALOGV("frames in busy Q = %d", g_busy_frame_queue.num_of_frames);
            while ((g_busy_frame_queue.num_of_frames)>0) {
                msm_frame* vframe = cam_frame_get_video();
                LINK_camframe_free_video(vframe);
            }
            ALOGV("frames in busy Q = %d after deQueing", g_busy_frame_queue.num_of_frames);

            //Clear the dangling buffers and put them in free queue
            for (int cnt = 0; cnt < kRecordBufferCount; cnt++) {
                if (record_buffers_tracking_flag[cnt] == true) {
                    ALOGI("Dangling buffer: offset = %d, buffer = %d", cnt, (unsigned int)recordframes[cnt].buffer);
                    LINK_camframe_free_video(&recordframes[cnt]);
                    record_buffers_tracking_flag[cnt] = false;
                }
            }

            // Start video thread and wait for busy frames to be encoded, this thread
            // should be closed in stopRecording
            mVideoThreadWaitLock.lock();
            mVideoThreadExit = 0;
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            mVideoThreadRunning = pthread_create(&mVideoThread,
                                              &attr,
                                              video_thread,
                                              NULL);
            mVideoThreadWaitLock.unlock();
            // Remove the left out frames in busy Q and them in free Q.
        }
#endif
    }
    return ret;
}

void QualcommCameraHardware::stopRecording()
{
    ALOGV("stopRecording: E");
    Mutex::Autolock l(&mLock);
    {
        mRecordFrameLock.lock();
        mReleasedRecordingFrame = true;
        mRecordWait.signal();
        mRecordFrameLock.unlock();

        if (mDataCallback && !(mCurrentTarget == TARGET_QSD8250) &&
                         (mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME)) {
            ALOGV("stopRecording: X, preview still in progress");
            return;
        }
    }
    // If output2 enabled, exit video thread, invoke stop recording ioctl
    if ((mCurrentTarget == TARGET_MSM7630) || (mCurrentTarget == TARGET_QSD8250) || (mCurrentTarget == TARGET_MSM8660))  {
        mVideoThreadWaitLock.lock();
        mVideoThreadExit = 1;
        mVideoThreadWaitLock.unlock();
        native_stop_ops(CAMERA_OPS_VIDEO_RECORDING, NULL);

        pthread_mutex_lock(&(g_busy_frame_queue.mut));
        pthread_cond_signal(&(g_busy_frame_queue.wait));
        pthread_mutex_unlock(&(g_busy_frame_queue.mut));
    }
    recordingState = 0; // recording not started
    ALOGV("stopRecording: X");
}

void QualcommCameraHardware::releaseRecordingFrame(const void *opaque)
{
    ALOGV("%s : BEGIN, opaque = 0x%p",__func__, opaque);
    Mutex::Autolock rLock(&mRecordFrameLock);
    mReleasedRecordingFrame = true;
    mRecordWait.signal();

    // Ff 7x30 : add the frame to the free camframe queue
    if ((mCurrentTarget == TARGET_MSM7630) || (mCurrentTarget == TARGET_QSD8250) || (mCurrentTarget == TARGET_MSM8660)) {
        ssize_t offset;
        size_t size;
        msm_frame* releaseframe = NULL;
        int cnt;
        for (cnt = 0; cnt < kRecordBufferCount; cnt++) {
            if(recordframes[cnt].buffer && ((unsigned long)opaque == recordframes[cnt].buffer) ){
                ALOGV("in release recording frame found match, releasing buffer %d", (unsigned int)recordframes[cnt].buffer);
                releaseframe = &recordframes[cnt];
                break;
            }
        }
        if (cnt < kRecordBufferCount) {
            // do this only if frame thread is running
            mFrameThreadWaitLock.lock();
            if(mFrameThreadRunning ) {
                //Reset the track flag for this frame buffer
                record_buffers_tracking_flag[cnt] = false;
                LINK_camframe_free_video(releaseframe);
            }
            mFrameThreadWaitLock.unlock();
        } else {
            ALOGE("in release recordingframe, buffer not found");
            for (int i=0; i< kRecordBufferCount; i++) {
                 ALOGE(" recordframes[%d].buffer = %d", i, (unsigned int)recordframes[i].buffer);
            }
        }
    }

    ALOGV("releaseRecordingFrame X");
}

bool QualcommCameraHardware::recordingEnabled()
{
    ALOGV("%s E", __FUNCTION__);
    return mCameraRunning && mDataCallbackTimestamp && (mMsgEnabled & CAMERA_MSG_VIDEO_FRAME);
}

void QualcommCameraHardware::notifyShutter(common_crop_t *crop, bool mPlayShutterSoundOnly)
{
    ALOGV("%s E", __FUNCTION__);
    private_handle_t *thumbnailHandle;
    if (mThumbnailBuffer) {
        thumbnailHandle = (private_handle_t *) (*mThumbnailBuffer);
    }
    mShutterLock.lock();
    image_rect_type size;

    if (mPlayShutterSoundOnly) {
        /* At this point, invoke Notify Callback to play shutter sound only.
         * We want to call notify callback again when we have the
         * yuv picture ready. This is to reduce blanking at the time
         * of displaying postview frame. Using ext2 to indicate whether
         * to play shutter sound only or register the postview buffers.
         */
        mNotifyCallback(CAMERA_MSG_SHUTTER, 0, mPlayShutterSoundOnly,
                            mCallbackCookie);
        mShutterLock.unlock();
        return;
    }

    if (mShutterPending && mNotifyCallback && (mMsgEnabled & CAMERA_MSG_SHUTTER)) {
        if (mSnapshotFormat == PICTURE_FORMAT_RAW) {
            size.width = previewWidth;
            size.height = previewHeight;
            mNotifyCallback(CAMERA_MSG_SHUTTER, (int32_t)&size, 0,
                        mCallbackCookie);
            mShutterPending = false;
            mShutterLock.unlock();
            return;
        }
        ALOGV("out2_w=%d, out2_h=%d, in2_w=%d, in2_h=%d",
             crop->out2_w, crop->out2_h, crop->in2_w, crop->in2_h);
        ALOGV("out1_w=%d, out1_h=%d, in1_w=%d, in1_h=%d",
             crop->out1_w, crop->out1_h, crop->in1_w, crop->in1_h);

        // To workaround a bug in MDP which happens if either
        // dimension > 2048, we display the thumbnail instead.

        // In case of 7x27, we use output2 for postview , which is of
        // preview size. Output2 was used for thumbnail previously.
        // Now thumbnail is generated from main image for 7x27.
        if (crop->in1_w == 0 || crop->in1_h == 0) {
            // Full size
            if (mCurrentTarget == TARGET_MSM7627) {
                jpegPadding = 0;
                size.width = mDimension.ui_thumbnail_width;
                size.height = mDimension.ui_thumbnail_height;
            } else {
                size.width = mDimension.picture_width;
                size.height = mDimension.picture_height;
                if (size.width > 2048 || size.height > 2048) {
                    size.width = mDimension.ui_thumbnail_width;
                    size.height = mDimension.ui_thumbnail_height;
                }
            }
        } else {
            // Cropped
            if (mCurrentTarget == TARGET_MSM7627) {
                jpegPadding = 8;
                size.width = (crop->in1_w + jpegPadding) & ~1;
                size.height = (crop->in1_h + jpegPadding) & ~1;
            } else {
                size.width = (crop->in2_w + jpegPadding) & ~1;
                size.height = (crop->in2_h + jpegPadding) & ~1;
                if (size.width > 2048 || size.height > 2048) {
                    size.width = (crop->in1_w + jpegPadding) & ~1;
                    size.height = (crop->in1_h + jpegPadding) & ~1;
                }
            }
        }
        //We need to create overlay with dimensions that the VFE output
        //is configured for post view.
        if ((mCurrentTarget == TARGET_MSM7630) ||
           (mCurrentTarget == TARGET_MSM8660)) {
            size.width = mDimension.ui_thumbnail_width;
            size.height = mDimension.ui_thumbnail_height;
        }

        //For streaming textures, we need to pass the main image in all the cases.
        if (strTexturesOn == true) {
            int rawWidth, rawHeight;
            mParameters.getPictureSize(&rawWidth, &rawHeight);
            size.width = rawWidth;
            size.height = rawHeight;
        }

        /* Now, invoke Notify Callback to unregister preview buffer
         * and register postview buffer with surface flinger. Set ext2
         * as 0 to indicate not to play shutter sound.
         */
        mNotifyCallback(CAMERA_MSG_SHUTTER, (int32_t)&size, 0,
                        mCallbackCookie);
        mShutterPending = false;
    }
    mShutterLock.unlock();
}

static void receive_shutter_callback(common_crop_t *crop)
{
    ALOGV("receive_shutter_callback: E");
    QualcommCameraHardware* obj = QualcommCameraHardware::getInstance();
    if (obj != 0) {
        /* Just play shutter sound at this time */
        obj->notifyShutter(crop, TRUE);
    }
    ALOGV("receive_shutter_callback: X");
}

// Crop the picture in place.
static void crop_yuv420(uint32_t width, uint32_t height,
                 uint32_t cropped_width, uint32_t cropped_height,
                 uint8_t *image, const char *name)
{
    uint32_t i;
    uint32_t x, y;
    uint8_t* chroma_src, *chroma_dst;
    int yOffsetSrc, yOffsetDst, CbCrOffsetSrc, CbCrOffsetDst;
    int mSrcSize, mDstSize;

    ALOGV("%s E", __FUNCTION__);
    //check if all fields needed eg. size and also how to set y offset. If condition for 7x27
    //and need to check if needed for 7x30.

    LINK_jpeg_encoder_get_buffer_offset(width, height, (uint32_t *)&yOffsetSrc,
                                       (uint32_t *)&CbCrOffsetSrc, (uint32_t *)&mSrcSize);

    LINK_jpeg_encoder_get_buffer_offset(cropped_width, cropped_height, (uint32_t *)&yOffsetDst,
                                       (uint32_t *)&CbCrOffsetDst, (uint32_t *)&mDstSize);

    // Calculate the start position of the cropped area.
    x = (width - cropped_width) / 2;
    y = (height - cropped_height) / 2;
    x &= ~1;
    y &= ~1;

    if((mCurrentTarget == TARGET_MSM7627)
        || (mCurrentTarget == TARGET_MSM7630)
        || (mCurrentTarget == TARGET_MSM8660)) {
        if (!strcmp("snapshot camera", name)) {
            chroma_src = image + CbCrOffsetSrc;
            chroma_dst = image + CbCrOffsetDst;
        } else {
            chroma_src = image + width * height;
            chroma_dst = image + cropped_width * cropped_height;
            yOffsetSrc = 0;
            yOffsetDst = 0;
            CbCrOffsetSrc = width * height;
            CbCrOffsetDst = cropped_width * cropped_height;
        }
    } else {
       chroma_src = image + CbCrOffsetSrc;
       chroma_dst = image + CbCrOffsetDst;
    }

    int32_t bufDst = yOffsetDst;
    int32_t bufSrc = yOffsetSrc + (width * y) + x;

    if (bufDst > bufSrc ){
        ALOGV("crop yuv Y destination position follows source position");
        /*
         * If buffer destination follows buffer source, memcpy
         * of lines will lead to overwriting subsequent lines. In order
         * to prevent this, reverse copying of lines is performed
         * for the set of lines where destination follows source and
         * forward copying of lines is performed for lines where source
         * follows destination. To calculate the position to switch,
         * the initial difference between source and destination is taken
         * and divided by difference between width and cropped width. For
         * every line copied the difference between source destination
         * drops by width - cropped width
         */
        //calculating inversion
        int position = ( bufDst - bufSrc ) / (width - cropped_width);
        // Copy luma component.
        for(i=position+1; i < cropped_height; i++){
            memmove(image + yOffsetDst + i * cropped_width,
                    image + yOffsetSrc + width * (y + i) + x,
                    cropped_width);
        }
        for(int j=position; j>=0; j--){
            memmove(image + yOffsetDst + j * cropped_width,
                    image + yOffsetSrc + width * (y + j) + x,
                    cropped_width);
        }
    } else {
        // Copy luma component.
        for(i = 0; i < cropped_height; i++)
            memcpy(image + yOffsetDst + i * cropped_width,
                   image + yOffsetSrc + width * (y + i) + x,
                   cropped_width);
    }

    // Copy chroma components.
    cropped_height /= 2;
    y /= 2;

    bufDst = CbCrOffsetDst;
    bufSrc = CbCrOffsetSrc + (width * y) + x;

    if (bufDst > bufSrc) {
        ALOGV("crop yuv Chroma destination position follows source position");
        /*
         * Similar to y
         */
        int position = ( bufDst - bufSrc ) / (width - cropped_width);
        for (i=position+1; i < cropped_height; i++) {
            memmove(chroma_dst + i * cropped_width,
                    chroma_src + width * (y + i) + x,
                    cropped_width);
        }
        for(int j=position; j >=0; j--) {
            memmove(chroma_dst + j * cropped_width,
                    chroma_src + width * (y + j) + x,
                    cropped_width);
        }
    } else {
        for(i = 0; i < cropped_height; i++)
            memcpy(chroma_dst + i * cropped_width,
                   chroma_src + width * (y + i) + x,
                   cropped_width);
    }
}

bool QualcommCameraHardware::receiveRawSnapshot(){
    ALOGE("receiveRawSnapshot E");

    Mutex::Autolock cbLock(&mCallbackLock);
    /* Issue notifyShutter with mPlayShutterSoundOnly as TRUE */
    notifyShutter(&mCrop, TRUE);

    if (mDataCallback && (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE)) {

        if (native_start_ops(CAMERA_OPS_GET_PICTURE, &mCrop) == false) {
            ALOGE("receiveRawSnapshot X: CAMERA_OPS_GET_PICTURE ioctl failed!");
            return false;
        }
        /* Its necessary to issue another notifyShutter here with
         * mPlayShutterSoundOnly as FALSE, since that is when the
         * preview buffers are unregistered with the surface flinger.
         * That is necessary otherwise the preview memory wont be
         * deallocated.
         */
        notifyShutter(&mCrop, FALSE);

        if (mDataCallback && (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE))
            mDataCallback(CAMERA_MSG_COMPRESSED_IMAGE, mRawMapped,
                data_counter, NULL, mCallbackCookie);
    }

    //cleanup
    deinitRawSnapshot();

    ALOGE("receiveRawSnapshot X");
    return true;
}

bool QualcommCameraHardware::receiveRawPicture()
{
    ALOGV("receiveRawPicture: E");

    Mutex::Autolock cbLock(&mCallbackLock);
    if (mDataCallback && ((mMsgEnabled & CAMERA_MSG_RAW_IMAGE) || mSnapshotDone)) {
        if (native_start_ops(CAMERA_OPS_GET_PICTURE, &mCrop) == false) {
            ALOGE("getPicture: CAMERA_OPS_GET_PICTURE ioctl failed!");
            return false;
        }
        mSnapshotDone = FALSE;
        mCrop.in1_w &= ~1;
        mCrop.in1_h &= ~1;
        mCrop.in2_w &= ~1;
        mCrop.in2_h &= ~1;

        // Crop the image if zoomed.
        if (mCrop.in2_w != 0 && mCrop.in2_h != 0 &&
                ((mCrop.in2_w + jpegPadding) < mCrop.out2_w) &&
                ((mCrop.in2_h + jpegPadding) < mCrop.out2_h) &&
                ((mCrop.in1_w + jpegPadding) < mCrop.out1_w)  &&
                ((mCrop.in1_h + jpegPadding) < mCrop.out1_h) ) {

            // By the time native_get_picture returns, picture is taken. Call
            // shutter callback if cam config thread has not done that.
            notifyShutter(&mCrop, FALSE);

            // We do not need jpeg encoder to upscale the image. Set the new
            // dimension for encoder.
            mDimension.orig_picture_dx = mCrop.in2_w + jpegPadding;
            mDimension.orig_picture_dy = mCrop.in2_h + jpegPadding;
            /* Don't update the thumbnail_width/height, if jpeg downscaling
             * is used to generate thumbnail. These parameters should contain
             * the original thumbnail dimensions.
             */
            if (strTexturesOn != true) {
                mDimension.thumbnail_width = mCrop.in1_w + jpegPadding;
                mDimension.thumbnail_height = mCrop.in1_h + jpegPadding;
            }
        } else {
            memset(&mCrop, 0 ,sizeof(mCrop));
            // By the time native_get_picture returns, picture is taken. Call
            // shutter callback if cam config thread has not done that.
            notifyShutter(&mCrop, FALSE);
        }

        int cropX = 0;
        int cropY = 0;
        int cropW = 0;
        int cropH = 0;
        //Caculate the crop dimensions from mCrop.
        //mCrop will have the crop dimensions for VFE's
        //postview output.
        if (mCrop.in1_w != 0 && mCrop.in1_h != 0) {
            cropX = (mCrop.out1_w - mCrop.in1_w + 1) / 2 - 1;
            cropY = (mCrop.out1_h - mCrop.in1_h + 1) / 2 - 1;
            if(cropX < 0) cropX = 0;
            if(cropY < 0) cropY = 0;
            cropW = cropX + mCrop.in1_w;
            cropH = cropY + mCrop.in1_h;
            mPreviewWindow->set_crop(mPreviewWindow, cropX, cropY, cropW, cropH);
            mResetWindowCrop = true;
        } else {
            /* as the VFE second output is being used for postView,
             * VPE is doing the necessary cropping. Clear the
             * preview cropping information with overlay, so that
             * the same  won't be applied to postview.
             */
            mPreviewWindow->set_crop(mPreviewWindow, 0, 0, mDimension.ui_thumbnail_width,
                                mDimension.ui_thumbnail_height);
        }
        ALOGV("Queueing Postview for display ");

        mDisplayLock.lock();
        private_handle_t *handle;
        if (mThumbnailBuffer != NULL) {
            handle = (private_handle_t *)(*mThumbnailBuffer);
            ALOGV("%s: Queueing postview buffer for display %d", __FUNCTION__,handle->fd);
            if (BUFFER_LOCKED == mThumbnailLockState) {
                if (GENLOCK_FAILURE == genlock_unlock_buffer(handle)) {
                    ALOGE("%s: genlock_unlock_buffer failed", __FUNCTION__);
                    mDisplayLock.unlock();
                } else {
                    mThumbnailLockState = BUFFER_UNLOCKED;
                }
            }
            status_t retVal = mPreviewWindow->enqueue_buffer(mPreviewWindow,
                                                             mThumbnailBuffer);
            ALOGV("enQ thumbnailbuffer");
            if (retVal != NO_ERROR)
                ALOGE("%s: Queuebuffer failed for postview buffer", __FUNCTION__);
            mDisplayLock.unlock();
            if (handle) {
                mParameters.getPreviewSize(&previewWidth, &previewHeight);
                int mBufferSize = previewWidth * previewHeight * 3/2;
                int mCbCrOffset = PAD_TO_WORD(previewWidth * previewHeight);
                ALOGI("unregistering thumbanail buffer");
                register_buf(mBufferSize,
                           mBufferSize,
                           mCbCrOffset,
                           0,
                           handle->fd,
                           0,
                           (uint8_t *)mThumbnailMapped,
                           MSM_PMEM_THUMBNAIL,
                           false,
                           false /* unregister */);
                if (munmap((void *)mThumbnailMapped,handle->size ) == -1) {
                    ALOGE("Error un-mmapping the thumbnail buffer!!!");
                }
            }
            mThumbnailBuffer = NULL;
            mThumbnailMapped = 0;
        }

        if (mDataCallback && (mMsgEnabled & CAMERA_MSG_RAW_IMAGE))
            mDataCallback(CAMERA_MSG_RAW_IMAGE, mRawMapped,data_counter,
                          NULL, mCallbackCookie);
        else if (mNotifyCallback && (mMsgEnabled & CAMERA_MSG_RAW_IMAGE_NOTIFY))
            mNotifyCallback(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0,
                            mCallbackCookie);

        if (strTexturesOn == true) {
            ALOGI("Raw Data given to app for processing...will wait for jpeg encode call");
            mEncodePending = true;
            mEncodePendingWaitLock.unlock();
            mJpegThreadWaitLock.lock();
            mJpegThreadWait.signal();
            mJpegThreadWaitLock.unlock();
        }
    }
    else ALOGV("Raw-picture callback was canceled--skipping.");

    if (strTexturesOn != true) {
        if (mDataCallback && (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE)) {
            mJpegSize = 0;
            mJpegThreadWaitLock.lock();
            if (LINK_jpeg_encoder_init()) {
                mJpegThreadRunning = true;
                mJpegThreadWaitLock.unlock();
                if (native_jpeg_encode()) {
                    ALOGV("receiveRawPicture: X (success)");
                    return true;
                }
                ALOGE("jpeg encoding failed");
            }
            else {
                ALOGE("receiveRawPicture X: jpeg_encoder_init failed.");
                mJpegThreadWaitLock.unlock();
            }
        }
        else ALOGV("JPEG callback is NULL, not encoding image.");
        deinitRaw();
        return false;
    }
    ALOGV("receiveRawPicture: X");
    return false;
}

void QualcommCameraHardware::receiveJpegPictureFragment(
    uint8_t *buff_ptr, uint32_t buff_size)
{
    ALOGV("receiveJpegPictureFragment size %d", buff_size);
    uint32_t remaining = mJpegMapped->size;
    remaining -= mJpegSize;
    uint8_t *base = (uint8_t *)mJpegMapped->data;
    if (buff_size > remaining) {
        ALOGE("receiveJpegPictureFragment: size %d exceeds what "
             "remains in JPEG heap (%d), truncating",
             buff_size,
             remaining);
        buff_size = remaining;
    }
    memcpy(base + mJpegSize, buff_ptr, buff_size);
    mJpegSize += buff_size;
}

void QualcommCameraHardware::receiveJpegPicture(void)
{
    ALOGV("receiveJpegPicture: E image (%d uint8_ts out of %d)",
         mJpegSize, mJpegMapped->size);
    Mutex::Autolock cbLock(&mCallbackLock);

    if (mDataCallback && (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE)) {
        ALOGI("receiveJpegPicture: giving jpeg image callback to services");
        mJpegCopyMapped = mGetMemory(-1, mJpegSize, 1, mCallbackCookie);
        if (!mJpegCopyMapped) {
          ALOGE("%s: mGetMemory failed.\n", __func__);
          return;
        }
        memcpy(mJpegCopyMapped->data, mJpegMapped->data, mJpegSize);
        mDataCallback(CAMERA_MSG_COMPRESSED_IMAGE,mJpegCopyMapped,data_counter,NULL,mCallbackCookie);
        if (NULL != mJpegCopyMapped) {
           mJpegCopyMapped->release(mJpegCopyMapped);
           mJpegCopyMapped = NULL;
        }
    }
    else ALOGV("JPEG callback was cancelled--not delivering image.");

    mJpegThreadWaitLock.lock();
    mJpegThreadRunning = false;
    mJpegThreadWait.signal();
    mJpegThreadWaitLock.unlock();

    ALOGV("receiveJpegPicture: X callback done.");
}

bool QualcommCameraHardware::previewEnabled()
{
    ALOGV("%s E", __FUNCTION__);
    /* If overlay is used the message CAMERA_MSG_PREVIEW_FRAME would
     * be disabled at CameraService layer. Hence previewEnabled would
     * return FALSE even though preview is running. Hence check for
     * mOverlay not being NULL to ensure that previewEnabled returns
     * accurate information.
     */
    ALOGV("%s: mCameraRunning : %d mPreviewWindow = %p",__FUNCTION__,mCameraRunning,mPreviewWindow);
    return mCameraRunning;
}

status_t QualcommCameraHardware::setRecordSize(const QCameraParameters& params)
{
    const char *recordSize = NULL;
    recordSize = params.get(QCameraParameters::KEY_VIDEO_SIZE);
    if (!recordSize) {
        mParameters.set(QCameraParameters::KEY_VIDEO_SIZE, "");
        //If application didn't set this parameter string, use the values from
        //getPreviewSize() as video dimensions.
        ALOGV("No Record Size requested, use the preview dimensions");
        videoWidth = previewWidth;
        videoHeight = previewHeight;
    } else {
        //Extract the record witdh and height that application requested.
        ALOGI("%s: requested record size %s", __FUNCTION__, recordSize);
        if (!parse_size(recordSize, videoWidth, videoHeight)) {
            mParameters.set(QCameraParameters::KEY_VIDEO_SIZE, recordSize);
            //VFE output1 shouldn't be greater than VFE output2.
            if ((previewWidth > videoWidth) || (previewHeight > videoHeight)) {
                //Set preview sizes as record sizes.
                ALOGI("Preview size %dx%d is greater than record size %dx%d,\
                   resetting preview size to record size",previewWidth,\
                     previewHeight, videoWidth, videoHeight);
                previewWidth = videoWidth;
                previewHeight = videoHeight;
                mParameters.setPreviewSize(previewWidth, previewHeight);
            }

            //Validate record size
            int isValid = 0;
            for (size_t i = 0; i <  PREVIEW_SIZE_COUNT; ++i) {
                if (videoWidth == preview_sizes[i].width
                   && videoHeight == preview_sizes[i].height) {
                    isValid = 1;
                }
            }
            if (!isValid) {
                videoWidth = DEFAULT_VIDEO_WIDTH;
                videoHeight = DEFAULT_VIDEO_HEIGHT;
            }

            if ((mCurrentTarget != TARGET_MSM7630)
                && (mCurrentTarget != TARGET_QSD8250)
                && (mCurrentTarget != TARGET_MSM8660) ) {
                //For Single VFE output targets, use record dimensions as preview dimensions.
                previewWidth = videoWidth;
                previewHeight = videoHeight;
                mParameters.setPreviewSize(previewWidth, previewHeight);
            }
        } else {
            mParameters.set(QCameraParameters::KEY_VIDEO_SIZE, "");
            ALOGE("initPreview X: failed to parse parameter record-size (%s)", recordSize);
            return BAD_VALUE;
        }
    }
    mParameters.setVideoSize(videoWidth,videoHeight);
    ALOGI("%s: preview dimensions: %dx%d", __FUNCTION__, previewWidth, previewHeight);
    ALOGI("%s: video dimensions: %dx%d", __FUNCTION__, videoWidth, videoHeight);
    mDimension.display_width = previewWidth;
    mDimension.display_height= previewHeight;
    return NO_ERROR;
}

status_t QualcommCameraHardware::setPreviewSize(const QCameraParameters& params)
{
    int width, height;
    ALOGV("%s E", __FUNCTION__);
    params.getPreviewSize(&width, &height);
    ALOGV("requested preview size %d x %d", width, height);

    // Validate the preview size
    for (size_t i = 0; i <  PREVIEW_SIZE_COUNT; ++i) {
        if (width ==  preview_sizes[i].width
            && height ==  preview_sizes[i].height) {
            mParameters.setPreviewSize(width, height);
            previewWidth = width;
            previewHeight = height;
            mDimension.display_width = width;
            mDimension.display_height= height;
            return NO_ERROR;
        }
    }
    ALOGE("Invalid preview size requested: %dx%d", width, height);
    mParameters.setPreviewSize(previewWidth, previewHeight);
    return BAD_VALUE;
}
status_t QualcommCameraHardware::setPreviewFpsRange(const QCameraParameters& params)
{
    int minFps,maxFps;
    params.getPreviewFpsRange(&minFps,&maxFps);
    ALOGI("FPS Range Values: %dx%d", minFps, maxFps);

    for(size_t i=0;i<FPS_RANGES_SUPPORTED_COUNT;i++) {
        if (minFps==FpsRangesSupported[i].minFPS && maxFps == FpsRangesSupported[i].maxFPS) {
            mParameters.setPreviewFpsRange(minFps,maxFps);
            return NO_ERROR;
        }
    }
    return BAD_VALUE;
}

status_t QualcommCameraHardware::setPreviewFrameRate(const QCameraParameters& params)
{
    if (!mCfgControl.mm_camera_is_supported(CAMERA_PARM_FPS)) {
        ALOGI("Set fps is not supported for this sensor");
        return NO_ERROR;
    }
    uint16_t previousFps = (uint16_t)mParameters.getPreviewFrameRate();
    uint16_t fps = (uint16_t)params.getPreviewFrameRate();
    ALOGV("requested preview frame rate  is %u", fps);

    if (mInitialized && (fps == previousFps)) {
        ALOGV("fps same as previous fps");
        return NO_ERROR;
    }

    if (MINIMUM_FPS <= fps && fps <=MAXIMUM_FPS) {
        mParameters.setPreviewFrameRate(fps);
        bool ret = native_set_parms(CAMERA_PARM_FPS,
                sizeof(fps), (void *)&fps);
        return ret ? NO_ERROR : UNKNOWN_ERROR;
    }
    return BAD_VALUE;

}

status_t QualcommCameraHardware::setPreviewFrameRateMode(const QCameraParameters& params) {
    if (!mCfgControl.mm_camera_is_supported(CAMERA_PARM_FPS_MODE) && !mCfgControl.mm_camera_is_supported(CAMERA_PARM_FPS)) {
        ALOGI("set fps mode is not supported for this sensor");
        return NO_ERROR;
    }
    const char *previousMode = mParameters.getPreviewFrameRateMode();
    const char *str = params.getPreviewFrameRateMode();
    if (mInitialized && !strcmp(previousMode, str)) {
        ALOGV("frame rate mode same as previous mode %s", previousMode);
        return NO_ERROR;
    }
    int32_t frameRateMode = attr_lookup(frame_rate_modes, sizeof(frame_rate_modes) / sizeof(str_map),str);
    if (frameRateMode != NOT_FOUND) {
        ALOGV("setPreviewFrameRateMode: %s ", str);
        mParameters.setPreviewFrameRateMode(str);
        bool ret = native_set_parms(CAMERA_PARM_FPS_MODE, sizeof(frameRateMode), (void *)&frameRateMode);
        if (!ret) return ret;
        //set the fps value when chaging modes
        int16_t fps = (uint16_t)params.getPreviewFrameRate();
        if (MINIMUM_FPS <= fps && fps <=MAXIMUM_FPS) {
            mParameters.setPreviewFrameRate(fps);
            ret = native_set_parms(CAMERA_PARM_FPS,
                                        sizeof(fps), (void *)&fps);
            return ret ? NO_ERROR : UNKNOWN_ERROR;
        }
        ALOGE("Invalid preview frame rate value: %d", fps);
        return BAD_VALUE;
    }
    ALOGE("Invalid preview frame rate mode value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QualcommCameraHardware::setJpegThumbnailSize(const QCameraParameters& params){
    int width = params.getInt(QCameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
    int height = params.getInt(QCameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
    ALOGV("requested jpeg thumbnail size %d x %d", width, height);

    // Validate the picture size
    for (unsigned int i = 0; i < JPEG_THUMBNAIL_SIZE_COUNT; ++i) {
        if (width == jpeg_thumbnail_sizes[i].width
            && height == jpeg_thumbnail_sizes[i].height) {
            mParameters.set(QCameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, width);
            mParameters.set(QCameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, height);
            return NO_ERROR;
        }
    }
    return BAD_VALUE;
}

status_t QualcommCameraHardware::setPictureSize(const QCameraParameters& params)
{
    int width, height;
    ALOGV("%s E", __FUNCTION__);
    params.getPictureSize(&width, &height);
    ALOGV("requested picture size %d x %d", width, height);

    // Validate the picture size
    for (int i = 0; i < supportedPictureSizesCount; ++i) {
        if (width == picture_sizes_ptr[i].width
            && height == picture_sizes_ptr[i].height) {
            mParameters.setPictureSize(width, height);
            mDimension.picture_width = width;
            mDimension.picture_height = height;
            return NO_ERROR;
        }
    }
    /* Dimension not among the ones in the list. Check if
     * its a valid dimension, if it is, then configure the
     * camera accordingly. else reject it.
     */
    if (isValidDimension(width, height)) {
        mParameters.setPictureSize(width, height);
        mDimension.picture_width = width;
        mDimension.picture_height = height;
        return NO_ERROR;
    } else
        ALOGE("Invalid picture size requested: %dx%d", width, height);
    return BAD_VALUE;
}

status_t QualcommCameraHardware::setJpegQuality(const QCameraParameters& params) {
    status_t rc = NO_ERROR;
    ALOGV("%s E", __FUNCTION__);
    int quality = params.getInt(QCameraParameters::KEY_JPEG_QUALITY);
    if (quality >= 0 && quality <= 100) {
        mParameters.set(QCameraParameters::KEY_JPEG_QUALITY, quality);
    } else {
        ALOGE("Invalid jpeg quality=%d", quality);
        rc = BAD_VALUE;
    }

    quality = params.getInt(QCameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
    if (quality > 0 && quality <= 100) {
        mParameters.set(QCameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, quality);
    } else {
        ALOGE("Invalid jpeg thumbnail quality=%d", quality);
        rc = BAD_VALUE;
    }
    return rc;
}

status_t QualcommCameraHardware::setEffect(const QCameraParameters& params)
{
    const char *str = params.get(QCameraParameters::KEY_EFFECT);
    int result;

    if (str != NULL) {
        int32_t value = attr_lookup(effects, sizeof(effects) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            if (!mCfgControl.mm_camera_is_parm_supported(CAMERA_PARM_EFFECT, (void *) &value)) {
               ALOGE("Camera Effect - %s mode is not supported for this sensor",str);
               return NO_ERROR;
            } else {
                mParameters.set(QCameraParameters::KEY_EFFECT, str);
                bool ret = native_set_parms(CAMERA_PARM_EFFECT, sizeof(value),
                                           (void *)&value,(int *)&result);
                if (result == MM_CAMERA_ERR_INVALID_OPERATION) {
                   ALOGI("Camera Effect: %s is not set as the selected value is not supported ", str);
                }
                return ret ? NO_ERROR : UNKNOWN_ERROR;
            }
        }
    }
    ALOGE("Invalid effect value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QualcommCameraHardware::setExposureCompensation(
        const QCameraParameters & params){

    if (!mCfgControl.mm_camera_is_supported(CAMERA_PARM_EXPOSURE_COMPENSATION)) {
        ALOGI("Exposure Compensation is not supported for this sensor");
        return NO_ERROR;
    }
    int numerator = params.getInt(QCameraParameters::KEY_EXPOSURE_COMPENSATION);
    if (EXPOSURE_COMPENSATION_MINIMUM_NUMERATOR <= numerator &&
            numerator <= EXPOSURE_COMPENSATION_MAXIMUM_NUMERATOR) {
        int16_t  numerator16 = (int16_t)(numerator & 0x0000ffff);
        uint16_t denominator16 = EXPOSURE_COMPENSATION_DENOMINATOR;
        uint32_t  value = 0;
        value = numerator16 << 16 | denominator16;

        mParameters.set(QCameraParameters::KEY_EXPOSURE_COMPENSATION,
                            numerator);
        bool ret = native_set_parms(CAMERA_PARM_EXPOSURE_COMPENSATION,
                                    sizeof(value), (void *)&value);
        return ret ? NO_ERROR : UNKNOWN_ERROR;
    }
    ALOGE("Invalid Exposure Compensation");
    return BAD_VALUE;
}

status_t QualcommCameraHardware::setAutoExposure(const QCameraParameters& params)
{
    if (!mCfgControl.mm_camera_is_supported(CAMERA_PARM_EXPOSURE)) {
        ALOGI("Auto Exposure not supported for this sensor");
        return NO_ERROR;
    }
    const char *str = params.get(QCameraParameters::KEY_AUTO_EXPOSURE);
    if (str != NULL) {
        int32_t value = attr_lookup(autoexposure, sizeof(autoexposure) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            mParameters.set(QCameraParameters::KEY_AUTO_EXPOSURE, str);
            bool ret = native_set_parms(CAMERA_PARM_EXPOSURE, sizeof(value),
                                       (void *)&value);
            return ret ? NO_ERROR : UNKNOWN_ERROR;
        }
    }
    ALOGE("Invalid auto exposure value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QualcommCameraHardware::setSharpness(const QCameraParameters& params)
{
    if (!mCfgControl.mm_camera_is_supported(CAMERA_PARM_SHARPNESS)) {
        ALOGI("Sharpness not supported for this sensor");
        return NO_ERROR;
    }
    int sharpness = params.getInt(QCameraParameters::KEY_SHARPNESS);
    if ((sharpness < CAMERA_MIN_SHARPNESS
            || sharpness > CAMERA_MAX_SHARPNESS))
        return UNKNOWN_ERROR;

    ALOGV("setting sharpness %d", sharpness);
    mParameters.set(QCameraParameters::KEY_SHARPNESS, sharpness);
    bool ret = native_set_parms(CAMERA_PARM_SHARPNESS, sizeof(sharpness),
                               (void *)&sharpness);
    return ret ? NO_ERROR : UNKNOWN_ERROR;
}

status_t QualcommCameraHardware::setContrast(const QCameraParameters& params)
{
    if (!mCfgControl.mm_camera_is_supported(CAMERA_PARM_CONTRAST)) {
        ALOGI("Contrast not supported for this sensor");
        return NO_ERROR;
    }
    const char *str = params.get(QCameraParameters::KEY_SCENE_MODE);
    int32_t value = attr_lookup(scenemode, sizeof(scenemode) / sizeof(str_map), str);

    if (value == CAMERA_BESTSHOT_OFF) {
        int contrast = params.getInt(QCameraParameters::KEY_CONTRAST);
        if ((contrast < CAMERA_MIN_CONTRAST)
                || (contrast > CAMERA_MAX_CONTRAST))
            return UNKNOWN_ERROR;

        ALOGV("setting contrast %d", contrast);
        mParameters.set(QCameraParameters::KEY_CONTRAST, contrast);
        bool ret = native_set_parms(CAMERA_PARM_CONTRAST, sizeof(contrast),
                                   (void *)&contrast);
        return ret ? NO_ERROR : UNKNOWN_ERROR;
    } else {
          ALOGI(" Contrast value will not be set " \
          "when the scenemode selected is %s", str);
    return NO_ERROR;
    }
}

status_t QualcommCameraHardware::setSaturation(const QCameraParameters& params)
{
    if (!mCfgControl.mm_camera_is_supported(CAMERA_PARM_SATURATION)) {
        ALOGI("Saturation not supported for this sensor");
        return NO_ERROR;
    }
    int result;
    int saturation = params.getInt(QCameraParameters::KEY_SATURATION);

    if ((saturation < CAMERA_MIN_SATURATION)
        || (saturation > CAMERA_MAX_SATURATION))
        return UNKNOWN_ERROR;

    ALOGV("Setting saturation %d", saturation);
    mParameters.set(QCameraParameters::KEY_SATURATION, saturation);
    bool ret = native_set_parms(CAMERA_PARM_SATURATION, sizeof(saturation),
        (void *)&saturation, (int *)&result);
    if (result == MM_CAMERA_ERR_INVALID_OPERATION)
        ALOGI("Saturation Value: %d is not set as the selected value is not supported", saturation);

    return ret ? NO_ERROR : UNKNOWN_ERROR;
}

status_t QualcommCameraHardware::setPreviewFormat(const QCameraParameters& params) {
    const char *str = params.getPreviewFormat();
    int32_t previewFormat = attr_lookup(preview_formats, sizeof(preview_formats) / sizeof(str_map), str);
    if (previewFormat != NOT_FOUND) {
        mParameters.set(QCameraParameters::KEY_PREVIEW_FORMAT, str);
        mPreviewFormat = previewFormat;
        return NO_ERROR;
    }
    ALOGE("Invalid preview format value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QualcommCameraHardware::setStrTextures(const QCameraParameters& params) {
    const char *str = params.get("strtextures");
    if (str != NULL) {
        ALOGV("strtextures = %s", str);
        mParameters.set("strtextures", str);
        if (!strncmp(str, "on", 2) || !strncmp(str, "ON", 2)) {
            strTexturesOn = true;
        } else if (!strncmp(str, "off", 3) || !strncmp(str, "OFF", 3)) {
            strTexturesOn = false;
        }
    }
    return NO_ERROR;
}

status_t QualcommCameraHardware::setBrightness(const QCameraParameters& params) {
    if (!mCfgControl.mm_camera_is_supported(CAMERA_PARM_BRIGHTNESS)) {
        ALOGI("Set Brightness not supported for this sensor");
        return NO_ERROR;
    }
    int brightness = params.getInt("luma-adaptation");
    if (mBrightness !=  brightness) {
        ALOGV(" new brightness value : %d ", brightness);
        mBrightness =  brightness;
        bool ret = native_set_parms(CAMERA_PARM_BRIGHTNESS, sizeof(mBrightness),
                                   (void *)&mBrightness);
        return ret ? NO_ERROR : UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

status_t QualcommCameraHardware::setSkinToneEnhancement(const QCameraParameters& params) {
    if (!mCfgControl.mm_camera_is_supported(CAMERA_PARM_SCE_FACTOR)) {
        ALOGI("SkinToneEnhancement not supported for this sensor");
        return NO_ERROR;
    }
    int skinToneValue = params.getInt("skinToneEnhancement");
    if (mSkinToneEnhancement != skinToneValue) {
        ALOGV(" new skinTone correction value : %d ", skinToneValue);
        mSkinToneEnhancement = skinToneValue;
        mParameters.set("skinToneEnhancement", skinToneValue);
        bool ret = native_set_parms(CAMERA_PARM_SCE_FACTOR, sizeof(mSkinToneEnhancement),
                        (void *)&mSkinToneEnhancement);
        return ret ? NO_ERROR : UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

status_t QualcommCameraHardware::setWhiteBalance(const QCameraParameters& params)
{
    if (!mCfgControl.mm_camera_is_supported(CAMERA_PARM_WHITE_BALANCE)) {
        ALOGI("WhiteBalance not supported for this sensor");
        return NO_ERROR;
    }

    int result;
    const char *str = params.get(QCameraParameters::KEY_WHITE_BALANCE);
    if (str != NULL) {
        int32_t value = attr_lookup(whitebalance, sizeof(whitebalance) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            mParameters.set(QCameraParameters::KEY_WHITE_BALANCE, str);
            bool ret = native_set_parms(CAMERA_PARM_WHITE_BALANCE, sizeof(value),
                                       (void *)&value, (int *)&result);
            if(result == MM_CAMERA_ERR_INVALID_OPERATION) {
                ALOGI("WhiteBalance Value: %s is not set as the selected value is not supported ", str);
            }
            return ret ? NO_ERROR : UNKNOWN_ERROR;
        }
    }
    ALOGE("Invalid whitebalance value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QualcommCameraHardware::setFlash(const QCameraParameters& params)
{
    if (!mCfgControl.mm_camera_is_supported(CAMERA_PARM_LED_MODE)) {
        ALOGI("%s: flash not supported", __FUNCTION__);
        return NO_ERROR;
    }
    const char *str = params.get(QCameraParameters::KEY_FLASH_MODE);
    if (str != NULL) {
        int32_t value = attr_lookup(flash, sizeof(flash) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            mParameters.set(QCameraParameters::KEY_FLASH_MODE, str);
            bool ret = native_set_parms(CAMERA_PARM_LED_MODE,
                                       sizeof(value), (void *)&value);
            return ret ? NO_ERROR : UNKNOWN_ERROR;
        }
    }
    ALOGE("Invalid flash mode value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QualcommCameraHardware::setAntibanding(const QCameraParameters& params)
{   int result;
    if (!mCfgControl.mm_camera_is_supported(CAMERA_PARM_ANTIBANDING)) {
        ALOGI("Parameter AntiBanding is not supported for this sensor");
        return NO_ERROR;
    }
    const char *str = params.get(QCameraParameters::KEY_ANTIBANDING);
    if (str != NULL) {
        int value = (camera_antibanding_type)attr_lookup(
            antibanding, sizeof(antibanding) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            camera_antibanding_type temp = (camera_antibanding_type) value;
            mParameters.set(QCameraParameters::KEY_ANTIBANDING, str);
            bool ret = native_set_parms(CAMERA_PARM_ANTIBANDING,
                       sizeof(camera_antibanding_type), (void *)&temp ,(int *)&result);
            if (result == MM_CAMERA_ERR_INVALID_OPERATION) {
                ALOGI("AntiBanding Value: %s is not supported for the given BestShot Mode", str);
            }
            return ret ? NO_ERROR : UNKNOWN_ERROR;
        }
    }
    ALOGE("Invalid antibanding value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QualcommCameraHardware::setLensshadeValue(const QCameraParameters& params)
{
    if (!mCfgControl.mm_camera_is_supported(CAMERA_PARM_ROLLOFF)) {
        ALOGI("Parameter Rolloff is not supported for this sensor");
        return NO_ERROR;
    }

    const char *str = params.get(QCameraParameters::KEY_LENSSHADE);
    if (str != NULL) {
        int value = attr_lookup(lensshade,
                                    sizeof(lensshade) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            int8_t temp = (int8_t)value;
            mParameters.set(QCameraParameters::KEY_LENSSHADE, str);

            native_set_parms(CAMERA_PARM_ROLLOFF, sizeof(int8_t), (void *)&temp);
            return NO_ERROR;
        }
    }
    ALOGE("Invalid lensShade value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QualcommCameraHardware::setContinuousAf(const QCameraParameters& params)
{
    if (mHasAutoFocusSupport) {
        const char *str = params.get("continuous-af");
        if (str != NULL) {
            int value = attr_lookup(continuous_af,
                    sizeof(continuous_af) / sizeof(str_map), str);
            if (value != NOT_FOUND) {
                int8_t temp = (int8_t)value;
                mParameters.set("continuous-af", str);

                native_set_parms(CAMERA_PARM_CONTINUOUS_AF, sizeof(int8_t), (void *)&temp);
                return NO_ERROR;
            }
        }
        ALOGE("Invalid continuous Af value: %s", (str == NULL) ? "NULL" : str);
        return BAD_VALUE;
    }
    return NO_ERROR;
}

status_t QualcommCameraHardware::setSelectableZoneAf(const QCameraParameters& params)
{
    if (mHasAutoFocusSupport && supportsSelectableZoneAf()) {
        const char *str = params.get(QCameraParameters::KEY_SELECTABLE_ZONE_AF);
        if (str != NULL) {
            int32_t value = attr_lookup(selectable_zone_af, sizeof(selectable_zone_af) / sizeof(str_map), str);
            if (value != NOT_FOUND) {
                mParameters.set(QCameraParameters::KEY_SELECTABLE_ZONE_AF, str);
                bool ret = native_set_parms(CAMERA_PARM_FOCUS_RECT, sizeof(value),
                        (void *)&value);
                return ret ? NO_ERROR : UNKNOWN_ERROR;
            }
        }
        ALOGE("Invalid selectable zone af value: %s", (str == NULL) ? "NULL" : str);
        return BAD_VALUE;
    }
    return NO_ERROR;
}

status_t QualcommCameraHardware::setTouchAfAec(const QCameraParameters& params)
{
    /* Don't know the AEC_ROI_* values */
    if (mHasAutoFocusSupport) {
        int xAec, yAec, xAf, yAf;

        params.getTouchIndexAec(&xAec, &yAec);
        params.getTouchIndexAf(&xAf, &yAf);
        const char *str = params.get(QCameraParameters::KEY_TOUCH_AF_AEC);

        if (str != NULL) {
            int value = attr_lookup(touchafaec,
                    sizeof(touchafaec) / sizeof(str_map), str);
            if (value != NOT_FOUND) {

                //Dx,Dy will be same as defined in res/layout/camera.xml
                //passed down to HAL in a key.value pair.

                int FOCUS_RECTANGLE_DX = params.getInt("touchAfAec-dx");
                int FOCUS_RECTANGLE_DY = params.getInt("touchAfAec-dy");
                mParameters.set(QCameraParameters::KEY_TOUCH_AF_AEC, str);
                mParameters.setTouchIndexAec(xAec, yAec);
                mParameters.setTouchIndexAf(xAf, yAf);

                cam_set_aec_roi_t aec_roi_value;
                roi_info_t af_roi_value;

                memset(&af_roi_value, 0, sizeof(roi_info_t));

                //If touch AF/AEC is enabled and touch event has occured then
                //call the ioctl with valid values.

                if (value == true 
                        && (xAec >= 0 && yAec >= 0)
                        && (xAf >= 0 && yAf >= 0)) {
                    //Set Touch AEC params (Pass the center co-ordinate)
                    aec_roi_value.aec_roi_enable = AEC_ROI_ON;
                    aec_roi_value.aec_roi_type = AEC_ROI_BY_COORDINATE;
                    aec_roi_value.aec_roi_position.coordinate.x = xAec;
                    aec_roi_value.aec_roi_position.coordinate.y = yAec;

                    //Set Touch AF params (Pass the top left co-ordinate)
                    af_roi_value.num_roi = 1;
                    if ((xAf-(FOCUS_RECTANGLE_DX/2)) < 0)
                        af_roi_value.roi[0].x = 1;
                    else
                        af_roi_value.roi[0].x = xAf - (FOCUS_RECTANGLE_DX/2);

                    if ((yAf-(FOCUS_RECTANGLE_DY/2)) < 0)
                        af_roi_value.roi[0].y = 1;
                    else
                        af_roi_value.roi[0].y = yAf - (FOCUS_RECTANGLE_DY/2);

                    af_roi_value.roi[0].dx = FOCUS_RECTANGLE_DX;
                    af_roi_value.roi[0].dy = FOCUS_RECTANGLE_DY;
                }
                else {
                    //Set Touch AEC params
                    aec_roi_value.aec_roi_enable = AEC_ROI_OFF;
                    aec_roi_value.aec_roi_type = AEC_ROI_BY_COORDINATE;
                    aec_roi_value.aec_roi_position.coordinate.x = DONT_CARE_COORDINATE;
                    aec_roi_value.aec_roi_position.coordinate.y = DONT_CARE_COORDINATE;

                    //Set Touch AF params
                    af_roi_value.num_roi = 0;
                }
                native_set_parms(CAMERA_PARM_AEC_ROI, sizeof(cam_set_aec_roi_t), (void *)&aec_roi_value);
                native_set_parms(CAMERA_PARM_AF_ROI, sizeof(roi_info_t), (void*)&af_roi_value);
            }
            return NO_ERROR;
        }
        ALOGE("Invalid Touch AF/AEC value: %s", (str == NULL) ? "NULL" : str);
        return BAD_VALUE;
    }
    return NO_ERROR;
}

status_t QualcommCameraHardware::setFaceDetection(const char *str)
{
    if (supportsFaceDetection() == false){
        ALOGI("Face detection is not enabled");
        return NO_ERROR;
    }
    if (str != NULL) {
        int value = attr_lookup(facedetection,
                                    sizeof(facedetection) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            mMetaDataWaitLock.lock();
            mFaceDetectOn = value;
            mMetaDataWaitLock.unlock();
            mParameters.set(QCameraParameters::KEY_FACE_DETECTION, str);
            return NO_ERROR;
        }
    }
    ALOGE("Invalid Face Detection value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t  QualcommCameraHardware::setISOValue(const QCameraParameters& params) {
    int8_t temp_hjr;
    if (!mCfgControl.mm_camera_is_supported(CAMERA_PARM_ISO)) {
        ALOGE("Parameter ISO Value is not supported for this sensor");
        return NO_ERROR;
    }
    const char *str = params.get(QCameraParameters::KEY_ISO_MODE);
    if (str != NULL) {
        int value = (camera_iso_mode_type)attr_lookup(
            iso, sizeof(iso) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            camera_iso_mode_type temp = (camera_iso_mode_type) value;
            if (value == CAMERA_ISO_DEBLUR) {
               temp_hjr = true;
               native_set_parms(CAMERA_PARM_HJR, sizeof(int8_t), (void*)&temp_hjr);
               mHJR = value;
            }
            else {
               if (mHJR == CAMERA_ISO_DEBLUR) {
                   temp_hjr = false;
                   native_set_parms(CAMERA_PARM_HJR, sizeof(int8_t), (void*)&temp_hjr);
                   mHJR = value;
               }
            }

            mParameters.set(QCameraParameters::KEY_ISO_MODE, str);
            native_set_parms(CAMERA_PARM_ISO, sizeof(camera_iso_mode_type), (void *)&temp);
            return NO_ERROR;
        }
    }
    ALOGE("Invalid Iso value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QualcommCameraHardware::setSceneDetect(const QCameraParameters& params)
{

    bool retParm1, retParm2;
    if (supportsSceneDetection()) {
        if (!mCfgControl.mm_camera_is_supported(CAMERA_PARM_BL_DETECTION) && !mCfgControl.mm_camera_is_supported(CAMERA_PARM_SNOW_DETECTION)) {
            ALOGE("Parameter Auto Scene Detection is not supported for this sensor");
            return NO_ERROR;
        }
        const char *str = params.get(QCameraParameters::KEY_SCENE_DETECT);
        if (str != NULL) {
            int32_t value = attr_lookup(scenedetect, sizeof(scenedetect) / sizeof(str_map), str);
            if (value != NOT_FOUND) {
                mParameters.set(QCameraParameters::KEY_SCENE_DETECT, str);

                retParm1 = native_set_parms(CAMERA_PARM_BL_DETECTION, sizeof(value),
                                           (void *)&value);

                retParm2 = native_set_parms(CAMERA_PARM_SNOW_DETECTION, sizeof(value),
                                           (void *)&value);

                //All Auto Scene detection modes should be all ON or all OFF.
                if (retParm1 == false || retParm2 == false) {
                    value = !value;
                    retParm1 = native_set_parms(CAMERA_PARM_BL_DETECTION, sizeof(value),
                                               (void *)&value);

                    retParm2 = native_set_parms(CAMERA_PARM_SNOW_DETECTION, sizeof(value),
                                               (void *)&value);
                }
                return (retParm1 && retParm2) ? NO_ERROR : UNKNOWN_ERROR;
            }
        }
        ALOGE("Invalid auto scene detection value: %s", (str == NULL) ? "NULL" : str);
        return BAD_VALUE;
    }
    return NO_ERROR;
}

status_t QualcommCameraHardware::setSceneMode(const QCameraParameters& params)
{
    if (!mCfgControl.mm_camera_is_supported(CAMERA_PARM_BESTSHOT_MODE)) {
        ALOGE("Parameter Scenemode not supported for this sensor");
        return NO_ERROR;
    }

    const char *str = params.get(QCameraParameters::KEY_SCENE_MODE);
    if (str != NULL) {
        int32_t value = attr_lookup(scenemode, sizeof(scenemode) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            mParameters.set(QCameraParameters::KEY_SCENE_MODE, str);
            bool ret = native_set_parms(CAMERA_PARM_BESTSHOT_MODE, sizeof(value),
                                       (void *)&value);
            return ret ? NO_ERROR : UNKNOWN_ERROR;
        }
    }
    ALOGE("Invalid scenemode value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}
status_t QualcommCameraHardware::setGpsLocation(const QCameraParameters& params)
{
    ALOGV("%s E", __FUNCTION__);
    const char *method = params.get(QCameraParameters::KEY_GPS_PROCESSING_METHOD);
    if (method) {
        mParameters.set(QCameraParameters::KEY_GPS_PROCESSING_METHOD, method);
    } else {
        mParameters.remove(QCameraParameters::KEY_GPS_PROCESSING_METHOD);
    }

    const char *latitude = params.get(QCameraParameters::KEY_GPS_LATITUDE);
    if (latitude) {
        ALOGE("latitude %s",latitude);
        mParameters.set(QCameraParameters::KEY_GPS_LATITUDE, latitude);
    } else {
        mParameters.remove(QCameraParameters::KEY_GPS_LATITUDE);
    }

    const char *latitudeRef = params.get(QCameraParameters::KEY_GPS_LATITUDE_REF);
    if (latitudeRef) {
        mParameters.set(QCameraParameters::KEY_GPS_LATITUDE_REF, latitudeRef);
    } else {
        mParameters.remove(QCameraParameters::KEY_GPS_LATITUDE_REF);
    }

    const char *longitude = params.get(QCameraParameters::KEY_GPS_LONGITUDE);
    if (longitude) {
        mParameters.set(QCameraParameters::KEY_GPS_LONGITUDE, longitude);
    } else {
        mParameters.remove(QCameraParameters::KEY_GPS_LONGITUDE);
    }

    const char *longitudeRef = params.get(QCameraParameters::KEY_GPS_LONGITUDE_REF);
    if (longitudeRef) {
        mParameters.set(QCameraParameters::KEY_GPS_LONGITUDE_REF, longitudeRef);
    } else {
        mParameters.remove(QCameraParameters::KEY_GPS_LONGITUDE_REF);
    }

    const char *altitudeRef = params.get(QCameraParameters::KEY_GPS_ALTITUDE_REF);
    if (altitudeRef) {
        mParameters.set(QCameraParameters::KEY_GPS_ALTITUDE_REF, altitudeRef);
    } else {
        mParameters.remove(QCameraParameters::KEY_GPS_ALTITUDE_REF);
    }

    const char *altitude = params.get(QCameraParameters::KEY_GPS_ALTITUDE);
    if (altitude) {
        mParameters.set(QCameraParameters::KEY_GPS_ALTITUDE, altitude);
    } else {
        mParameters.remove(QCameraParameters::KEY_GPS_ALTITUDE);
    }

    const char *status = params.get(QCameraParameters::KEY_GPS_STATUS);
    if (status) {
        mParameters.set(QCameraParameters::KEY_GPS_STATUS, status);
    }

    const char *dateTime = params.get(QCameraParameters::KEY_EXIF_DATETIME);
    if (dateTime) {
        mParameters.set(QCameraParameters::KEY_EXIF_DATETIME, dateTime);
    } else {
        mParameters.remove(QCameraParameters::KEY_EXIF_DATETIME);
    }

    const char *timestamp = params.get(QCameraParameters::KEY_GPS_TIMESTAMP);
    if (timestamp) {
        mParameters.set(QCameraParameters::KEY_GPS_TIMESTAMP, timestamp);
    } else {
        mParameters.remove(QCameraParameters::KEY_GPS_TIMESTAMP);
    }

    return NO_ERROR;
}

status_t QualcommCameraHardware::setRotation(const QCameraParameters& params)
{
    status_t rc = NO_ERROR;
    ALOGV("%s E", __FUNCTION__);
    int rotation = params.getInt(QCameraParameters::KEY_ROTATION);
    if (rotation != NOT_FOUND) {
        if (rotation == 0 || rotation == 90 || rotation == 180
            || rotation == 270) {
            mParameters.set(QCameraParameters::KEY_ROTATION, rotation);
            mRotation = rotation;
        } else {
            ALOGE("Invalid rotation value: %d", rotation);
            rc = BAD_VALUE;
        }
    }
    return rc;
}

status_t QualcommCameraHardware::setZoom(const QCameraParameters& params)
{
    if (!mCfgControl.mm_camera_is_supported(CAMERA_PARM_ZOOM)) {
        ALOGI("Parameter setZoom is not supported for this sensor");
        return NO_ERROR;
    }
    status_t rc = NO_ERROR;
    // No matter how many different zoom values the driver can provide, HAL
    // provides applictations the same number of zoom levels. The maximum driver
    // zoom value depends on sensor output (VFE input) and preview size (VFE
    // output) because VFE can only crop and cannot upscale. If the preview size
    // is bigger, the maximum zoom ratio is smaller. However, we want the
    // zoom ratio of each zoom level is always the same whatever the preview
    // size is. Ex: zoom level 1 is always 1.2x, zoom level 2 is 1.44x, etc. So,
    // we need to have a fixed maximum zoom value and do read it from the
    // driver.
    static const int ZOOM_STEP = 1;
    int32_t zoom_level = params.getInt("zoom");
    ALOGV("Set zoom, level=%d", zoom_level);
    if (zoom_level >= 0 && zoom_level <= mMaxZoom-1) {
        mParameters.set("zoom", zoom_level);
        int32_t zoom_value = ZOOM_STEP * zoom_level;
        bool ret = native_set_parms(CAMERA_PARM_ZOOM,
            sizeof(zoom_value), (void *)&zoom_value);
        rc = ret ? NO_ERROR : UNKNOWN_ERROR;
    } else {
        rc = BAD_VALUE;
    }
    return rc;
}

status_t QualcommCameraHardware::setFocusMode(const QCameraParameters& params)
{
    const char *str = params.get(QCameraParameters::KEY_FOCUS_MODE);
    if (str != NULL) {
        int32_t value = attr_lookup(focus_modes,
                                    sizeof(focus_modes) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            mParameters.set(QCameraParameters::KEY_FOCUS_MODE, str);
            // Focus step is reset to infinity when preview is started. We do
            // not need to do anything now.
            return NO_ERROR;
        }
    }
    ALOGE("Invalid focus mode value: %s", (str == NULL) ? "NULL" : str);
    return BAD_VALUE;
}

status_t QualcommCameraHardware::setOrientation(const QCameraParameters& params)
{
    ALOGV("%s E", __FUNCTION__);
    const char *str = params.get("orientation");

    if (str != NULL) {
        if (strcmp(str, "portrait") == 0 || strcmp(str, "landscape") == 0) {
            // Camera service needs this to decide if the preview frames and raw
            // pictures should be rotated.
            mParameters.set("orientation", str);
        } else {
            ALOGE("Invalid orientation value: %s", str);
            return BAD_VALUE;
        }
    }
    return NO_ERROR;
}

status_t QualcommCameraHardware::setPictureFormat(const QCameraParameters& params)
{
    ALOGV("%s E", __FUNCTION__);
    const char * str = params.get(QCameraParameters::KEY_PICTURE_FORMAT);

    if (str != NULL) {
        int32_t value = attr_lookup(picture_formats,
                                    sizeof(picture_formats) / sizeof(str_map), str);
        if (value != NOT_FOUND) {
            mParameters.set(QCameraParameters::KEY_PICTURE_FORMAT, str);
        } else {
            ALOGE("Invalid Picture Format value: %s", str);
            return BAD_VALUE;
        }
    }
    return NO_ERROR;
}

QualcommCameraHardware::MMCameraDL::MMCameraDL(){
    ALOGV("MMCameraDL: E");
    libmmcamera = NULL;
#if DLOPEN_LIBMMCAMERA
    libmmcamera = ::dlopen("liboemcamera.so", RTLD_NOW);
#endif
    ALOGV("Open MM camera DL libeomcamera loaded at %p ", libmmcamera);
    ALOGV("MMCameraDL: X");
}

void * QualcommCameraHardware::MMCameraDL::pointer(){
    ALOGV("MMCameraDL::pointer(): EX");
    return libmmcamera;
}

QualcommCameraHardware::MMCameraDL::~MMCameraDL(){
    ALOGV("~MMCameraDL: E");
    LINK_mm_camera_destroy();
#if DLOPEN_LIBMMCAMERA
    if (libmmcamera != NULL) {
        ::dlclose(libmmcamera);
        ALOGV("closed MM Camera DL ");
    }
    libmmcamera = NULL;
#endif
    ALOGV("~MMCameraDL: X");
}

wp<QualcommCameraHardware::MMCameraDL> QualcommCameraHardware::MMCameraDL::instance;
Mutex QualcommCameraHardware::MMCameraDL::singletonLock;


sp<QualcommCameraHardware::MMCameraDL> QualcommCameraHardware::MMCameraDL::getInstance(){
    ALOGV("MMCameraDL::getInstance(): E");
    Mutex::Autolock instanceLock(singletonLock);
    sp<MMCameraDL> mmCamera = instance.promote();
    if (mmCamera == NULL){
        mmCamera = new MMCameraDL();
        instance = mmCamera;
    }
    ALOGV("MMCameraDL::getInstance(): X");
    return mmCamera;
}

QualcommCameraHardware::MemPool::MemPool(int buffer_size, int num_buffers,
                                         int frame_size,
                                         const char *name) :
    mBufferSize(buffer_size),
    mNumBuffers(num_buffers),
    mFrameSize(frame_size),
    mBuffers(NULL), mName(name)
{
    ALOGV("%s E", __FUNCTION__);
    int page_size_minus_1 = getpagesize() - 1;
    mAlignedBufferSize = (buffer_size + page_size_minus_1) & (~page_size_minus_1);
}

void QualcommCameraHardware::MemPool::completeInitialization()
{
    // If we do not know how big the frame will be, we wait to allocate
    // the buffers describing the individual frames until we do know their
    // size.
    ALOGV("%s E", __FUNCTION__);

    if (mFrameSize > 0) {
        ALOGE("Before new Mem BASE #buffers :%d",mNumBuffers);
        mBuffers = new sp<MemoryBase>[mNumBuffers];
        for (int i = 0; i < mNumBuffers; i++) {
            mBuffers[i] = new
                MemoryBase(mHeap,
                           i * mAlignedBufferSize,
                           mFrameSize);
        }
    }
}

QualcommCameraHardware::AshmemPool::AshmemPool(int buffer_size, int num_buffers,
                                               int frame_size,
                                               const char *name) :
    QualcommCameraHardware::MemPool(buffer_size,
                                    num_buffers,
                                    frame_size,
                                    name)
{
    ALOGV("constructing MemPool %s backed by ashmem: "
         "%d frames @ %d uint8_ts, "
         "buffer size %d",
         mName,
         num_buffers, frame_size, buffer_size);

    int page_mask = getpagesize() - 1;
    int ashmem_size = buffer_size * num_buffers;
    ashmem_size += page_mask;
    ashmem_size &= ~page_mask;

    mHeap = new MemoryHeapBase(ashmem_size);

    completeInitialization();
}

QualcommCameraHardware::MemPool::~MemPool()
{
    ALOGV("destroying MemPool %s", mName);
    if (mFrameSize > 0)
        delete [] mBuffers;
    mHeap.clear();
    ALOGV("destroying MemPool %s completed", mName);
}

static bool register_buf(int size,
                         int frame_size,
                         int cbcr_offset,
                         int yoffset,
                         int pmempreviewfd,
                         uint32_t offset,
                         uint8_t *buf,
                         int pmem_type,
                         bool vfe_can_write,
                         bool register_buffer)
{
    struct msm_pmem_info pmemBuf;

    pmemBuf.type     = pmem_type;
    pmemBuf.fd       = pmempreviewfd;
    pmemBuf.offset   = offset;
    pmemBuf.len      = size;
    pmemBuf.vaddr    = buf;
    pmemBuf.y_off    = yoffset;
    pmemBuf.cbcr_off = cbcr_offset;

    pmemBuf.active   = vfe_can_write;

    ALOGV("register_buf: reg = %d buffer = %p",
         !register_buffer, buf);
    if (native_start_ops(register_buffer ? CAMERA_OPS_REGISTER_BUFFER :
         CAMERA_OPS_UNREGISTER_BUFFER ,(void *)&pmemBuf) < 0) {
         ALOGE("register_buf: MSM_CAM_IOCTL_(UN)REGISTER_PMEM  error %s",
             strerror(errno));
         return false;
    }

    return true;

}

status_t QualcommCameraHardware::MemPool::dump(int fd, const Vector<String16>& args) const
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    CAMERA_HAL_UNUSED(args);
    snprintf(buffer, 255, "QualcommCameraHardware::AshmemPool::dump\n");
    result.append(buffer);
    if (mName) {
        snprintf(buffer, 255, "mem pool name (%s)\n", mName);
        result.append(buffer);
    }
    if (mHeap != 0) {
        snprintf(buffer, 255, "heap base(%p), size(%d), flags(%d), device(%s)\n",
                 mHeap->getBase(), mHeap->getSize(),
                 mHeap->getFlags(), mHeap->getDevice());
        result.append(buffer);
    }
    snprintf(buffer, 255,
             "buffer size (%d), number of buffers (%d), frame size(%d)",
             mBufferSize, mNumBuffers, mFrameSize);
    result.append(buffer);
    write(fd, result.string(), result.size());
    return NO_ERROR;
}

static void receive_camframe_callback(struct msm_frame *frame)
{
    ALOGV("%s E", __FUNCTION__);
    QualcommCameraHardware* obj = QualcommCameraHardware::getInstance();
    if (obj != 0) {
        obj->receivePreviewFrame(frame);
    }
}

static void receive_camstats_callback(camstats_type stype, camera_preview_histogram_info* histinfo)
{
    QualcommCameraHardware* obj = QualcommCameraHardware::getInstance();
    if (obj != 0) {
        obj->receiveCameraStats(stype,histinfo);
    }
}

static void receive_liveshot_callback(liveshot_status status, uint32_t jpeg_size)
{
    if (status == LIVESHOT_SUCCESS) {
        QualcommCameraHardware* obj = QualcommCameraHardware::getInstance();
        if (obj != 0) {
            obj->receiveLiveSnapshot(jpeg_size);
        }
    }
    else
        ALOGE("Liveshot not succesful");
}

static void receive_jpeg_fragment_callback(uint8_t *buff_ptr, uint32_t buff_size)
{
    ALOGV("receive_jpeg_fragment_callback E");
    QualcommCameraHardware* obj = QualcommCameraHardware::getInstance();
    if (obj != 0) {
        obj->receiveJpegPictureFragment(buff_ptr, buff_size);
    }
    ALOGV("receive_jpeg_fragment_callback X");
}

static void receive_jpeg_callback(jpeg_event_t status)
{
    ALOGV("receive_jpeg_callback E (completion status %d)", status);
    if (status == JPEG_EVENT_DONE) {
        QualcommCameraHardware* obj = QualcommCameraHardware::getInstance();
        if (obj != 0) {
            obj->receiveJpegPicture();
        }
    }
    ALOGV("receive_jpeg_callback X");
}

// 720p : video frame calbback from camframe
static void receive_camframe_video_callback(struct msm_frame *frame)
{
    ALOGV("receive_camframe_video_callback E");
    QualcommCameraHardware* obj = QualcommCameraHardware::getInstance();
    if (obj != 0) {
        obj->receiveRecordingFrame(frame);
    }
    ALOGV("receive_camframe_video_callback X");
}

void QualcommCameraHardware::setCallbacks(camera_notify_callback notify_cb,
                             camera_data_callback data_cb,
                             camera_data_timestamp_callback data_cb_timestamp,
                             camera_request_memory get_memory,
                             void* user)
{
    ALOGV("%s E", __FUNCTION__);
    Mutex::Autolock lock(mLock);
    mNotifyCallback = notify_cb;
    mDataCallback = data_cb;
    mDataCallbackTimestamp = data_cb_timestamp;
    mGetMemory = get_memory;
    mCallbackCookie = user;
}

int32_t QualcommCameraHardware::getNumberOfVideoBuffers()
{
    ALOGV("getNumOfVideoBuffers: %d", kRecordBufferCount);
    return kRecordBufferCount;
}

sp<IMemory> QualcommCameraHardware::getVideoBuffer(int32_t index)
{
    if (index > kRecordBufferCount)
        return NULL;
    else
        return NULL;
}

void QualcommCameraHardware::enableMsgType(int32_t msgType)
{
    ALOGV("%s E", __FUNCTION__);
    Mutex::Autolock lock(mLock);
    mMsgEnabled |= msgType;
}

void QualcommCameraHardware::disableMsgType(int32_t msgType)
{
    ALOGV("%s E", __FUNCTION__);
    Mutex::Autolock lock(mLock);
    mMsgEnabled &= ~msgType;
}

bool QualcommCameraHardware::msgTypeEnabled(int32_t msgType)
{
    ALOGV("%s E", __FUNCTION__);
    return (mMsgEnabled & msgType);
}

void QualcommCameraHardware::receive_camframe_error_timeout(void) {
    ALOGI("receive_camframe_error_timeout: E");
    Mutex::Autolock l(&mCamframeTimeoutLock);
    ALOGE("Camframe timed out. Not receiving any frames from camera driver ");
    camframe_timeout_flag = TRUE;
    mNotifyCallback(CAMERA_MSG_ERROR, CAMERA_ERROR_UNKNOWN, 0,
                    mCallbackCookie);
    ALOGI("receive_camframe_error_timeout: X");
}

static void receive_camframe_error_callback(camera_error_type err) {
    QualcommCameraHardware* obj = QualcommCameraHardware::getInstance();
    if (obj != 0) {
        if ((err == CAMERA_ERROR_TIMEOUT) ||
            (err == CAMERA_ERROR_ESD)) {
            /* Handling different error types is dependent on the requirement.
             * Do the same action by default
             */
            obj->receive_camframe_error_timeout();
        }
    }
}

bool QualcommCameraHardware::storePreviewFrameForPostview(void) {
    ALOGV("storePreviewFrameForPostview : E ");

    /* Since there is restriction on the maximum overlay dimensions
     * that can be created, we use the last preview frame as postview
     * for 7x30. */
    ALOGV("storePreviewFrameForPostview : X ");
    return true;
}

bool QualcommCameraHardware::isValidDimension(int width, int height) {
    ALOGV("%s E", __FUNCTION__);
    bool retVal = FALSE;
    /* This function checks if a given resolution is valid or not.
     * A particular resolution is considered valid if it satisfies
     * the following conditions:
     * 1. width & height should be multiple of 16.
     * 2. width & height should be less than/equal to the dimensions
     *    supported by the camera sensor.
     * 3. the aspect ratio is a valid aspect ratio and is among the
     *    commonly used aspect ratio as determined by the thumbnail_sizes
     *    data structure.
     */

    if ((width == CEILING16(width)) && (height == CEILING16(height))
        && (width <= maxSnapshotWidth)
        && (height <= maxSnapshotHeight)) {
        uint32_t pictureAspectRatio = (uint32_t)((width * Q12)/height);
        for(uint32_t i = 0; i < THUMBNAIL_SIZE_COUNT; i++ ) {
            if (thumbnail_sizes[i].aspect_ratio == pictureAspectRatio) {
                retVal = TRUE;
                break;
            }
        }
    }
    return retVal;
}
status_t QualcommCameraHardware::getBufferInfo(sp<IMemory>& Frame, size_t *alignedSize) {
    status_t ret = UNKNOWN_ERROR;
    ALOGV("getBufferInfo E ");
    if ((mCurrentTarget == TARGET_MSM7630) || (mCurrentTarget == TARGET_QSD8250) || (mCurrentTarget == TARGET_MSM8660)) {
        ALOGE("RecordHeap is null. Buffer information wont be updated ");
        Frame = NULL;
        ret = UNKNOWN_ERROR;
    } else {
        ALOGE("PreviewHeap is null. Buffer information wont be updated ");
        Frame = NULL;
        ret = UNKNOWN_ERROR;
    }
    ALOGV("getBufferInfo X ");
    return ret;
}

void QualcommCameraHardware::encodeData() {
    ALOGV("encodeData: E");

    if (mDataCallback && (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE)) {
        mJpegSize = 0;
        mJpegThreadWaitLock.lock();
        if (LINK_jpeg_encoder_init()) {
            mJpegThreadRunning = true;
            mJpegThreadWaitLock.unlock();
            if (native_jpeg_encode()) {
                ALOGV("encodeData: X (success)");
                //Wait until jpeg encoding is done and call jpeg join
                //in this context. Also clear the resources.
                mJpegThreadWaitLock.lock();
                while (mJpegThreadRunning) {
                    ALOGV("encodeData: waiting for jpeg thread to complete.");
                    mJpegThreadWait.wait(mJpegThreadWaitLock);
                    ALOGV("encodeData: jpeg thread completed.");
                }
                mJpegThreadWaitLock.unlock();
                //Call jpeg join in this thread context
                LINK_jpeg_encoder_join();
            }
            ALOGE("encodeData: jpeg encoding failed");
        }
        else {
            ALOGE("encodeData X: jpeg_encoder_init failed.");
            mJpegThreadWaitLock.unlock();
        }
    }
    else ALOGV("encodeData: JPEG callback is NULL, not encoding image.");
    //clear the resources
    deinitRaw();
    //Encoding is done.
    mEncodePendingWaitLock.lock();
    mEncodePending = false;
    mEncodePendingWait.signal();
    mEncodePendingWaitLock.unlock();

    ALOGV("encodeData: X");
}

void QualcommCameraHardware::getCameraInfo()
{
    struct msm_camera_info camInfo;
    int i, ret;

    ALOGV("%s E", __FUNCTION__);
    int camfd = open(MSM_CAMERA_CONTROL, O_RDWR);
    if (camfd >= 0) {
        ret = ioctl(camfd, MSM_CAM_IOCTL_GET_CAMERA_INFO, &camInfo);
        close(camfd);

        if (ret < 0) {
             ALOGE("getCameraInfo: MSM_CAM_IOCTL_GET_CAMERA_INFO fd %d error %s",
                  camfd, strerror(errno));
             HAL_numOfCameras = 0;
             return;
        }

        for (i = 0; i < camInfo.num_cameras; ++i) {
             HAL_cameraInfo[i].camera_id = i + 1;
             HAL_cameraInfo[i].position = camInfo.is_internal_cam[i] == 1 ? FRONT_CAMERA : BACK_CAMERA;
             HAL_cameraInfo[i].sensor_mount_angle = camInfo.s_mount_angle[i];
             HAL_cameraInfo[i].modes_supported = CAMERA_MODE_2D;
             if (camInfo.has_3d_support[i])
                  HAL_cameraInfo[i].modes_supported |= CAMERA_MODE_3D;

             ALOGV("camera %d, facing: %d, orientation: %d, mode: %d\n", HAL_cameraInfo[i].camera_id, 
                  HAL_cameraInfo[i].position, HAL_cameraInfo[i].sensor_mount_angle, HAL_cameraInfo[i].modes_supported);
        }
        HAL_numOfCameras = camInfo.num_cameras;
    }
    ALOGV("HAL_numOfCameras: %d\n", HAL_numOfCameras);
    ALOGV("%s X", __FUNCTION__);
}

extern "C" int HAL_getNumberOfCameras()
{
    QualcommCameraHardware::getCameraInfo();
    return HAL_numOfCameras;
}

extern "C" void HAL_getCameraInfo(int cameraId, struct CameraInfo* cameraInfo)
{
    int i;
    char mDeviceName[PROPERTY_VALUE_MAX];
    if (cameraInfo == NULL) {
        ALOGE("cameraInfo is NULL");
        return;
    }

    property_get("ro.board.platform",mDeviceName," ");

    /* Update camera info if needed */
    if (HAL_numOfCameras < 1) HAL_getNumberOfCameras();

    for (i = 0; i < HAL_numOfCameras; i++) {
        if (i == cameraId) {
            ALOGI("Found a matching camera info for ID %d", cameraId);
            cameraInfo->facing = (HAL_cameraInfo[i].position == BACK_CAMERA)?
                                   CAMERA_FACING_BACK : CAMERA_FACING_FRONT;
            // App Orientation not needed for 7x27 , sensor mount angle 0 is
            // enough.
            if(cameraInfo->facing == CAMERA_FACING_FRONT)
                cameraInfo->orientation = HAL_cameraInfo[i].sensor_mount_angle;
            else if( !strncmp(mDeviceName, "msm7627", 7))
                cameraInfo->orientation = HAL_cameraInfo[i].sensor_mount_angle;
            else if( !strncmp(mDeviceName, "msm8660", 7))
                cameraInfo->orientation = HAL_cameraInfo[i].sensor_mount_angle;
            else
                cameraInfo->orientation = ((APP_ORIENTATION - HAL_cameraInfo[i].sensor_mount_angle) + 360)%360;

            ALOGI("%s: orientation = %d", __FUNCTION__, cameraInfo->orientation);
#if 0
            cameraInfo->mode = 0;
            if (HAL_cameraInfo[i].modes_supported & CAMERA_MODE_2D)
                cameraInfo->mode |= CAMERA_SUPPORT_MODE_2D;
            if (HAL_cameraInfo[i].modes_supported & CAMERA_MODE_3D)
                cameraInfo->mode |= CAMERA_SUPPORT_MODE_3D;

            ALOGI("%s: modes supported = %d", __FUNCTION__, cameraInfo->mode);
#endif
            return;
        }
    }
    ALOGE("Unable to find matching camera info for ID %d", cameraId);
}

extern "C" QualcommCameraHardware* HAL_openCameraHardware(int cameraId)
{
    int i;
    ALOGI("openCameraHardware: call createInstance");

    /* Update camera info if needed */
    if (HAL_numOfCameras < 1) HAL_getNumberOfCameras();

    for (i = 0; i < HAL_numOfCameras; i++) {
        if (i == cameraId) {
            ALOGI("openCameraHardware:Valid camera ID %d", cameraId);
            parameter_string_initialized = false;
            HAL_currentCameraId = cameraId;
            return QualcommCameraHardware::createInstance();
        }
    }
    ALOGE("openCameraHardware:Invalid camera ID %d", cameraId);
    return NULL;
}

}; // namespace android

