#include <android/log.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "va_interface_v1.h"
#include "AdvMotionTrack.h"

#define LOG_TAG   "va_motion_sample"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define WORKING_WIDTH   320
#define WORKING_HEIGHT  180
#define DEFAULT_SENSITIVITY 5

typedef struct {
    CAdvMotionTrack    *motionTrack;
    uint32_t           frame_width;
    uint32_t           frame_height;
    uint32_t           frame_idx;
    bool               motion_detected;
    bool               last_event;
} motion_param_t;

typedef struct {
    uint32_t           frame_width;
    uint32_t           frame_height;
    uint32_t           frame_idx;
    bool               motion_detected;
    bool               last_event;
} meta_data;

int32_t motiontrack_init(void **handle)
{
    ALOGI("%s: ", __func__);
    motion_param_t *local_data = (motion_param_t *)malloc(sizeof(motion_param_t));
    *handle = (void *)local_data;
    local_data->frame_idx = 0;
    local_data->motion_detected = 0;

    local_data->motionTrack = new CAdvMotionTrack(WORKING_WIDTH, WORKING_HEIGHT,
            DEFAULT_SENSITIVITY, true, true);

    return 0;
}

int32_t motiontrack_get_supported_events(void *handle, int32_t *events,
                                    int32_t *size)
{
    ALOGI("%s: ", __func__);
    events[0] = VA_EVENT_USER_0; // motion detect
    events[1] = VA_EVENT_USER_1; // motion vanish
    *size = 2;

    return 0;
}

int32_t motiontrack_process(void *handle, struct va_frame *frame)
{
    motion_param_t* ldata = (motion_param_t*) handle;

    ldata->last_event = ldata->motion_detected;  // backup last status
    ldata->frame_idx++;                          // update frame id for new frame

    if (ldata->frame_idx == 1){
        ALOGI("%s:format %u", __func__, frame->format);
        ALOGI("%s:width %u", __func__, frame->width);
        ALOGI("%s:height %u", __func__, frame->height);
        ALOGI("%s:plane0 %p", __func__, frame->plane0);
        ALOGI("%s:stride0 %u", __func__, frame->stride0);
        ALOGI("%s:size0 %u", __func__, frame->size0);
        ALOGI("%s:plane1 %p", __func__, frame->plane1);
        ALOGI("%s:stride1 %u", __func__, frame->stride1);
        ALOGI("%s:size1 %u", __func__, frame->size1);
        ALOGI("%s:time_stamp %lld", __func__, frame->time_stamp);
        ALOGI("%s:working size %dx%d", __func__, WORKING_WIDTH, WORKING_HEIGHT);

        ldata->frame_width = frame->width;
        ldata->frame_height = frame->height;
        ldata->motionTrack->AddMotionArea(0, 0,
                frame->width, frame->height,
                frame->width, frame->height,
                DEFAULT_SENSITIVITY);
    }

    ldata->motion_detected = ldata->motionTrack->MotionDetection(
            (unsigned char *)frame->plane0, frame->width, frame->height, false);

    if (ldata->motion_detected != ldata->last_event){
        ALOGI("%s: frame_idx %d: motion_detected %d", __func__, ldata->frame_idx, ldata->motion_detected);
    }

    return 0;
}

int32_t motiontrack_set_config(void *handle, char *config, size_t size,
            uint32_t config_type, char *metaConfig, size_t metaSize)
{
    // Not used
    return 0;
}

int32_t motiontrack_get_config_size(void *handle, uint32_t config_type,
            uint32_t *configSize)
{
    // Not used
    return 0;
}


int32_t motiontrack_get_config(void *handle, uint32_t config_type,
            uint8_t *config, uint32_t configSize)
{
    // Not used
    return 0;
}

int32_t motiontrack_get_events_count(void *handle, int32_t *count)
{
    motion_param_t* ldata = (motion_param_t*) handle;
    if (ldata->motion_detected || ldata->last_event) {
        *count = 1;
    } else {
        *count = 0;
    }
    return 0;
}

int32_t motiontrack_get_events(void *handle, va_event *events, int32_t num_events)
{
    motion_param_t* ldata = (motion_param_t*) handle;

    if (ldata->motion_detected) {
        events[0].event_type = VA_EVENT_USER_0;
        sprintf(events[0].event_name, "MOTION_SAMPLE_DETECTED");
    } else {
        events[0].event_type = VA_EVENT_USER_1;
        sprintf(events[0].event_name, "MOTION_SAMPLE_VANISHED");
    }
    events[0].event_box.start_x = 0;
    events[0].event_box.start_y = 0;
    events[0].event_box.width = 0;
    events[0].event_box.height = 0;

    meta_data meta;
    meta.frame_width        = ldata->frame_width;
    meta.frame_height       = ldata->frame_height;
    meta.frame_idx          = ldata->frame_idx;
    meta.motion_detected    = ldata->motion_detected;
    meta.last_event         = ldata->last_event;

    events[0].event_data.data_size = sizeof(meta_data);
    memcpy(&(events[0].event_data.data[0]), (uint8_t*)&meta, sizeof(meta_data));

    return 0;
}


int32_t motiontrack_set_event(void *handle, int32_t type, bool flag)
{
    // Not used
    return 0;
}

int32_t motiontrack_deinit( void *handle)
{
    ALOGI("%s: ", __func__);

    motion_param_t* ldata = (motion_param_t*) handle;

    if (ldata->motionTrack != NULL){
        delete ldata->motionTrack;
        ldata->motionTrack = NULL;
    }

    if (handle) {
        free(handle);
    }

    return 0;
}

va_module_methods_v1 motiontrack_methods = {
    .init                   = motiontrack_init,
    .get_supported_events   = motiontrack_get_supported_events,
    .process                = motiontrack_process,
    .set_config             = motiontrack_set_config,
    .get_config_size        = motiontrack_get_config_size,
    .get_config             = motiontrack_get_config,
    .get_events_count       = motiontrack_get_events_count,
    .get_events             = motiontrack_get_events,
    .set_event              = motiontrack_set_event,
    .deinit                 = motiontrack_deinit,
};

va_module_v1 VA_ALG_INFO_SYM = {
    .api_version = VA_API_V1,
    .name = "motiontrack_sample",
    .author = "avc",
    .methods = &motiontrack_methods,
};
