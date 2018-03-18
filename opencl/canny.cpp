#include <CL/cl.hpp>
#include "opencl-filters.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <cmath>

using namespace std;
using namespace cl;

void CL_canny(char * src_c, int width, int height, float uthreshold, float lthreshold){

    assert(openCL_initialized);

    float * src = (float*)malloc(width*height*sizeof(float));

    // Intensity channel only (color average)
    unsigned char (*imgRGB)[3] = (unsigned char (*)[3])src_c;
    for (int i = 0; i < height*width; i++) {
        float temp = imgRGB[i][0]+imgRGB[i][1]+imgRGB[i][2];
        src[i] = temp/3;
    }

    /* Build PROGRAM from source, for specific context */
    cl::Program program;
    ifstream sourceFile("opencl/canny.cl");
    /* NOTE: reading the source code from another file

    during runtime makes the binary's location important.
    Not good. */
    if (!sourceFile.is_open()) {
        cerr << "Can't open CL kernel source." << endl;
        exit(1);
    }
    string sourceCode(istreambuf_iterator<char>(sourceFile), (istreambuf_iterator<char>()));
    cl::Program::Sources sources(1, make_pair(sourceCode.c_str(), sourceCode.length()+1));
    program = cl::Program(context, sources);
    program.build(context.getInfo<CL_CONTEXT_DEVICES>());

    // Create an OpenCL Image / texture and transfer data to the device
    cl_int err = 0;
    Image2D clImage = Image2D(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, ImageFormat(CL_INTENSITY, CL_FLOAT), width, height, 0, (void*)src, &err);

    // Create a buffer for the result
    Buffer clResult = Buffer(context, CL_MEM_WRITE_ONLY, sizeof(float)*width*height);

    // Create and run kernel
    Kernel kernel = Kernel(program, "canny");
    kernel.setArg(0, clImage);
    kernel.setArg(1, clResult);

    if (cl_int err = queue.enqueueNDRangeKernel(
        kernel,
        NullRange,
        NDRange(width, height),
        NullRange // default
    )){
        cerr << "ERROR: " << getCLErrorString(err) << endl;
        exit(1);
    }

    // Transfer img back to host
    queue.enqueueReadBuffer(clResult, CL_TRUE, 0, sizeof(float)*height*width, src);


    for (int i = 0; i < height*width; i++) {
        imgRGB[i][0] = src[i];
        imgRGB[i][1] = src[i];
        imgRGB[i][2] = src[i];
    }

    return;
}
