# Build Tensorflow Model In Android Camera

## Prerequisites

* Ubuntu 14.04 LTS
* Bazel 0.11.1
* Tensorflow r1.4
* Android NDK r14b

## Installation
* Android NDK:

	Download Android NDK r14b, and place at ```~/```
	
	```bash
	~$ unzip android-ndk-r14b-linux-x86_64.zip
	```

* Tensorflow:

	```bash
	~$ git clone https://github.com/tensorflow/tensorflow.git
	~$ cd tensorflow
	~/tensorflow$ git checkout r1.3
	
	```
	Edit ~/tensorflow/WORKSPACE
	
	```
	# Uncomment and update the paths in these entries to build the Android demo.
	android_sdk_repository(
	    name = "androidsdk",
	    api_level = 25,
	    # Ensure that you have the build_tools_version below installed in the
	    # SDK manager as it updates periodically.
	    build_tools_version = "26.0.1",
	    # Replace with path to Android SDK on your system
	    path = "/home/USER/Andorid/Sdk",
	)
	
	android_ndk_repository(
	    name="androidndk",
	    path="/home/USER/android-ndk-r14b",
	    # This needs to be 14 or higher to compile TensorFlow.
	    # Please specify API level to >= 21 to build for 64-bit
	    # archtectures or the Android NDK will automatically select biggest
	    # API level that it supports without notice.
	    # Note that the NDK version is not the API level.
	    api_level=21)
	```
	Edit ~/tensorflow/tensorflow/workspace.bzl
	
	```
	def tf_workspace(path_prefix="", tf_repo_name=""):
	  # We must check the bazel version before trying to parse any other BUILD
	  # files, in case the parsing of those build files depends on the bazel
	  # version we require here.
	  check_bazel_version_at_least("0.11.1")
	  cuda_configure(name="local_config_cuda")
	  sycl_configure(name="local_config_sycl")
	  python_configure(name="local_config_python")
	```

* Bazel:

	```bash
	~$ sudo apt-get install openjdk-8-jdk
	~$ sudo add-apt-repository ppa:webupd8team/java
	~$ sudo apt-get update && sudo apt-get install oracle-java8-installer
	~$ echo "deb [arch=amd64] http://storage.googleapis.com/bazel-apt stable jdk1.8" | sudo tee /etc/apt/sources.list.d/bazel.list
	curl https://bazel.build/bazel-release.pub.gpg | sudo apt-key add -
	~$ sudo apt-get update && sudo apt-get install bazel
	```
	If you want to build from source for specific version:
	
	```
	~$ mkdir bazel && cd bazel
	~/bazel$ wget https://github.com/bazelbuild/bazel/releases/download/0.11.1/bazel-0.11.1-dist.zip
	~/bazel$ unzip bazel-0.11.1-dist.zip
	~/bazel$ ./compile.sh
	```
	The output executable is now located in ```output/bazel```. Add a ```PATH``` entry to your ```.bashrc```, or just export it in your current shell:
	
	```
	~/$ export PATH=`pwd`/output:$PATH
	```

## Build Tensorflow libary for Android Camera

1. Extract tensorflow operator in your model

	```bash
	~$ cd tensorflow
	~/tensorflow$ ./configure
	```
	Check your python path and python package path are right.
	
	And then:
	
	```
	~/tensorflow$ bazel build \
				  --incompatible_load_argument_is_label=false \
				  --incompatible_disallow_uncalled_set_constructor=false \
				  tensorflow/python/tools:print_selective_registration_header && \
	              bazel-bin/tensorflow/python/tools/print_selective_registration_header \
	              --graphs=path/to/your/model/graph.pb > ops_to_register.h
	~/tensorflow$ mv ops_to_register.h tensorflow/core/framework
	```
	If you face some build error about bazel varsion check.
	It a bug from bazel.
	
	Edit the file from ```"/home/USER/.cache/bazel/_bazel_USER/xxxxxxxx/external/io_bazel_rules_closure/closure/repositories.bzl", line 69, in closure_repositories```
	
	```_check_bazel_version("Closure Rules", "0.4.5")```
	
	to
	
	```_check_bazel_version("Closure Rules", "0.11.1")```
	
	The path is correspond to your error message.
   
   And try again.
	
2. Compile your own tensorflow libary for android

	Add the following statement to the end of the file ```tensorflow/contrib/android/BUILD```:
	
	```
	cc_binary(
	    name = "libtensorflow.so",
	    srcs = [],
	    copts = tf_copts() + [
	        "-ffunction-sections",
	        "-fdata-sections",
	    ],
	    linkopts = if_android([
	        "-landroid",
	        "-llog",
	        "-lm",
	        "-z defs",
	        "-s",
	        "-Wl,--gc-sections",
		# soname is required for the so to load on api > 22
	        "-Wl,-soname=libtensorflow.so",
	        "-Wl,--version-script",
	        "//tensorflow/c:version_script.lds",
	    ]),
	    linkshared = 1,
	    linkstatic = 1,
	    tags = [
	        "manual",
	        "notap",
	    ],
	    deps = [
	        "//tensorflow/c:c_api",
	        "//tensorflow/c:version_script.lds",
	        "//tensorflow/core:android_tensorflow_lib",
	    ],
	)
	```
	
	Now you can start building tensorflow libary.
	
	```
	~/tensorflow$ bazel build -c opt --copt="-DSELECTIVE_REGISTRATION" \
			      --copt="-DSUPPORT_SELECTIVE_REGISTRATION" \
			      //tensorflow/contrib/android:libtensorflow.so \
			      --verbose_failures \
			      --incompatible_load_argument_is_label=false \
				  --incompatible_disallow_uncalled_set_constructor=false \
			      --host_crosstool_top=@bazel_tools//tools/cpp:toolchain \
			      --crosstool_top=//external:android/crosstool \
			      --cpu=armeabi-v7a
	```
	
