#include <android/log.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>
#include <time.h>
#include <vector>

#include <include/tensorflow/c/c_api.h>

#include "va_interface_v1.h"

#define LOG_TAG   "va_tf_mobilenet_ssd"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

typedef struct {
    TF_Status          *status;
    TF_Session         *session;
    TF_Tensor          *input;
    TF_Tensor          *output_scores;
    TF_Tensor          *output_boxes;
    TF_Tensor          *output_classes;
    std::vector<TF_Tensor*> output_values;
    TF_Output          input_opout;
    TF_Output          output_opout;
    std::vector<TF_Output> outputs;

    int64_t            in_dims[4];
    int64_t            out_dims[2];
    int64_t            out_scores_dims[2];
    int64_t            out_boxes_dims[3];
    int64_t            out_classes_dims[2];
    unsigned char      values[1][1080][1920][3];
    unsigned char      *YDataScale;
    unsigned char      *UVDataScale;
    int                num_bytes_in;
    int                num_bytes_out;
    int                num_bytes_scores;
    int                num_bytes_boxes;
    int                num_bytes_classes;
    uint32_t           frame_width;
    uint32_t           frame_height;
    uint32_t           frame_idx;
} motion_param_t;

typedef struct {
    uint32_t           frame_width;
    uint32_t           frame_height;
    uint32_t           frame_idx;
    bool               motion_detected;
    bool               last_event;
} meta_data;

TF_Buffer* read_file(const char* file);                                                   
void free_buffer(void* data, size_t length) {                                             
        free(data);                                                                       
}

static void Deallocator(void* data, size_t length, void* arg) {
        //free(data);
        // *reinterpret_cast<bool*>(arg) = true;
}

int32_t tf_mobilenet_ssd_init(void **handle)
{
    ALOGI("%s: ", __func__);
    motion_param_t *local_data = (motion_param_t *)malloc(sizeof(motion_param_t));
    *handle = (void *)local_data;

    // Create new graph and load graph_def
    TF_Buffer *graph_def = read_file("/vendor/lib/frozen_inference_graph.pb");
    TF_Graph *graph = TF_NewGraph();
    // Import graph_def into graph                                                          
    local_data->status = TF_NewStatus();                                                     
    TF_ImportGraphDefOptions* opts = TF_NewImportGraphDefOptions();                         
    TF_GraphImportGraphDef(graph, graph_def, opts, local_data->status);
    TF_DeleteImportGraphDefOptions(opts);
    TF_DeleteBuffer(graph_def);

    if (TF_GetCode(local_data->status) != TF_OK) {
        ALOGI("ERROR: Unable to import graph %s", TF_Message(local_data->status));        
        return 1;
    }
    else{
        ALOGI("Successfully imported graph");
    }

    local_data->YDataScale = new unsigned char[1920 * 1080];
    local_data->UVDataScale = new unsigned char[1920 * 540];
    // Prepare input and outputs information
    int64_t in_dims[4] = {1, 1080, 1920, 3};
    memcpy(local_data->in_dims, in_dims, sizeof(in_dims));

    int64_t out_scores_dims[2] = {1, 100};
    int64_t out_boxes_dims[3] = {1, 100, 4};
    int64_t out_classes_dims[2] = {1, 100};
    memcpy(local_data->out_scores_dims, out_scores_dims, sizeof(out_scores_dims));
    memcpy(local_data->out_boxes_dims, out_boxes_dims, sizeof(out_boxes_dims));
    memcpy(local_data->out_classes_dims, out_classes_dims, sizeof(out_classes_dims));

    local_data->num_bytes_in = 1080 * 1920 * 3 * sizeof(uint8_t);
    local_data->num_bytes_scores = 100 * sizeof(float);
    local_data->num_bytes_boxes = 100 * 4 * sizeof(float);
    local_data->num_bytes_classes = 100 * sizeof(int);

    // Create input and output nodes from model
    TF_Operation* input_op = TF_GraphOperationByName(graph, "image_tensor");
    local_data->input_opout = {input_op, 0};

    TF_Operation* output_scores_op = TF_GraphOperationByName(graph, "detection_scores");
    TF_Output output_scores_opout = {output_scores_op, 0};
    TF_Operation* output_boxes_op = TF_GraphOperationByName(graph, "detection_boxes");
    TF_Output output_boxes_opout = {output_boxes_op, 0};
    TF_Operation* output_classes_op = TF_GraphOperationByName(graph, "detection_classes");
    TF_Output output_classes_opout = {output_classes_op, 0};

    local_data->outputs.push_back(output_scores_opout);
    local_data->outputs.push_back(output_boxes_opout);
    local_data->outputs.push_back(output_classes_opout);

    local_data->output_scores = TF_AllocateTensor(TF_FLOAT, local_data->out_scores_dims, 2, local_data->num_bytes_scores);
    local_data->output_boxes = TF_AllocateTensor(TF_FLOAT, local_data->out_boxes_dims, 3, local_data->num_bytes_boxes);
    local_data->output_classes = TF_AllocateTensor(TF_INT32, local_data->out_classes_dims, 2, local_data->num_bytes_classes);
    local_data->output_values.push_back(local_data->output_scores);
    local_data->output_values.push_back(local_data->output_boxes);
    local_data->output_values.push_back(local_data->output_classes);

    // Create tensorflow session
    ALOGI("Running session...");
    TF_SessionOptions* sess_opts = TF_NewSessionOptions();
    local_data->session = TF_NewSession(graph, sess_opts, local_data->status);
    assert(TF_GetCode(local_data->status) == TF_OK);
    return 0;
}

