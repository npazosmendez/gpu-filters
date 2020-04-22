#include <cl2.hpp>
#include "opencl-filters.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <cmath>
#include <kernels.h>

using namespace std;
using namespace cl;

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
    static int _width, _height = -1;
    static bool canny_initialized = false;

    cl_int err = 0;
    if (not canny_initialized){
        /* 1. Build PROGRAM from source, for specific context */
        createProgram("canny.cl");

        /*2. Create kernels */
        k_intensity_gauss = Kernel(program, "intensity_gauss_filter");
        k_max_edges = Kernel(program, "max_edges");
        k_hysteresis = Kernel(program, "hysteresis");
        k_torgb = Kernel(program, "floatEdges_to_RGBChar");

        // Int (Bool) for hysteresis iteration
        cl_intbool = Buffer(context, CL_MEM_USE_HOST_PTR, sizeof(int), &intbool, &err);
        clHandleError(__FILE__,__LINE__,err);

        canny_initialized = true;
    }

    if (_width != width or _height != height){
        /* 3. Buffers setup */
        // OpenCL Image / texture to store image intensity
        clImage = Image2D(context, CL_MEM_READ_WRITE, ImageFormat(CL_DEPTH, CL_FLOAT), width, height, 0, NULL, &err);
        clHandleError(__FILE__,__LINE__,err);

        // Buffer for the temp float result
        clResult_float = Buffer(context, CL_MEM_READ_WRITE, sizeof(float)*width*height, NULL, &err);
        clHandleError(__FILE__,__LINE__,err);

        // Buffer for input/output image in rgb (uchar3)
        cl_charImage = Buffer(context, CL_MEM_READ_WRITE, sizeof(char)*width*height*3, NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
    }

    return;
}

void CL_canny(char * src_c, int width, int height, float uthreshold, float lthreshold){

    if(!openCL_initialized) initCL();
    initCLCanny(width, height);

   	cl_int err = 0;
    // 1. Transfer image to GPU
	err = queue.enqueueWriteBuffer(cl_charImage, CL_TRUE, 0, 3*height*width, src_c);
	clHandleError(__FILE__,__LINE__,err);

    // 2. Run kernels
    // Compute intensity as rgb average and smooth with gaussian kernel
    k_intensity_gauss.setArg(0, cl_charImage);
    k_intensity_gauss.setArg(1, clImage);
    k_intensity_gauss.setArg(2, (cl_int)width);
    k_intensity_gauss.setArg(3, (cl_int)height);
    err = queue.enqueueNDRangeKernel(
        k_intensity_gauss,
        NullRange,
        NDRange(width, height),
        NullRange // default
    );
	clHandleError(__FILE__,__LINE__,err);

    // Compute gradient intensity and discard below-thresh and non-max edges
    k_max_edges.setArg(0, clImage);
    k_max_edges.setArg(1, clResult_float);
    k_max_edges.setArg(2, (cl_float)uthreshold);
    k_max_edges.setArg(3, (cl_float)lthreshold);
    err = queue.enqueueNDRangeKernel(
        k_max_edges,
        NullRange,
        NDRange(width, height),
        NullRange // default
    );
	clHandleError(__FILE__,__LINE__,err);

    // Hysteresis
    k_hysteresis.setArg(0, cl_intbool);
    k_hysteresis.setArg(1, clResult_float);
    k_hysteresis.setArg(2, (cl_int)width);
    k_hysteresis.setArg(3, (cl_int)height);
    int changes = 1;
    while (changes) {
        changes = 0;
        queue.enqueueWriteBuffer(cl_intbool, CL_TRUE, 0, sizeof(int), &changes);
        err = queue.enqueueNDRangeKernel(
            k_hysteresis,
            NullRange,
            NDRange(width, height),
            NullRange // default
        );
		clHandleError(__FILE__,__LINE__,err);
        err = queue.enqueueReadBuffer(cl_intbool, CL_TRUE, 0, sizeof(int), &changes);
		clHandleError(__FILE__,__LINE__,err);
        queue.finish();
    }

    // Convert float-intensity image to uchar3-rgb image
    k_torgb.setArg(0, clResult_float);
    k_torgb.setArg(1, cl_charImage);
    err = queue.enqueueNDRangeKernel(
        k_torgb,
        NullRange,
        NDRange(width, height),
        NullRange // default
    );
	clHandleError(__FILE__,__LINE__,err);

    // 3. Transfer img back to host
    queue.enqueueReadBuffer(cl_charImage, CL_TRUE, 0, 3*height*width, src_c);

    queue.finish();

    return;
}
