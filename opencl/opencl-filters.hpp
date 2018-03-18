#ifndef OPENCLFILTERS_H
#define OPENCLFILTERS_H


/* Image filters */

void CL_convoluion2D(float* src, int width, int height, float * kernel_img, int ksize, float* dst);

void CL_canny(char * src, int width, int height, float uthreshold, float lthreshold);

/* OpenCL setup and config */

#include <CL/cl.hpp>
extern cl::Platform platform;
extern cl::Device device;
extern cl::Context context;
extern cl::CommandQueue queue;
extern bool openCL_initialized;

void initCL();
char *getCLErrorString(cl_int err);
void printImageFormat(cl::ImageFormat format);

#endif
