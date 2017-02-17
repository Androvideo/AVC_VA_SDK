#include <android/log.h>
#include <stdlib.h>
#include <stdio.h>

#include "va_interface_v1.h"

#define LOG_TAG   "va_template"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

int32_t template_init(void **handle)
{
    return 0;
}

int32_t template_get_supported_events(void *handle, int32_t *events,
                                    int32_t *size)
{
    return 0;
}

int32_t template_process(void *handle, struct va_frame *frame)
{
    return 0;
}

int32_t template_get_events_count(void *handle, int32_t *count)
{
    return 0;
}

int32_t template_get_events(void *handle, va_event *events, int32_t num_events)
{
    return 0;
}

int32_t template_deinit( void *handle)
{
    return 0;
}

int32_t template_set_config(void *handle, char *config, size_t size,
            uint32_t config_type, char *metaConfig, size_t metaSize)
{
    // Not used
    return 0;
}

int32_t template_get_config_size(void *handle, uint32_t config_type,
            uint32_t *configSize)
{
    // Not used
    return 0;
}


int32_t template_get_config(void *handle, uint32_t config_type,
            uint8_t *config, uint32_t configSize)
{
    // Not used
    return 0;
}

int32_t template_set_event(void *handle, int32_t type, bool flag)
{
    // Not used
    return 0;
}

va_module_methods_v1 template_methods = {
    .init                   = template_init,
    .get_supported_events   = template_get_supported_events,
    .process                = template_process,
    .set_config             = template_set_config,
    .get_config_size        = template_get_config_size,
    .get_config             = template_get_config,
    .get_events_count       = template_get_events_count,
    .get_events             = template_get_events,
    .set_event              = template_set_event,
    .deinit                 = template_deinit,
};

va_module_v1 VA_ALG_INFO_SYM = {
    .api_version = VA_API_V1,
    .name = "template",
    .author = "your name",
    .methods = &template_methods,
};
