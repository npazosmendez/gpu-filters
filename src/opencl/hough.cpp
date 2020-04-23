
#include <cl2.hpp>
#include "opencl-filters.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <cmath>
extern "C" {
    #include <c-filters.h>
}
using namespace std;
using namespace cl;

Buffer cl_hough_counter;
Buffer cl_image_counter;
Buffer cl_charImage_;
Buffer cl_max;

Kernel k_count_edges;
Kernel k_draw_counter;
Kernel k_find_peaks;

#define MAX_WIDTH 2000
#define MAX_HEIGHT 1200
int threshold = 0;

void initCLHough(int width, int height, int a_ammount, int p_ammount){
    static int _width = -1, _height = -1;
    static int _a_ammount = -1, _p_ammount = -1;
    static bool CL_hough_initialized = false;

    if (not CL_hough_initialized){
        // TODO: use utils for this
        /* 1. Build PROGRAM from source, for specific context z*/
        ifstream sourceFile("opencl/hough.cl");
        /* NOTE: reading the source code from another file
        during runtime makes the binary's location important.
        Not good. */
        if (!sourceFile.is_open()) {
            cerr << "Can't open CL kernel source." << endl;
            exit(1);
        }
        string sourceCode(istreambuf_iterator<char>(sourceFile), (istreambuf_iterator<char>()));
        Program::Sources sources(1, sourceCode);
        program = Program(context, sources);
        program.build(context.getInfo<CL_CONTEXT_DEVICES>());

        /*2. Create kernels */
        k_count_edges = Kernel(program, "edges_counter");
        k_draw_counter = Kernel(program, "draw_counter");
        k_find_peaks = Kernel(program, "find_peaks");

        CL_hough_initialized = true;
    }


    if (_width != width or _height != height or
        _a_ammount != a_ammount or _p_ammount != p_ammount){
        /* 3. Buffers setup */
        // OpenCL Image / texture to store image intensity
        cl_int err = 0;
        cl_hough_counter = Buffer(context, CL_MEM_READ_WRITE, sizeof(int)*a_ammount*p_ammount, NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
        cl_image_counter = Buffer(context, CL_MEM_READ_WRITE, sizeof(char)*3*a_ammount*p_ammount, NULL, &err);
        clHandleError(__FILE__,__LINE__,err);

        cl_charImage_ = Buffer(context, CL_MEM_READ_WRITE, sizeof(char)*width*height*3, NULL, &err);
        clHandleError(__FILE__,__LINE__,err);

        cl_max = Buffer(context, CL_MEM_USE_HOST_PTR, sizeof(int), &threshold, &err);
        clHandleError(__FILE__,__LINE__,err);
    }

    return;
}

void CL_hough(char * src, int width, int height, int a_ammount, int p_ammount, char* counter){

    float uthreshold = 150;
    float lthreshold = 100;
    char canny_edges[MAX_WIDTH * MAX_HEIGHT * 3];
    memcpy(canny_edges, src, 3 * sizeof(char) * width * height);
    CL_canny(canny_edges, width, height, uthreshold, lthreshold);

    if(!openCL_initialized) initCL();
    initCLHough(width, height, a_ammount, p_ammount);


    cl_int err = 0;
    // 1. Transfer image to GPU
    err = queue.enqueueWriteBuffer(cl_charImage_, CL_TRUE, 0, 3*height*width, canny_edges);
    clHandleError(__FILE__,__LINE__,err);

    err = queue.enqueueFillBuffer(cl_hough_counter, (int)0, 0, a_ammount*p_ammount*sizeof(int));
    clHandleError(__FILE__,__LINE__,err);

    err = queue.enqueueFillBuffer(cl_max, (int)0, 0, sizeof(int));
    clHandleError(__FILE__,__LINE__,err);

    // 2. Run kernels

    /* count edges using hough transform */
    k_count_edges.setArg(0, cl_charImage_);
    k_count_edges.setArg(1, cl_hough_counter);
    k_count_edges.setArg(2, (cl_int)a_ammount);
    k_count_edges.setArg(3, (cl_int)p_ammount);
    k_count_edges.setArg(4, cl_max);
    err = queue.enqueueNDRangeKernel(
        k_count_edges,
        NullRange,
        NDRange(width, height),
        NullRange // default
    );
    clHandleError(__FILE__,__LINE__,err);

    err = queue.enqueueReadBuffer(cl_max, CL_TRUE, 0, sizeof(int), &threshold);
    clHandleError(__FILE__,__LINE__,err);

    threshold = (int)(threshold * 0.5);

    /* draw counter in buffer */
    if (counter != NULL){
        k_draw_counter.setArg(0, cl_hough_counter);
        k_draw_counter.setArg(1, cl_image_counter);
        err = queue.enqueueNDRangeKernel(
            k_draw_counter,
            NullRange,
            NDRange(a_ammount, p_ammount),
            NullRange // default
        );
        clHandleError(__FILE__,__LINE__,err);
    }


    err = queue.enqueueWriteBuffer(cl_charImage_, CL_TRUE, 0, 3*height*width, src);
    clHandleError(__FILE__,__LINE__,err);
    /* find peakds in counter */
    k_find_peaks.setArg(0, cl_hough_counter);
    k_find_peaks.setArg(1, cl_image_counter);
    k_find_peaks.setArg(2, (cl_int)a_ammount);
    k_find_peaks.setArg(3, (cl_int)p_ammount);   
    k_find_peaks.setArg(4, cl_charImage_);   
    k_find_peaks.setArg(5, (cl_int)width);   
    k_find_peaks.setArg(6, (cl_int)height);   
    k_find_peaks.setArg(7, (cl_int)threshold);
    err = queue.enqueueNDRangeKernel(
        k_find_peaks,
        NullRange,
        NDRange(a_ammount, p_ammount),
        NullRange // default
    );
    clHandleError(__FILE__,__LINE__,err);

    // 3. Transfer img back to host
    err = queue.enqueueReadBuffer(cl_charImage_, CL_TRUE, 0, 3*height*width, src);
    clHandleError(__FILE__,__LINE__,err);
    if (counter != NULL){
        err = queue.enqueueReadBuffer(cl_image_counter, CL_TRUE, 0, 3*p_ammount*a_ammount, counter);
        clHandleError(__FILE__,__LINE__,err);
    }

    queue.finish();

    return;
}
