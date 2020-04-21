#include "opencl-filters.hpp"
#include <cl2.hpp>
#include <iostream>
#include <fstream>

using namespace std;
using namespace cl;

cl::Platform platform;
cl::Device device;
cl::Context context;
cl::CommandQueue queue;
cl::Program program;

bool openCL_initialized = false;
int device_index = 0;

void initCL(bool verbose){
    /* Find PLATFORM */
    cl::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    if(platforms.empty()){
        cerr << "No OpenCL platforms found, aborting." << endl;
        exit(1);
    }
    if (verbose){
        cout << "Available platforms:" << endl;
        for (unsigned int i = 0; i < platforms.size(); i++)
        cout << "\t* " << platforms[i].getInfo<CL_PLATFORM_NAME>() << ", " << platforms[i].getInfo<CL_PLATFORM_VENDOR>() << endl;
    }
    platform = platforms[0]; // la primera platform

    /* Find available DEVICES */
    cl::vector<Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
    if (verbose){
        cout << "Available devices:" << endl;
        for (unsigned int i = 0; i < devices.size(); i++)
        cout << "\t* " << devices[i].getInfo<CL_DEVICE_NAME>() << ", " << devices[i].getInfo<CL_DEVICE_VENDOR>() << endl;
    }
    device = devices[device_index]; // el primer device

    /* Create CONTEXT on that platform */
    cl_context_properties context_properties[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)(platform)(), 0};
    context = cl::Context(devices, context_properties);

    /* Create COMMAND QUEUE for a device */
    queue = CommandQueue(context, device);

    if (verbose){
        cout << "Using platform: " << platform.getInfo<CL_PLATFORM_NAME>() << endl;
        cout << "Using device: " << device.getInfo<CL_DEVICE_NAME>() << endl;
    }
    openCL_initialized = true;
}

void selectDevice(int i) {
    device_index = i;
}

void createProgram(string filename) {
    // Read source
    /* NOTE: reading the source code from another file
    during runtime makes the binary's location important.
    Not good. */
    ifstream sourceFile(std::string("opencl/") + filename);
    if (!sourceFile.is_open()) {
        cerr << "Can't open CL kernel source." << endl;
        exit(1);
    }
    string sourceCode(istreambuf_iterator<char>(sourceFile), (istreambuf_iterator<char>()));
    Program::Sources sources(1, sourceCode);

    // Build program
    cl_int err = 0;
    program = Program(context, sources, &err);
    clHandleError(__FILE__, __LINE__, err);
    //const char* options = "-Werror";
    const char* options = "";
    err = program.build(context.getInfo<CL_CONTEXT_DEVICES>(), options);
    clHandleError(__FILE__, __LINE__, err);
}

void createProgram2(char* sourceCode) {
    Program::Sources sources(1, sourceCode);

    // Build program
    cl_int err = 0;
    program = Program(context, sources, &err);
    clHandleError(__FILE__, __LINE__, err);
    //const char* options = "-Werror";
    const char* options = "";
    err = program.build(context.getInfo<CL_CONTEXT_DEVICES>(), options);
    clHandleError(__FILE__, __LINE__, err);
}

void printImageFormat(ImageFormat format){
    #define CASE(order) case order: cout << #order; break;
    switch (format.image_channel_order){
        CASE(CL_R);
        CASE(CL_A);
        CASE(CL_RG);
        CASE(CL_RA);
        CASE(CL_RGB);
        CASE(CL_RGBA);
        CASE(CL_BGRA);
        CASE(CL_ARGB);
        CASE(CL_INTENSITY);
        CASE(CL_LUMINANCE);
        CASE(CL_Rx);
        CASE(CL_RGx);
        CASE(CL_RGBx);
        CASE(CL_DEPTH);
        CASE(CL_DEPTH_STENCIL);
    }
    #undef CASE

    cout << " - ";

    #define CASE(type) case type: cout << #type; break;
    switch (format.image_channel_data_type){
        CASE(CL_SNORM_INT8);
        CASE(CL_SNORM_INT16);
        CASE(CL_UNORM_INT8);
        CASE(CL_UNORM_INT16);
        CASE(CL_UNORM_SHORT_565);
        CASE(CL_UNORM_SHORT_555);
        CASE(CL_UNORM_INT_101010);
        CASE(CL_SIGNED_INT8);
        CASE(CL_SIGNED_INT16);
        CASE(CL_SIGNED_INT32);
        CASE(CL_UNSIGNED_INT8);
        CASE(CL_UNSIGNED_INT16);
        CASE(CL_UNSIGNED_INT32);
        CASE(CL_HALF_FLOAT);
        CASE(CL_FLOAT);
        CASE(CL_UNORM_INT24);
    }
    #undef CASE
    cout << endl;
}