int32_t tf_mobilenet_ssd_get_supported_events(void *handle, int32_t *events,
                                    int32_t *size)
{
    return 0;
}

int32_t tf_mobilenet_ssd_process(void *handle, struct va_frame *frame)
{
    motion_param_t* ldata = (motion_param_t*) handle;

    ldata->frame_idx++;                          // update frame id for new frame
    ALOGI("Hello from TensorFlow C library version %s", TF_Version());

    float START,END;
    START = clock();

    ALOGI("width %u", frame->width);
    ALOGI("height %u", frame->height);

    // Prepare new input for model
    memcpy(ldata->YDataScale, frame->plane0, frame->width*frame->height);
    memcpy(ldata->UVDataScale, frame->plane1, frame->size1);

    float Y, U, V;
    for (int i=0; i < frame->height; i++){
        for (int j=0; j < frame->width; j++){
            
            Y = ldata->YDataScale[i * frame->stride0 + j];
            V = ldata->UVDataScale[ (int)( (i/2)*frame->stride1 + (j/2)*2 )];
            U = ldata->UVDataScale[ (int)( (i/2)*frame->stride1 + (j/2)*2 ) + 1];
            ldata->values[0][i][j][0] = static_cast<unsigned char>( Y + 1.402 * (V - 128) );
            ldata->values[0][i][j][1] = static_cast<unsigned char>( Y - 0.344 * (U - 128) - 0.714 * (V - 128) );
            ldata->values[0][i][j][2] = static_cast<unsigned char>( Y + 1.772 * (U - 128) );

        }
    }

    TF_Tensor* input = TF_NewTensor(TF_UINT8, ldata->in_dims, 4, ldata->values, ldata->num_bytes_in, &Deallocator, 0);
    ALOGI("---------Start inference-----------------");
    TF_SessionRun(ldata->session, nullptr,
                  &ldata->input_opout, &input, 1,
                  &ldata->outputs[0], &ldata->output_values[0], 3,
                  nullptr, 0, nullptr, ldata->status);
    
    if (TF_GetCode(ldata->status) != TF_OK) {
        ALOGI("ERROR: Unable to run session %s", TF_Message(ldata->status));        
        return 1;
    }
    else{
        ALOGI("Successfully inference");
    }

    TF_DeleteTensor(input);
    END = clock();
    ALOGI("inference cost time: %f s", (END - START) / CLOCKS_PER_SEC);

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

        ldata->frame_width = frame->width;
        ldata->frame_height = frame->height;
    }

    return 0;
}

int32_t tf_mobilenet_ssd_get_events_count(void *handle, int32_t *count)
{
    motion_param_t* ldata = (motion_param_t*) handle;
    float* scores = static_cast<float*>(TF_TensorData(ldata->output_values[0]));
    float threshold = 0.35;
    for (int i = 0; i < 100; ++i)
    {
        if (*scores > threshold) (*count)++;
        scores++;
    }
    return 0;
}

