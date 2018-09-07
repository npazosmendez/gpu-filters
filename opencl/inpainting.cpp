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
extern "C" {
    #include "../c/cutils.h"
    #include "../c/c-filters.h"
}
#include "opencl-filters.hpp"

using namespace std;
using namespace cl;

static int PATCH_RADIUS = 4;

/*
    Algorithm outline
     - Calculate image border
     - Calculate priority for all pixels in the border
     - Find pixel with most priority
     - Find patch most similar with the patch of the previous pixel
     - Copy patches
     - Loop until image is filled
*/


// GLOBALS
// +++++++

// Macros
#define LINEAR3(y,x,z) (3*((y)*width+(x))+(z))
#define LINEAR(y,x) ((y)*width+(x))
#define forn(i,n) for(int i=0; i<(n); i++)

// Host Buffers
#define MAX_LEN 2000
char contour_mask[MAX_LEN*MAX_LEN]; // whether a pixel belongs to the border
cl_float confidence[MAX_LEN*MAX_LEN];  // how trustworthy the info of a pixel is
cl_float priority[MAX_LEN*MAX_LEN];    // patch priority as next target
cl_float diffs[MAX_LEN*MAX_LEN];    // differences of patch to target

// Device Memory Objects
Kernel k_contour;
Kernel k_patch_priorities;
Kernel k_target_diffs;
Buffer b_img;
Buffer b_mask;
Buffer b_contour_mask;
Buffer b_confidence;
Buffer b_diffs;
Buffer b_priorities;

// Misc
static cl_int err = 0;


// ALGORITHM
// +++++++++

