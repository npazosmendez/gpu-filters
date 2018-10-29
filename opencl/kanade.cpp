//
// Created by Daro on 24/09/2018.
//

#include <cl2.hpp>
#include <iostream>
#include "stdio.h"
#include "stdbool.h"
#include "string.h"
#include "math.h"
#include "time.h"
#include "float.h"
#include <stdint.h>
#include "assert.h"
#include <chrono>
#include <iomanip>
extern "C" {
    #include "../c/cutils.h"
    #include "../c/c-filters.h"
}
#include "opencl-filters.hpp"

using namespace std;
using namespace std::chrono;
using namespace cl;


#define LINEAR(x, y) (y)*width+(x)
#define forn(i,n) for(int i=0; i<(n); i++)


static int SOBEL_KERNEL_RADIUS = 1;


#define SOBEL_KERNEL_DIAMETER (2*SOBEL_KERNEL_RADIUS+1)

static float KERNEL_GAUSSIAN_BLUR[] = {
        1/16.0, 1/8.0 , 1/16.0,
        1/8.0 , 1/4.0 , 1/8.0 ,
        1/16.0, 1/8.0 , 1/16.0
};

static bool initialized = false;

static int * pyramidal_widths;
static int * pyramidal_heights;

static float ** pyramidal_intensities_old;
static float ** pyramidal_intensities_new;

static float ** pyramidal_blurs_old;
static float ** pyramidal_blurs_new;

static vecf ** pyramidal_flows;


static float * debug_gradient_x;
static float * debug_gradient_y;


// +++++++
// CL CODE
static Kernel k_calculate_flow;
static Kernel k_convolution_x;
static Kernel k_convolution_y;
static Kernel k_convolution_blur;
static Kernel k_subsample;

static Buffer ** b_pyramidal_intensities_old;
static Buffer ** b_pyramidal_intensities_new;

static Buffer ** b_pyramidal_blurs_old;
static Buffer ** b_pyramidal_blurs_new;

static Buffer ** b_pyramidal_gradients_x;
static Buffer ** b_pyramidal_gradients_y;

static Buffer ** b_pyramidal_flows;

static cl_int err = 0;
// CL CODE
// +++++++






