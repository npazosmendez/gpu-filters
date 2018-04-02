#include <cl.hpp>
#include "opencl-filters.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <cmath>

using namespace std;
using namespace cl;

Program program;
bool canny_initialized = false;

Image2D clImage;
Buffer clResult_float;
Buffer cl_charImage;
int intbool;
Buffer cl_intbool;

Kernel k_intensity_gauss;
Kernel k_max_edges;
Kernel k_hysteresis;
Kernel k_torgb;

void initCLCanny(int width, int height){
    /* 1. Build PROGRAM from source, for specific context */
    ifstream sourceFile("opencl/canny.cl");
    /* NOTE: reading the source code from another file
    during runtime makes the binary's location important.
    Not good. */
    if (!sourceFile.is_open()) {
        cerr << "Can't open CL kernel source." << endl;
        exit(1);
    }
    string sourceCode(istreambuf_iterator<char>(sourceFile), (istreambuf_iterator<char>()));
    Program::Sources sources(1, make_pair(sourceCode.c_str(), sourceCode.length()+1));
    program = Program(context, sources);
    program.build(context.getInfo<CL_CONTEXT_DEVICES>());

    /*2. Create kernels */
    k_intensity_gauss = Kernel(program, "intensity_gauss_filter");
    k_max_edges = Kernel(program, "max_edges");
    k_hysteresis = Kernel(program, "hysteresis");
    k_torgb = Kernel(program, "floatEdges_to_RGBChar");

    /* 3. Buffers setup */
    // OpenCL Image / texture to store image intensity
    cl_int err = 0;
    clImage = Image2D(context, CL_MEM_READ_WRITE, ImageFormat(CL_INTENSITY, CL_FLOAT), width, height, 0, NULL, &err);
    clHandleError(__FILE__,__LINE__,err);

    // Buffer for the temp float result
    clResult_float = Buffer(context, CL_MEM_READ_WRITE, sizeof(float)*width*height, NULL, &err);
    clHandleError(__FILE__,__LINE__,err);

    // Buffer for input/output image in rgb (uchar3)
    cl_charImage = Buffer(context, CL_MEM_READ_WRITE, sizeof(char)*width*height*3, NULL, &err);
    clHandleError(__FILE__,__LINE__,err);

    // Int (Bool) for hysteresis iteration
    cl_intbool = Buffer(context, CL_MEM_USE_HOST_PTR, sizeof(int), &intbool, &err);
    clHandleError(__FILE__,__LINE__,err);


    canny_initialized = true;
    return;
}

void CL_canny(char * src_c, int width, int height, float uthreshold, float lthreshold){

    if(!openCL_initialized) initCL();
    if(!canny_initialized) initCLCanny(width, height);

    // 1. Transfer image to GPU
    clHandleError(__FILE__,__LINE__,queue.enqueueWriteBuffer(cl_charImage, CL_TRUE, 0, 3*height*width, src_c));

    // 2. Run kernels

    // Compute intensity as rgb average and smooth with gaussian kernel
    k_intensity_gauss.setArg(0, cl_charImage);
    k_intensity_gauss.setArg(1, clImage);
    clHandleError(__FILE__,__LINE__,queue.enqueueNDRangeKernel(
        k_intensity_gauss,
        NullRange,
        NDRange(width, height),
        NullRange // default
    ));

    // Compute gradient intensity and discard below-thresh and non-max edges
    k_max_edges.setArg(0, clImage);
    k_max_edges.setArg(1, clResult_float);
    k_max_edges.setArg(2, (cl_float)uthreshold);
    k_max_edges.setArg(3, (cl_float)lthreshold);
    clHandleError(__FILE__,__LINE__,queue.enqueueNDRangeKernel(
        k_max_edges,
        NullRange,
        NDRange(width, height),
        NullRange // default
    ));

    // TODO: hysteresis
    k_hysteresis.setArg(0, cl_intbool);
    k_hysteresis.setArg(1, clResult_float);
    int iterations = 0;
    int changes = 1;
    while (changes) {
        changes = 0;
        queue.enqueueWriteBuffer(cl_intbool, CL_TRUE, 0, sizeof(int), &changes);
        clHandleError(__FILE__,__LINE__,queue.enqueueNDRangeKernel(
            k_hysteresis,
            NullRange,
            NDRange(width, height),
            NullRange // default
        ));
        queue.enqueueReadBuffer(cl_intbool, CL_TRUE, 0, sizeof(int), &changes);
        queue.finish();
        iterations++;
    }
    // cout << iterations << endl;

    // Convert float-intensity image to uchar3-rgb image
    k_torgb.setArg(0, clResult_float);
    k_torgb.setArg(1, cl_charImage);
    clHandleError(__FILE__,__LINE__,queue.enqueueNDRangeKernel(
        k_torgb,
        NullRange,
        NDRange(width, height),
        NullRange // default
    ));

    // 3. Transfer img back to host
    queue.enqueueReadBuffer(cl_charImage, CL_TRUE, 0, 3*height*width, src_c);

    queue.finish();

    return;
}