void CL_inpaint_init(int width, int height, char * img, bool * mask, int * debug) {

    cout << "Initializing OpenCL model for Inpainting\n";

    // Buffer Initialization
    memset(confidence, 0, MAX_LEN*MAX_LEN*sizeof(float));
    memset(contour_mask, 0, MAX_LEN*MAX_LEN*sizeof(bool));
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            confidence[LINEAR(i,j)] = mask[LINEAR(i,j)] ? 0.0 : 1.0;
        }
    }

    // OpenCL initialization
    if(!openCL_initialized) initCL();

    // Program object
    createProgram("inpainting.cl");

    // Kernels
    k_contour = Kernel(program, "contour");
    k_patch_priorities = Kernel(program, "patch_priorities");
    k_target_diffs = Kernel(program, "target_diffs");

    // Buffers
    b_img = Buffer(context, CL_MEM_READ_WRITE, sizeof(char)*width*height*3, NULL, &err); // image in rgb (uchar3)
        clHandleError(__FILE__,__LINE__,err);
    b_mask = Buffer(context, CL_MEM_READ_WRITE, sizeof(char)*width*height, NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
    b_contour_mask = Buffer(context, CL_MEM_READ_WRITE, sizeof(char)*width*height, NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
    b_confidence = Buffer(context, CL_MEM_READ_WRITE, sizeof(cl_float)*width*height, NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
    b_priorities = Buffer(context, CL_MEM_READ_WRITE, sizeof(cl_float)*width*height, NULL, &err);
        clHandleError(__FILE__,__LINE__,err);
    b_diffs = Buffer(context, CL_MEM_READ_WRITE, sizeof(cl_float)*width*height, NULL, &err); // differences to target
        clHandleError(__FILE__,__LINE__,err);
}


bool CL_inpaint_step(int width, int height, char * img, bool * mask, int * debug) {

#ifdef DEBUG
    memset(debug, 0, width*height*sizeof(int));
#endif

    // 0. WRITE
    // ++++++++

    err = queue.enqueueWriteBuffer(b_mask, CL_TRUE, 0, sizeof(char)*width*height, mask);
    clHandleError(__FILE__,__LINE__,err);

    err = queue.enqueueWriteBuffer(b_img, CL_TRUE, 0, sizeof(char)*width*height*3, img);
    clHandleError(__FILE__,__LINE__,err);

    // 1. CALCULATE CONTOUR
    // ++++++++++++++++++++
#ifdef PROFILE
    tstart = clock();
#endif

    // Buffer clear
    memset(contour_mask, 0, MAX_LEN*MAX_LEN*sizeof(bool));

    // Write to Device
    err = queue.enqueueWriteBuffer(b_contour_mask, CL_TRUE, 0, sizeof(char)*width*height, contour_mask);
    clHandleError(__FILE__,__LINE__,err);

    // Kernel execute
    k_contour.setArg(0, b_mask);
    k_contour.setArg(1, b_contour_mask);
    err = queue.enqueueNDRangeKernel(
            k_contour,
            NullRange,
            NDRange(width, height),
            NullRange // default
    );
    clHandleError(__FILE__,__LINE__,err);

    // Read result
    err = queue.enqueueReadBuffer(b_contour_mask, CL_TRUE, 0, sizeof(char)*width*height, contour_mask);
    clHandleError(__FILE__,__LINE__,err);

    // Exit condition
    int contour_size = 0;
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++) {
        if (contour_mask[LINEAR(i,j)]) contour_size++;
    }

    if (contour_size == 0) {
        return 0;
    }

#ifdef PROFILE
    tend = clock();
    tcount = (float)(tend - tstart) / CLOCKS_PER_SEC;
    printf("Contour (size = %d): %f\n", contour_size, tcount);
#endif

    // 2. FIND TARGET PATCH
    // ++++++++++++++++++++
#ifdef PROFILE
    tstart = clock();
#endif

    // Buffer clear
    memset(priority, -1, MAX_LEN*MAX_LEN*sizeof(cl_float));

    // Write to Device
    err = queue.enqueueWriteBuffer(b_contour_mask, CL_TRUE, 0, sizeof(char)*width*height, contour_mask);
    clHandleError(__FILE__,__LINE__,err);

    err = queue.enqueueWriteBuffer(b_confidence, CL_TRUE, 0, sizeof(cl_float)*width*height, confidence);
    clHandleError(__FILE__,__LINE__,err);

    // Kernel Execute
    k_patch_priorities.setArg(0, b_img);
    k_patch_priorities.setArg(1, b_mask);
    k_patch_priorities.setArg(2, b_contour_mask);
    k_patch_priorities.setArg(3, b_confidence);
    k_patch_priorities.setArg(4, b_priorities);
    err = queue.enqueueNDRangeKernel(
            k_patch_priorities,
            NullRange,
            NDRange(width, height),
            NullRange // default
    );
    clHandleError(__FILE__,__LINE__,err);

    // Read result
    err = queue.enqueueReadBuffer(b_priorities, CL_TRUE, 0, sizeof(cl_float)*width*height, priority);
    clHandleError(__FILE__,__LINE__,err);

    err = queue.enqueueReadBuffer(b_confidence, CL_TRUE, 0, sizeof(cl_float)*width*height, confidence);
    clHandleError(__FILE__,__LINE__,err);

    // Reduce
    int max_i = -1;
    int max_j = -1;
    float max_priority = -1.0;

    forn(x, width) forn(y, height) {
        if (priority[LINEAR(y,x)] > max_priority) {
            max_priority = priority[LINEAR(y,x)];
            max_i = y;
            max_j = x;
        }
    }

#ifdef PROFILE
    tend = clock();
    tcount = (float)(tend - tstart) / CLOCKS_PER_SEC;
    printf("Target patch (%d, %d): %f\n", max_i, max_j, tcount);
#endif

    // 3. FIND SOURCE PATCH
    // ++++++++++++++++++++
#ifdef PROFILE
    tstart = clock();
#endif

    // Write to Device
    //err = queue.enqueueWriteBuffer(b_mask, CL_TRUE, 0, sizeof(char)*width*height, mask);
    //clHandleError(__FILE__,__LINE__,err);

    cl_int2 best_target = { max_j, max_i };

    // Kernel Execute
    k_target_diffs.setArg(0, b_img);
    k_target_diffs.setArg(1, b_mask);
    k_target_diffs.setArg(2, best_target);
    k_target_diffs.setArg(3, b_diffs);
    err = queue.enqueueNDRangeKernel(
        k_target_diffs,
        NullRange,
        NDRange(width, height),
        NullRange // default
    );
	clHandleError(__FILE__,__LINE__,err);

    // Read result
    err = queue.enqueueReadBuffer(b_diffs, CL_TRUE, 0, sizeof(cl_float)*width*height, diffs);
	clHandleError(__FILE__,__LINE__,err);

	// Reduce
    float cl_min_diff = FLT_MAX;
    int cl_min_source_i = -1;
    int cl_min_source_j = -1;
    forn(x, width) forn(y, height) {
        if (float(diffs[LINEAR(y,x)]) < cl_min_diff) {
            cl_min_diff = diffs[LINEAR(y,x)];
            cl_min_source_i = y;
            cl_min_source_j = x;
        }
    }

#ifdef PROFILE
    tend = clock();
    tcount = (float)(tend - tstart) / CLOCKS_PER_SEC;
    printf("Source patch(%d, %d): %f\n", cl_min_source_i, cl_min_source_j, tcount);
#endif

#ifdef DEBUG
    debug[LINEAR(max_i, max_j)] = 2; // RED - Target
    debug[LINEAR(cl_min_source_i, cl_min_source_j)] = 1; // BLUE - Source
#endif

    // 4. COPY
    // +++++++
#ifdef PROFILE
    tstart = clock();
#endif

    for (int ki = -PATCH_RADIUS; ki <= PATCH_RADIUS; ki++) {
        for (int kj = -PATCH_RADIUS; kj <= PATCH_RADIUS; kj++) {
            int target_i = max_i + ki;
            int target_j = max_j + kj;
            int source_i = cl_min_source_i + ki;
            int source_j = cl_min_source_j + kj;

            if (within(target_i, 0, height) &&  \
                within(target_j, 0, width) &&   \
                mask[LINEAR(target_i, target_j)]) {
                img[LINEAR3(target_i, target_j, 0)] = img[LINEAR3(source_i, source_j, 0)];
                img[LINEAR3(target_i, target_j, 1)] = img[LINEAR3(source_i, source_j, 1)];
                img[LINEAR3(target_i, target_j, 2)] = img[LINEAR3(source_i, source_j, 2)];
                mask[LINEAR(target_i, target_j)] = false;
            }
        }
    }

#ifdef PROFILE
    tend = clock();
    tcount = (float)(tend - tstart) / CLOCKS_PER_SEC;
    printf("Copy: %f\n", tcount);
    printf("\n");
#endif

    return 1;
}


void CL_inpainting(char * ptr, int width, int height, bool * mask_ptr, int * debug = nullptr) {
    CL_inpaint_init(width, height, ptr, mask_ptr, debug);

    while (true) {
        bool is_more = CL_inpaint_step(width, height, ptr, mask_ptr, debug);
        if (!is_more) break;
    }
}