char *getCLErrorString(cl_int err) {
    switch (err) {
        case CL_SUCCESS:                          return (char *) "CL_SUCCESS: Success!";
        case CL_DEVICE_NOT_FOUND:                 return (char *) "CL_DEVICE_NOT_FOUND: Device not found.";
        case CL_DEVICE_NOT_AVAILABLE:             return (char *) "CL_DEVICE_NOT_AVAILABLE: Device not available";
        case CL_COMPILER_NOT_AVAILABLE:           return (char *) "CL_COMPILER_NOT_AVAILABLE: Compiler not available";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:    return (char *) "CL_MEM_OBJECT_ALLOCATION_FAILURE: Memory object allocation failure";
        case CL_OUT_OF_RESOURCES:                 return (char *) "CL_OUT_OF_RESOURCES: Out of resources";
        case CL_OUT_OF_HOST_MEMORY:               return (char *) "CL_OUT_OF_HOST_MEMORY: Out of host memory";
        case CL_PROFILING_INFO_NOT_AVAILABLE:     return (char *) "CL_PROFILING_INFO_NOT_AVAILABLE: Profiling information not available";
        case CL_MEM_COPY_OVERLAP:                 return (char *) "CL_MEM_COPY_OVERLAP: Memory copy overlap";
        case CL_IMAGE_FORMAT_MISMATCH:            return (char *) "CL_IMAGE_FORMAT_MISMATCH: Image format mismatch";
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:       return (char *) "CL_IMAGE_FORMAT_NOT_SUPPORTED: Image format not supported";
        case CL_BUILD_PROGRAM_FAILURE:            return (char *) "CL_BUILD_PROGRAM_FAILURE: Program build failure";
        case CL_MAP_FAILURE:                      return (char *) "CL_MAP_FAILURE: Map failure";
        case CL_INVALID_VALUE:                    return (char *) "CL_INVALID_VALUE: Invalid value";
        case CL_INVALID_DEVICE_TYPE:              return (char *) "CL_INVALID_DEVICE_TYPE: Invalid device type";
        case CL_INVALID_PLATFORM:                 return (char *) "CL_INVALID_PLATFORM: Invalid platform";
        case CL_INVALID_DEVICE:                   return (char *) "CL_INVALID_DEVICE: Invalid device";
        case CL_INVALID_CONTEXT:                  return (char *) "CL_INVALID_CONTEXT: Invalid context";
        case CL_INVALID_QUEUE_PROPERTIES:         return (char *) "CL_INVALID_QUEUE_PROPERTIES: Invalid queue properties";
        case CL_INVALID_COMMAND_QUEUE:            return (char *) "CL_INVALID_COMMAND_QUEUE: Invalid command queue";
        case CL_INVALID_HOST_PTR:                 return (char *) "CL_INVALID_HOST_PTR: Invalid host pointer";
        case CL_INVALID_MEM_OBJECT:               return (char *) "CL_INVALID_MEM_OBJECT: Invalid memory object";
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:  return (char *) "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR: Invalid image format descriptor";
        case CL_INVALID_IMAGE_SIZE:               return (char *) "CL_INVALID_IMAGE_SIZE: Invalid image size";
        case CL_INVALID_SAMPLER:                  return (char *) "CL_INVALID_SAMPLER: Invalid sampler";
        case CL_INVALID_BINARY:                   return (char *) "CL_INVALID_BINARY: Invalid binary";
        case CL_INVALID_BUILD_OPTIONS:            return (char *) "CL_INVALID_BUILD_OPTIONS: Invalid build options";
        case CL_INVALID_PROGRAM:                  return (char *) "CL_INVALID_PROGRAM: Invalid program";
        case CL_INVALID_PROGRAM_EXECUTABLE:       return (char *) "CL_INVALID_PROGRAM_EXECUTABLE: Invalid program executable";
        case CL_INVALID_KERNEL_NAME:              return (char *) "CL_INVALID_KERNEL_NAME: Invalid kernel name";
        case CL_INVALID_KERNEL_DEFINITION:        return (char *) "CL_INVALID_KERNEL_DEFINITION: Invalid kernel definition";
        case CL_INVALID_KERNEL:                   return (char *) "CL_INVALID_KERNEL: Invalid kernel";
        case CL_INVALID_ARG_INDEX:                return (char *) "CL_INVALID_ARG_INDEX: Invalid argument index";
        case CL_INVALID_ARG_VALUE:                return (char *) "CL_INVALID_ARG_VALUE: Invalid argument value";
        case CL_INVALID_ARG_SIZE:                 return (char *) "CL_INVALID_ARG_SIZE: Invalid argument size";
        case CL_INVALID_KERNEL_ARGS:              return (char *) "CL_INVALID_KERNEL_ARGS: Invalid kernel arguments";
        case CL_INVALID_WORK_DIMENSION:           return (char *) "CL_INVALID_WORK_DIMENSION: Invalid work dimension";
        case CL_INVALID_WORK_GROUP_SIZE:          return (char *) "CL_INVALID_WORK_GROUP_SIZE: Invalid work group size";
        case CL_INVALID_WORK_ITEM_SIZE:           return (char *) "CL_INVALID_WORK_ITEM_SIZE: Invalid work item size";
        case CL_INVALID_GLOBAL_OFFSET:            return (char *) "CL_INVALID_GLOBAL_OFFSET: Invalid global offset";
        case CL_INVALID_EVENT_WAIT_LIST:          return (char *) "CL_INVALID_EVENT_WAIT_LIST: Invalid event wait list";
        case CL_INVALID_EVENT:                    return (char *) "CL_INVALID_EVENT: Invalid event";
        case CL_INVALID_OPERATION:                return (char *) "CL_INVALID_OPERATION: Invalid operation";
        case CL_INVALID_GL_OBJECT:                return (char *) "CL_INVALID_GL_OBJECT: Invalid OpenGL object";
        case CL_INVALID_BUFFER_SIZE:              return (char *) "CL_INVALID_BUFFER_SIZE: Invalid buffer size";
        case CL_INVALID_MIP_LEVEL:                return (char *) "CL_INVALID_MIP_LEVEL: Invalid mip-map level";
        default:                                  return (char *) "Unknown";
    }
}

void clHandleError(std::string file, int line, cl_int err){
    if (err) {
        cerr << "ERROR @ " << file << "::" << line << ": " << getCLErrorString(err) << endl;
        if(err == CL_INVALID_KERNEL || err == CL_INVALID_PROGRAM_EXECUTABLE || err == CL_BUILD_PROGRAM_FAILURE){
		// Probably a build error. Show log
		string log = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device,NULL);
		cerr << log << endl;
	}
	exit(1);
    }
}

void clHandleError(cl_int err){
    clHandleError("",-1,err);
}
