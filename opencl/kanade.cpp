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

static float ** pyramidal_min_eigens;

static cl_float2 * flow_output;

static Kernel k_calculate_flow;
static Kernel k_convolution_x;
static Kernel k_convolution_y;
static Kernel k_convolution_blur;
static Kernel k_subsample;
static Kernel k_calculate_intensity;
static Kernel k_calculate_tensor_and_mineigen;

static Buffer * b_img_old;
static Buffer * b_img_new;

static Buffer ** b_pyramidal_intensities_old;
static Buffer ** b_pyramidal_intensities_new;

static Buffer ** b_pyramidal_blurs_old;
static Buffer ** b_pyramidal_blurs_new;

static Buffer ** b_pyramidal_gradients_x;
static Buffer ** b_pyramidal_gradients_y;

static Buffer ** b_pyramidal_tensors;
static Buffer ** b_pyramidal_min_eigens;

static Buffer ** b_pyramidal_flows;

static cl_int err = 0;



static void init(int in_width, int in_height, int levels) {

    // OpenCL initialization
    if(!openCL_initialized) initCL();

    // Program object
    createProgram("kanade.cl");

    // Kernels
    k_calculate_flow = Kernel(program, "calculate_flow");
    k_calculate_tensor_and_mineigen = Kernel(program, "calculate_tensor_and_mineigen");
    k_convolution_x = Kernel(program, "convolution_x");
    k_convolution_y = Kernel(program, "convolution_y");
    k_convolution_blur = Kernel(program, "convolution_blur");
    k_subsample = Kernel(program, "subsample");
    k_calculate_intensity = Kernel(program, "calculate_intensity");

    // Flag
    initialized = true;

    // Img buffers
    b_img_new = new Buffer(context, CL_MEM_READ_WRITE, MAX_BUFFER_SIZE * sizeof(char), NULL, &err);
    clHandleError(__FILE__,__LINE__,err);
    b_img_old = new Buffer(context, CL_MEM_READ_WRITE, MAX_BUFFER_SIZE * sizeof(char), NULL, &err);
    clHandleError(__FILE__,__LINE__,err);

    // Output buffer
    flow_output = (cl_float2 *) malloc(MAX_BUFFER_SIZE * sizeof(cl_float2));

    // Pyramid buffers
    pyramidal_widths = (int *) malloc(levels * sizeof(int));
    pyramidal_heights = (int *) malloc(levels * sizeof(int));

    pyramidal_min_eigens = (float **) malloc(levels * sizeof(float*));

    b_pyramidal_intensities_old = (Buffer **) malloc(levels * sizeof(Buffer *));
    b_pyramidal_intensities_new = (Buffer **) malloc(levels * sizeof(Buffer *));
    b_pyramidal_gradients_x = (Buffer **) malloc(levels * sizeof(Buffer *));
    b_pyramidal_gradients_y = (Buffer **) malloc(levels * sizeof(Buffer *));
    b_pyramidal_blurs_old = (Buffer **) malloc(levels * sizeof(Buffer *));
    b_pyramidal_blurs_new = (Buffer **) malloc(levels * sizeof(Buffer *));
    b_pyramidal_tensors = (Buffer **) malloc(levels * sizeof(Buffer *));
    b_pyramidal_min_eigens = (Buffer **) malloc(levels * sizeof(Buffer *));
    b_pyramidal_flows = (Buffer **) malloc(levels * sizeof(Buffer *));

    // Image buffers
    int current_width = in_width;
    int current_height = in_height;
    int current_bufsize = MAX_BUFFER_SIZE;

    forn(pi, levels) {
        pyramidal_widths[pi] = current_width;
        pyramidal_heights[pi] = current_height;

        pyramidal_min_eigens[pi] = (float *) malloc(current_bufsize * sizeof(float));

        b_pyramidal_intensities_old[pi] = new Buffer(context, CL_MEM_READ_WRITE, current_bufsize * sizeof(float), NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
        b_pyramidal_intensities_new[pi] = new Buffer(context, CL_MEM_READ_WRITE, current_bufsize * sizeof(float), NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
        b_pyramidal_gradients_x[pi] = new Buffer(context, CL_MEM_READ_WRITE, current_bufsize * sizeof(float), NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
        b_pyramidal_gradients_y[pi] = new Buffer(context, CL_MEM_READ_WRITE, current_bufsize * sizeof(float), NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
        b_pyramidal_blurs_old[pi] = new Buffer(context, CL_MEM_READ_WRITE, current_bufsize * sizeof(float), NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
        b_pyramidal_blurs_new[pi] = new Buffer(context, CL_MEM_READ_WRITE, current_bufsize * sizeof(float), NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
        b_pyramidal_tensors[pi] = new Buffer(context, CL_MEM_READ_WRITE, current_bufsize * sizeof(float[2][2]), NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
        b_pyramidal_min_eigens[pi] = new Buffer(context, CL_MEM_READ_WRITE, current_bufsize * sizeof(float), NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
        b_pyramidal_flows[pi] = new Buffer(context, CL_MEM_READ_WRITE, current_bufsize * sizeof(cl_float2), NULL, &err);
        clHandleError(__FILE__,__LINE__,err);

        current_width /= 2;
        current_height /= 2;
        current_bufsize /= 2;
    }
}

static void refresh_size(int width, int height, int levels) {
    forn(pi, levels) {
        pyramidal_widths[pi] = width;
        pyramidal_heights[pi] = height;
        width /= 2;
        height /= 2;
    }
}

static void calculate_flow(int pi, int levels) {
    // Matrices
    /*
     * A = [ sum IxIx  sum IxIy           b = [ - sum IxIt
     *       sum IyIx  sum IyIy ]               - sum IyIt ]
     */

    // Variables
    int width = pyramidal_widths[pi];
    int height = pyramidal_heights[pi];

    int previous_width = pi < levels - 1 ? pyramidal_widths[pi+1] : -1;
    int previous_height = pi < levels - 1 ? pyramidal_heights[pi+1] : -1;

    float * min_eigen = pyramidal_min_eigens[pi];

    Buffer * b_gradient_x = b_pyramidal_gradients_x[pi];
    Buffer * b_gradient_y = b_pyramidal_gradients_y[pi];
    Buffer * b_intensity_old = b_pyramidal_intensities_old[pi];
    Buffer * b_intensity_new = b_pyramidal_intensities_new[pi];
    Buffer * b_tensor = b_pyramidal_tensors[pi];
    Buffer * b_min_eigen = b_pyramidal_min_eigens[pi];
    Buffer * b_flow = b_pyramidal_flows[pi];

    Buffer * b_previous_flow = NULL;
    if (pi < levels - 1) {
        b_previous_flow = b_pyramidal_flows[pi+1];
    }

    // Tensors + Min Eigenvalue
    k_calculate_tensor_and_mineigen.setArg(0, *b_gradient_x);
    k_calculate_tensor_and_mineigen.setArg(1, *b_gradient_y);
    k_calculate_tensor_and_mineigen.setArg(2, *b_tensor);
    k_calculate_tensor_and_mineigen.setArg(3, *b_min_eigen);
    k_calculate_tensor_and_mineigen.setArg(4, *b_intensity_old);
    err = queue.enqueueNDRangeKernel(
            k_calculate_tensor_and_mineigen,
            NullRange,
            NDRange(width, height),
            NullRange // default
    );
    queue.finish();
    clHandleError(__FILE__,__LINE__,err);

    // Maximum min eigenvalue
    err = queue.enqueueReadBuffer(*b_min_eigen, CL_TRUE, 0, sizeof(float)*width*height, min_eigen);
    clHandleError(__FILE__,__LINE__,err);

    float max_min_eigen = FLT_MIN;
    forn(i, width * height) {
        max_min_eigen = max(max_min_eigen, min_eigen[i]);
    }

    // LK
    k_calculate_flow.setArg(0, width);
    k_calculate_flow.setArg(1, height);
    k_calculate_flow.setArg(2, *b_gradient_x);
    k_calculate_flow.setArg(3, *b_gradient_y);
    k_calculate_flow.setArg(4, *b_intensity_old);
    k_calculate_flow.setArg(5, *b_intensity_new);
        k_calculate_flow.setArg(6, *b_flow);
    k_calculate_flow.setArg(7, previous_width);
    k_calculate_flow.setArg(8, previous_height);
    if (b_previous_flow != NULL) {
        k_calculate_flow.setArg(9, *b_previous_flow);
    } else {
        k_calculate_flow.setArg(9, NULL);
    }
    k_calculate_flow.setArg(10, *b_tensor);
    k_calculate_flow.setArg(11, *b_min_eigen);
    k_calculate_flow.setArg(12, max_min_eigen);
    k_calculate_flow.setArg(13, pi);
    err = queue.enqueueNDRangeKernel(
            k_calculate_flow,
            NullRange,
            NDRange(width, height),
            NullRange // default
    );
    queue.finish();
    clHandleError(__FILE__,__LINE__,err);
}

void convolution(Kernel kernel, Buffer * src, Buffer * dst, int width, int height) {
    kernel.setArg(0, *src);
    kernel.setArg(1, *dst);
    err = queue.enqueueNDRangeKernel(
            kernel,
            NullRange,
            NDRange(width, height),
            NullRange // default
    );
    queue.finish();
    clHandleError(__FILE__,__LINE__,err);
}

void CL_kanade(int in_width, int in_height, char * img_old, char * img_new, vecf * output_flow, int levels) {

    if (!initialized) init(in_width, in_height, levels);
    if (in_width != pyramidal_widths[0]) refresh_size(in_width, in_height, levels);

    // Zero Flow
    forn(pi, levels) {
        int width = pyramidal_widths[pi];
        int height = pyramidal_heights[pi];

        Buffer * b_flow = b_pyramidal_flows[pi];

        err = queue.enqueueFillBuffer(*b_flow, (int)0, 0, width * height * sizeof(cl_float2));
        queue.finish();
        clHandleError(__FILE__,__LINE__,err);
    }

    // Full Image Intensity
    int full_width = pyramidal_widths[0];
    int full_height = pyramidal_heights[0];

    Buffer * b_full_intensity_old = b_pyramidal_intensities_old[0];
    Buffer * b_full_intensity_new = b_pyramidal_intensities_new[0];

    err = queue.enqueueWriteBuffer(*b_img_old, CL_TRUE, 0, 3 * full_width * full_height * sizeof(char), img_old);
    clHandleError(__FILE__,__LINE__,err);
    err = queue.enqueueWriteBuffer(*b_img_new, CL_TRUE, 0, 3 * full_width * full_height * sizeof(char), img_new);
    clHandleError(__FILE__,__LINE__,err);

    k_calculate_intensity.setArg(0, *b_img_old);
    k_calculate_intensity.setArg(1, *b_full_intensity_old);
    err = queue.enqueueNDRangeKernel(
            k_calculate_intensity,
            NullRange,
            NDRange(full_height * full_width, 1),
            NullRange // default
    );
    queue.finish();
    clHandleError(__FILE__,__LINE__,err);

    k_calculate_intensity.setArg(0, *b_img_new);
    k_calculate_intensity.setArg(1, *b_full_intensity_new);
    err = queue.enqueueNDRangeKernel(
            k_calculate_intensity,
            NullRange,
            NDRange(full_height * full_width, 1),
            NullRange // default
    );
    queue.finish();
    clHandleError(__FILE__,__LINE__,err);

    // Pyramid Construction
    forn(pi, levels - 1) {

        // Blur
        int width = pyramidal_widths[pi];
        int height = pyramidal_heights[pi];

        Buffer * b_intensity_old = b_pyramidal_intensities_old[pi];
        Buffer * b_intensity_new = b_pyramidal_intensities_new[pi];
        Buffer * b_blur_old = b_pyramidal_blurs_old[pi];
        Buffer * b_blur_new = b_pyramidal_blurs_new[pi];

        k_convolution_blur.setArg(0, *b_intensity_old);
        k_convolution_blur.setArg(1, *b_blur_old);
        err = queue.enqueueNDRangeKernel(
                k_convolution_blur,
                NullRange,
                NDRange(width, height),
                NullRange // default
        );
        queue.finish();
        clHandleError(__FILE__,__LINE__,err);

        k_convolution_blur.setArg(0, *b_intensity_new);
        k_convolution_blur.setArg(1, *b_blur_new);
        err = queue.enqueueNDRangeKernel(
                k_convolution_blur,
                NullRange,
                NDRange(width, height),
                NullRange // default
        );
        queue.finish();
        clHandleError(__FILE__,__LINE__,err);

        // Sub-sample
        int next_width = pyramidal_widths[pi+1];
        int next_height = pyramidal_heights[pi+1];

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
        queue.finish();
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
        queue.finish();
        clHandleError(__FILE__,__LINE__,err);
    }

    for (int pi = levels - 1; pi >= 0; pi--) {

        // Gradients
        int width = pyramidal_widths[pi];
        int height = pyramidal_heights[pi];

        Buffer * b_gradient_x = b_pyramidal_gradients_x[pi];
        Buffer * b_gradient_y = b_pyramidal_gradients_y[pi];
        Buffer * b_intensity_old = b_pyramidal_intensities_old[pi];

        k_convolution_x.setArg(0, *b_intensity_old);
        k_convolution_x.setArg(1, *b_gradient_x);
        err = queue.enqueueNDRangeKernel(
                k_convolution_x,
                NullRange,
                NDRange(width, height),
                NullRange // default
        );
        queue.finish();
        clHandleError(__FILE__,__LINE__,err);

        k_convolution_y.setArg(0, *b_intensity_old);
        k_convolution_y.setArg(1, *b_gradient_y);
        err = queue.enqueueNDRangeKernel(
                k_convolution_y,
                NullRange,
                NDRange(width, height),
                NullRange // default
        );
        queue.finish();
        clHandleError(__FILE__,__LINE__,err);

        // LK algorithm
        calculate_flow(pi, levels);
    }

    // Output
    err = queue.enqueueReadBuffer(*b_pyramidal_flows[0], CL_TRUE, 0, sizeof(cl_float2)*full_width*full_height, flow_output);
    clHandleError(__FILE__,__LINE__,err);

    forn(i, full_width * full_height) {
        output_flow[i] = (vecf) { flow_output[i].x, flow_output[i].y };
    }
}