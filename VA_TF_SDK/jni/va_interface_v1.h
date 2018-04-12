/*
 * Copyright (c) 2016 AndroVideo, Inc.
 * All Rights Reserved.
 *
 * Third-party developers SHOULD NOT modify this file
 */

#ifndef __VA_INTERFACE_V1_H__
#define __VA_INTERFACE_V1_H__

#include "va_const.h"

typedef struct va_module_v1 {

    /* Used API version of this module */
    uint16_t api_version;

    /* Name of this module */
    const char *name;

    /* Author of the module */
    const char *author;

    /* VA module methods */
    struct va_module_methods_v1* methods;

} va_module_v1;

typedef struct va_frame {
    uint32_t    format; // 0: NV12, 1: NV21
    uint32_t    width;
    uint32_t    height;
    void        *plane0;
    uint32_t    stride0;
    uint32_t    size0;
    void        *plane1;
    uint32_t    stride1;
    uint32_t    size1;
    int64_t     time_stamp;
} va_frame;

typedef struct va_rect {
    int32_t start_x;
    int32_t start_y;
    int32_t width;
    int32_t height;
} va_rect;

typedef struct va_data {
    uint32_t  data_size;
    uint8_t   data[MAX_EVENTDATA_LEN];
} va_data;

typedef struct va_event {
    int32_t             event_type;
    char                event_name[MAX_EVENTNAME_LEN];
    va_rect             event_box;
    va_data             event_data;
} va_event;

typedef struct va_module_methods_v1 {

    int32_t (*init)(void **handle);

    int32_t (*get_supported_events)(void *handle, int32_t *events,
                                    int32_t *size);

    int32_t (*process)(void *handle, struct va_frame *frame);

    int32_t (*set_config)(void *handle, char *config, size_t size,
            uint32_t config_type, char *metaConfig, size_t metaSize);

    int32_t (*get_config_size)(void *handle, uint32_t config_type,
            uint32_t *configSize);

    int32_t (*get_config)(void *handle, uint32_t config_type,
            uint8_t *config, uint32_t configSize);

    int32_t (*get_events_count)(void *handle, int32_t *count);

    int32_t (*get_events)(void *handle, va_event *events, int32_t num_events);

    int32_t (*set_event)(void *handle, int32_t type, bool flag);

    int32_t (*deinit)( void *handle);

} va_module_methods_v1;


#endif