static void init(int in_width, int in_height, int levels) {

    // +++++++
    // CL CODE
    cout << "Initializing OpenCL model for Optical Flow\n";

    // OpenCL initialization
    if(!openCL_initialized) initCL();

    // Program object
    createProgram("kanade.cl");

    // Kernels
    k_calculate_flow = Kernel(program, "calculate_flow");
    k_convolution_x = Kernel(program, "convolution_x");
    k_convolution_y = Kernel(program, "convolution_y");
    k_convolution_blur = Kernel(program, "convolution_blur");
    k_subsample = Kernel(program, "subsample");
    // CL CODE
    // +++++++


    // Flag
    initialized = true;

    // Pyramid buffers
    pyramidal_widths = (int *) malloc(levels * sizeof(int));
    pyramidal_heights = (int *) malloc(levels * sizeof(int));
    pyramidal_intensities_old = (float **) malloc(levels * sizeof(float *));
    pyramidal_intensities_new = (float **) malloc(levels * sizeof(float *));
    pyramidal_blurs_old = (float **) malloc(levels * sizeof(float *));
    pyramidal_blurs_new = (float **) malloc(levels * sizeof(float *));
    pyramidal_flows = (vecf **) malloc(levels * sizeof(vecf *));



    // +++++++
    // CL CODE
    b_pyramidal_intensities_old = (Buffer **) malloc(levels * sizeof(Buffer *));
    b_pyramidal_intensities_new = (Buffer **) malloc(levels * sizeof(Buffer *));
    b_pyramidal_gradients_x = (Buffer **) malloc(levels * sizeof(Buffer *));
    b_pyramidal_gradients_y = (Buffer **) malloc(levels * sizeof(Buffer *));
    b_pyramidal_blurs_old = (Buffer **) malloc(levels * sizeof(Buffer *));
    b_pyramidal_blurs_new = (Buffer **) malloc(levels * sizeof(Buffer *));
    b_pyramidal_flows = (Buffer **) malloc(levels * sizeof(Buffer *));
    // CL CODE
    // +++++++


    debug_gradient_x = (float *) malloc(in_width * in_height * sizeof(float));
    debug_gradient_y = (float *) malloc(in_width * in_height * sizeof(float));


    // Image buffers
    int current_width = in_width;
    int current_height = in_height;

    forn(pi, levels) {
        pyramidal_widths[pi] = current_width;
        pyramidal_heights[pi] = current_height;
        pyramidal_intensities_old[pi] = (float *) malloc(current_width * current_height * sizeof(float));
        pyramidal_intensities_new[pi] = (float *) malloc(current_width * current_height * sizeof(float));
        pyramidal_blurs_old[pi] = (float *) malloc(current_width * current_height * sizeof(float));
        pyramidal_blurs_new[pi] = (float *) malloc(current_width * current_height * sizeof(float));
        pyramidal_flows[pi] = (vecf *) malloc(current_width * current_height * sizeof(vecf));

        // +++++++
        // CL CODE
        b_pyramidal_intensities_old[pi] = new Buffer(context, CL_MEM_READ_WRITE, current_width * current_height * sizeof(float), NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
        b_pyramidal_intensities_new[pi] = new Buffer(context, CL_MEM_READ_WRITE, current_width * current_height * sizeof(float), NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
        b_pyramidal_gradients_x[pi] = new Buffer(context, CL_MEM_READ_WRITE, current_width * current_height * sizeof(float), NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
        b_pyramidal_gradients_y[pi] = new Buffer(context, CL_MEM_READ_WRITE, current_width * current_height * sizeof(float), NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
        b_pyramidal_blurs_old[pi] = new Buffer(context, CL_MEM_READ_WRITE, current_width * current_height * sizeof(float), NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
        b_pyramidal_blurs_new[pi] = new Buffer(context, CL_MEM_READ_WRITE, current_width * current_height * sizeof(float), NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
        b_pyramidal_flows[pi] = new Buffer(context, CL_MEM_READ_WRITE, current_width * current_height * sizeof(vecf), NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
        // CL CODE
        // +++++++

        current_width /= 2;
        current_height /= 2;
    }
}

static void finish() {
    // TODO free buffers
}

static void calculate_flow(int pi, int levels) {
    // Matrices
    /*
     * A = [ sum IxIx  sum IxIy           b = [ - sum IxIt
     *       sum IyIx  sum IyIy ]               - sum IyIt ]
     */

    int width = pyramidal_widths[pi];
    int height = pyramidal_heights[pi];
    float * intensity_old = pyramidal_intensities_old[pi];
    float * intensity_new = pyramidal_intensities_new[pi];
    vecf * flow = pyramidal_flows[pi];

    vecf * previous_flow = pi < levels - 1 ? pyramidal_flows[pi+1] : NULL;
    int previous_width = pi < levels - 1 ? pyramidal_widths[pi+1] : -1;
    int previous_height = pi < levels - 1 ? pyramidal_heights[pi+1] : -1;

    // +++++++
    // CL CODE
    Buffer * b_gradient_x = b_pyramidal_gradients_x[pi];
    Buffer * b_gradient_y = b_pyramidal_gradients_y[pi];
    Buffer * b_intensity_old = b_pyramidal_intensities_old[pi];
    Buffer * b_intensity_new = b_pyramidal_intensities_new[pi];
    Buffer * b_flow = b_pyramidal_flows[pi];

    err = queue.enqueueWriteBuffer(*b_intensity_old, CL_TRUE, 0, sizeof(float)*width*height, intensity_old);
        clHandleError(__FILE__,__LINE__,err);
    err = queue.enqueueWriteBuffer(*b_intensity_new, CL_TRUE, 0, sizeof(float)*width*height, intensity_new);
        clHandleError(__FILE__,__LINE__,err);
    err = queue.enqueueWriteBuffer(*b_flow, CL_TRUE, 0, sizeof(vecf)*width*height, flow);
        clHandleError(__FILE__,__LINE__,err);

    Buffer * b_previous_flow = NULL;
    if (pi < levels - 1) {
        b_previous_flow = b_pyramidal_flows[pi+1];
        err = queue.enqueueWriteBuffer(*b_previous_flow, CL_TRUE, 0, sizeof(vecf)*previous_width*previous_height, previous_flow);
            clHandleError(__FILE__,__LINE__,err);
    }

    k_calculate_flow.setArg(0, width);
    k_calculate_flow.setArg(1, height);
    k_calculate_flow.setArg(2, *b_gradient_x);
    k_calculate_flow.setArg(3, *b_gradient_y);
    k_calculate_flow.setArg(4, *b_intensity_old);
    k_calculate_flow.setArg(5, *b_intensity_new);
    k_calculate_flow.setArg(6, *b_flow);
    k_calculate_flow.setArg(7, previous_width);
    k_calculate_flow.setArg(8, previous_height);
    k_calculate_flow.setArg(9, *b_previous_flow);

    err = queue.enqueueNDRangeKernel(
            k_calculate_flow,
            NullRange,
            NDRange(width, height),
            NullRange // default
    );
    clHandleError(__FILE__,__LINE__,err);

    err = queue.enqueueReadBuffer(*b_flow, CL_TRUE, 0, sizeof(vecf)*width*height, flow);
        clHandleError(__FILE__,__LINE__,err);
}

void CL_kanade(int in_width, int in_height, char * img_old, char * img_new, vec * output_flow, int levels) {

    if (!initialized) init(in_width, in_height, levels);

    // Zero Flow
    forn(pi, levels) {
        int width = pyramidal_widths[pi];
        int height = pyramidal_heights[pi];
        vecf * flow = pyramidal_flows[pi];

        memset(flow, 0, width * height * sizeof(vecf));
    }


    // Full Image Intensity
    int full_width = pyramidal_widths[0];
    int full_height = pyramidal_heights[0];
    float * full_intensity_old = pyramidal_intensities_old[0];
    float * full_intensity_new = pyramidal_intensities_new[0];

    // Kernel 3
    // Profile first to see if it's worth it

    forn(i, full_height * full_width) {
        unsigned char (*img_oldRGB)[3] = (unsigned char (*)[3])img_old;
        full_intensity_old[i] = (img_oldRGB[i][0] + img_oldRGB[i][1] + img_oldRGB[i][2]) / 3;
    }

    forn(i, full_height * full_width) {
        unsigned char (*img_newRGB)[3] = (unsigned char (*)[3])img_new;
        full_intensity_new[i] = (img_newRGB[i][0] + img_newRGB[i][1] + img_newRGB[i][2]) / 3;
    }


    // Pyramid Construction
    forn(pi, levels - 1) {
        // Sub-image
        int width = pyramidal_widths[pi];
        int height = pyramidal_heights[pi];
        float * intensity_old = pyramidal_intensities_old[pi];
        float * intensity_new = pyramidal_intensities_new[pi];
        float * blur_old = pyramidal_blurs_old[pi];
        float * blur_new = pyramidal_blurs_new[pi];

        // Gaussian blur
        convoluion2D(intensity_old, width, height, KERNEL_GAUSSIAN_BLUR, 3, blur_old);
        convoluion2D(intensity_new, width, height, KERNEL_GAUSSIAN_BLUR, 3, blur_new);

        // CL CODE
        // +++++++
        Buffer * b_intensity_old = b_pyramidal_intensities_old[pi];
        Buffer * b_intensity_new = b_pyramidal_intensities_new[pi];
        Buffer * b_blur_old = b_pyramidal_blurs_old[pi];
        Buffer * b_blur_new = b_pyramidal_blurs_new[pi];

        err = queue.enqueueWriteBuffer(*b_intensity_old, CL_TRUE, 0, sizeof(float)*width*height, intensity_old);
        clHandleError(__FILE__,__LINE__,err);
        err = queue.enqueueWriteBuffer(*b_intensity_new, CL_TRUE, 0, sizeof(float)*width*height, intensity_new);
        clHandleError(__FILE__,__LINE__,err);

        k_convolution_blur.setArg(0, *b_intensity_old);
        k_convolution_blur.setArg(1, *b_blur_old);
        err = queue.enqueueNDRangeKernel(
                k_convolution_blur,
                NullRange,
                NDRange(width, height),
                NullRange // default
        );
        clHandleError(__FILE__,__LINE__,err);

        k_convolution_blur.setArg(0, *b_intensity_new);
        k_convolution_blur.setArg(1, *b_blur_new);
        err = queue.enqueueNDRangeKernel(
                k_convolution_blur,
                NullRange,
                NDRange(width, height),
                NullRange // default
        );
        clHandleError(__FILE__,__LINE__,err);

        err = queue.enqueueReadBuffer(*b_blur_new, CL_TRUE, 0, sizeof(float)*width*height, blur_new);
        err = queue.enqueueReadBuffer(*b_blur_old, CL_TRUE, 0, sizeof(float)*width*height, blur_old);

        // +++++++
        // CL CODE

        // Kernel 1: Convolution Gauss (width, height, intensity_old, intensity_new)
        // Kernel 2: Subsample (previous_width, previous_height, previous_buffer,  next_width, next_height, next_buffer)
        //   Alternative plus: Merge kernels together (sobel kernels, subsample kernels and convolution kernels)

        // Sub-sample

        int next_width = pyramidal_widths[pi+1];
        int next_height = pyramidal_heights[pi+1];
        float * next_intensity_old = pyramidal_intensities_old[pi+1];
        float * next_intensity_new = pyramidal_intensities_new[pi+1];

        // CL CODE
        // +++++++
        err = queue.enqueueWriteBuffer(*b_blur_new, CL_TRUE, 0, sizeof(float)*width*height, blur_new);
        clHandleError(__FILE__,__LINE__,err);
        err = queue.enqueueWriteBuffer(*b_blur_old, CL_TRUE, 0, sizeof(float)*width*height, blur_old);
        clHandleError(__FILE__,__LINE__,err);

        Buffer * b_next_intensity_old = b_pyramidal_intensities_old[pi+1];
        Buffer * b_next_intensity_new = b_pyramidal_intensities_new[pi+1];

        k_subsample.setArg(0, width);
        k_subsample.setArg(1, height);
        k_subsample.setArg(2, *b_blur_old);
        k_subsample.setArg(3, *b_next_intensity_old);
        err = queue.enqueueNDRangeKernel(
                k_subsample,
                NullRange,
                NDRange(next_width, next_height),
                NullRange // default
        );
        clHandleError(__FILE__,__LINE__,err);

        k_subsample.setArg(0, width);
        k_subsample.setArg(1, height);
        k_subsample.setArg(2, *b_blur_new);
        k_subsample.setArg(3, *b_next_intensity_new);
        err = queue.enqueueNDRangeKernel(
                k_subsample,
                NullRange,
                NDRange(next_width, next_height),
                NullRange // default
        );
        clHandleError(__FILE__,__LINE__,err);

        err = queue.enqueueReadBuffer(*b_next_intensity_new, CL_TRUE, 0, sizeof(float)*next_width*next_height, next_intensity_new);
        err = queue.enqueueReadBuffer(*b_next_intensity_old, CL_TRUE, 0, sizeof(float)*next_width*next_height, next_intensity_old);

        clHandleError(__FILE__,__LINE__,err);
        // +++++++
        // CL CODE
    }

    for (int pi = levels - 1; pi >= 0; pi--) {
        // Sub-image
        int width = pyramidal_widths[pi];
        int height = pyramidal_heights[pi];
        float * intensity_old = pyramidal_intensities_old[pi];

        Buffer * b_gradient_x = b_pyramidal_gradients_x[pi];
        Buffer * b_gradient_y = b_pyramidal_gradients_y[pi];
        Buffer * b_intensity_old = b_pyramidal_intensities_old[pi];

        err = queue.enqueueWriteBuffer(*b_intensity_old, CL_TRUE, 0, sizeof(float)*width*height, intensity_old);
        clHandleError(__FILE__,__LINE__,err);

        // +++++++
        // CL CODE
        k_convolution_x.setArg(0, *b_intensity_old);
        k_convolution_x.setArg(1, *b_gradient_x);
        err = queue.enqueueNDRangeKernel(
                k_convolution_x,
                NullRange,
                NDRange(width, height),
                NullRange // default
        );
        clHandleError(__FILE__,__LINE__,err);

        k_convolution_y.setArg(0, *b_intensity_old);
        k_convolution_y.setArg(1, *b_gradient_y);
        err = queue.enqueueNDRangeKernel(
                k_convolution_y,
                NullRange,
                NDRange(width, height),
                NullRange // default
        );
        clHandleError(__FILE__,__LINE__,err);

        // CL CODE
        // +++++++

        // LK algorithm (+ corner detection)
        calculate_flow(pi, levels);
    }

    // Output
    vecf * full_flow = pyramidal_flows[0];

    forn(i, full_width * full_height) {
        output_flow[i] = (vec) { (int) full_flow[i].x, (int) full_flow[i].y };
    }
}