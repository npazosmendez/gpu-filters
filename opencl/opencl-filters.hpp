#ifndef OPENCLFILTERS_H
#define OPENCLFILTERS_H

#include <string>

/* Image filters */

void CL_convoluion2D(float* src, int width, int height, float * kernel_img, int ksize, float* dst);

void CL_canny(char * src, int width, int height, float uthreshold, float lthreshold);

/* OpenCL setup and config */

#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif
extern cl::Platform platform;
extern cl::Device device;
extern cl::Context context;
extern cl::CommandQueue queue;
extern bool openCL_initialized;

void initCL();
char *getCLErrorString(cl_int err);
void printImageFormat(cl::ImageFormat format);
void clHandleError(std::string file, int line, cl_int err);
void clHandleError(cl_int err);

#endif
