#ifndef OPENCLFILTERS_H
#define OPENCLFILTERS_H

#include <string>

/* Image filters */

void CL_convoluion2D(float* src, int width, int height, float * kernel_img, int ksize, float* dst);

void CL_canny(char * src, int width, int height, float uthreshold, float lthreshold);
void CL_hough(char * src, int width, int height, int a_ammount, int p_ammount, char* counter);

void CL_inpaint_init(int width, int height, char * ptr, bool * mask_ptr);
bool CL_inpaint_step(int width, int height, char * ptr, bool * mask_ptr);

/* OpenCL setup and config */

#include <cl2.hpp>
extern cl::Platform platform;
extern cl::Device device;
extern cl::Context context;
extern cl::CommandQueue queue;
extern cl::Program program;
extern bool openCL_initialized;

void initCL();
char *getCLErrorString(cl_int err);
void printImageFormat(cl::ImageFormat format);
void clHandleError(std::string file, int line, cl_int err);
void clHandleError(cl_int err);

void createProgram(std::string filename);

#endif