int32_t tf_mobilenet_ssd_get_events(void *handle, va_event *events, int32_t num_events)
{
    motion_param_t* ldata = (motion_param_t*) handle;
    float* boxes = static_cast<float*>(TF_TensorData(ldata->output_values[1]));
    meta_data meta;
    meta.frame_width        = ldata->frame_width;
    meta.frame_height       = ldata->frame_height;
    meta.frame_idx          = 1;
    meta.motion_detected    = true;
    meta.last_event         = true;

    for (int i = 0; i < num_events; i++){
        events[i].event_type = VA_EVENT_USER_0;
        sprintf(events[i].event_name, "TF_MOBILENET_SSD_object_%d", i);
	events[i].event_box.start_x = static_cast<int> (*(boxes + 1) * 1920);
	events[i].event_box.start_y = static_cast<int> (*(boxes + 0) * 1080);
	events[i].event_box.width = static_cast<int> ((*(boxes + 3) - *(boxes + 1)) * 1920);
	events[i].event_box.height = static_cast<int> ((*(boxes + 2) - *(boxes + 0)) * 1080);
        events[i].event_data.data_size = sizeof(meta_data);
        memcpy(&(events[i].event_data.data[0]), (uint8_t*)&meta, sizeof(meta_data));
        boxes += 4;
    }
    

    
    return 0;
}

int32_t tf_mobilenet_ssd_deinit( void *handle)
{
    ALOGI("%s: ", __func__);

    motion_param_t* ldata = (motion_param_t*) handle;

    TF_DeleteTensor(ldata->input);
    TF_DeleteTensor(ldata->output_scores);
    TF_DeleteTensor(ldata->output_boxes);
    TF_DeleteTensor(ldata->output_classes);
    TF_CloseSession(ldata->session, ldata->status);
    TF_DeleteSession(ldata->session, ldata->status);
    TF_DeleteStatus(ldata->status);

    if (handle) {
        free(handle);
    }
    return 0;
}

int32_t tf_mobilenet_ssd_set_config(void *handle, char *config, size_t size,
            uint32_t config_type, char *metaConfig, size_t metaSize)
{
    // Not used
    return 0;
}

int32_t tf_mobilenet_ssd_get_config_size(void *handle, uint32_t config_type,
            uint32_t *configSize)
{
    // Not used
    return 0;
}


int32_t tf_mobilenet_ssd_get_config(void *handle, uint32_t config_type,
            uint8_t *config, uint32_t configSize)
{
    // Not used
    return 0;
}

int32_t tf_mobilenet_ssd_set_event(void *handle, int32_t type, bool flag)
{
    // Not used
    return 0;
}

va_module_methods_v1 tf_mobilenet_ssd_methods = {
    .init                   = tf_mobilenet_ssd_init,
    .get_supported_events   = tf_mobilenet_ssd_get_supported_events,
    .process                = tf_mobilenet_ssd_process,
    .set_config             = tf_mobilenet_ssd_set_config,
    .get_config_size        = tf_mobilenet_ssd_get_config_size,
    .get_config             = tf_mobilenet_ssd_get_config,
    .get_events_count       = tf_mobilenet_ssd_get_events_count,
    .get_events             = tf_mobilenet_ssd_get_events,
    .set_event              = tf_mobilenet_ssd_set_event,
    .deinit                 = tf_mobilenet_ssd_deinit,
};

va_module_v1 VA_ALG_INFO_SYM = {
    .api_version = VA_API_V1,
    .name = "tf_mobilenet_ssd",
    .author = "avc",
    .methods = &tf_mobilenet_ssd_methods,
};

TF_Buffer* read_file(const char* file) {

  FILE *f = fopen(file, "rb");
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);                                                                  
  fseek(f, 0, SEEK_SET);  //same as rewind(f);                                            

  void* data = malloc(fsize);                                                             
  fread(data, fsize, 1, f);
  fclose(f);

  TF_Buffer* buf = TF_NewBuffer();                                                        
  buf->data = data;
  buf->length = fsize;                                                                    
  buf->data_deallocator = free_buffer;
                                                    
  return buf;
}