3. The files ```bazel-bin/tensorflow/contrib/android/libtensorflow.so``` and ```tensorflow/c/c_api.h``` are all you need for android camera

## Basic usage for Tensorflow C API

* frozen your model as frozen_model.pb first

```c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <tensorflow/c/c_api.h>


TF_Buffer* read_file(const char* file);                                                   
void free_buffer(void* data, size_t length) {                                             
        free(data);                                                                       
}

static void Deallocator(void* data, size_t length, void* arg) {
        free(data);
        // *reinterpret_cast<bool*>(arg) = true;
}

int main() {
  printf("Hello from TensorFlow C library version %s\n", TF_Version());

  TF_Buffer* graph_def = read_file("frozen_model.pb");                      
  TF_Graph* graph = TF_NewGraph();

  // Import graph_def into graph                                                          
  TF_Status* status = TF_NewStatus();                                                     
  TF_ImportGraphDefOptions* opts = TF_NewImportGraphDefOptions();                         
  TF_GraphImportGraphDef(graph, graph_def, opts, status);
  TF_DeleteImportGraphDefOptions(opts);
  if (TF_GetCode(status) != TF_OK) {
          fprintf(stderr, "ERROR: Unable to import graph %s", TF_Message(status));        
          return 1;
  }       
  fprintf(stdout, "Successfully imported graph\n");
  
  #
  # The following block can help you find your correct operation name
  #
  /*
  int n_ops = 700;
  for (int i=0; i<n_ops; i++){
    size_t pos = i;
    std::cout << "Input: " << TF_OperationName(TF_GraphNextOperation(graph, &pos)) << "\n";
  }
  */
  
  // Create variables to store the size of the input and output variables
  const int num_bytes_in = 300 * 300 * 3 * sizeof(uint8_t);
  const int num_bytes_out = 100 * sizeof(float);

  // Set input dimensions - this should match the dimensionality of the input in
  // the loaded graph, in this case it's four dimensional.
  int64_t in_dims[4] = {1, 300, 300, 3};
  int64_t out_dims[2] = {1, 100};

  // ######################
  // Set up graph inputs
  // ######################

  // Create a variable containing your values, in this case the input is a
  // 3-dimensional float
  uint8_t values[1][300][300][3] = {{{{0}}}};

  // Create vectors to store graph input operations and input tensors
  std::vector<TF_Output> inputs;
  std::vector<TF_Tensor*> input_values;

  // Pass the graph and a string name of your input operation
  // (make sure the operation name is correct)
  TF_Operation* input_op = TF_GraphOperationByName(graph, "image_tensor");
  TF_Output input_opout = {input_op, 0};
  inputs.push_back(input_opout);

  // Create the input tensor using the dimension (in_dims) and size (num_bytes_in)
  // variables created earlier
  TF_Tensor* input = TF_NewTensor(TF_UINT8, in_dims, 4, values, num_bytes_in, &Deallocator, 0);
  input_values.push_back(input);

  // Optionally, you can check that your input_op and input tensors are correct
  // by using some of the functions provided by the C API.
  std::cout << "Input data info: " << TF_Dim(input, 0) << "\n";
  std::cout << "Input op info: " << TF_OperationNumOutputs(input_op) << "\n";
  

  // Create vector to store graph output operations
  std::vector<TF_Output> outputs;
  TF_Operation* output_op = TF_GraphOperationByName(graph, "detection_scores");
  TF_Output output_opout = {output_op, 0};
  outputs.push_back(output_opout);

  // Create TF_Tensor* vector
  std::vector<TF_Tensor*> output_values(outputs.size(), nullptr);

  // Similar to creating the input tensor, however here we don't yet have the
  // output values, so we use TF_AllocateTensor()
  TF_Tensor* output_value = TF_AllocateTensor(TF_FLOAT, out_dims, 2, num_bytes_out);
  output_values.push_back(output_value);

  // As with inputs, check the values for the output operation and output tensor
  std::cout << "Output: " << TF_OperationName(output_op) << "\n";
  std::cout << "Output info: " << TF_Dim(output_value, 0) << "\n";

  // ######################
  // Run graph
  // ######################
  fprintf(stdout, "Running session...\n");
  TF_SessionOptions* sess_opts = TF_NewSessionOptions();
  TF_Session* session = TF_NewSession(graph, sess_opts, status);
  assert(TF_GetCode(status) == TF_OK);
                                                     
  std::cout<<inputs.size()<<"---------"<<outputs.size()<<std::endl;
  // Call TF_SessionRun
  TF_SessionRun(session, nullptr,
                &inputs[0], &input_values[0], inputs.size(),
                &outputs[0], &output_values[0], outputs.size(),
                nullptr, 0, nullptr, status);
  if (TF_GetCode(status) != TF_OK) {
          fprintf(stderr, "ERROR: Unable to import graph %s", TF_Message(status));        
          return 1;
  }  
  
  // Assign the values from the output tensor to a variable and iterate over them
  float* out_vals = static_cast<float*>(TF_TensorData(output_values[0]));
  for (int i = 0; i < 100; ++i)
  {
      std::cout << i << " Output values info: " << *out_vals << "\n";
      out_vals++;
  }

  fprintf(stdout, "Successfully run session\n");

  // Delete variables
  TF_CloseSession(session, status);
  TF_DeleteSession(session, status);
  TF_DeleteSessionOptions(sess_opts);
  TF_DeleteBuffer(graph_def);
  TF_DeleteGraph(graph);
  TF_DeleteStatus(status);
  std::cout<<"Finished!"<<std::endl;
  return 0;
}

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
```
