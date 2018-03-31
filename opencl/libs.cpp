#include "opencl-filters.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include <cmath>
#include <cl.hpp>

using namespace cl;
using namespace std;


void CL_convoluion2D(float* src, int width, int height, float * kernel_img, int ksize, float* dst){

    assert(openCL_initialized);

    /* Build PROGRAM from source, for specific context */
    cl::Program program;
    ifstream sourceFile("libs/convolution2D.cl");
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


    // Create an OpenCL Image / texture and transfer data to the device+
    cl_int err = 0;
    Image2D clImage = Image2D(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, ImageFormat(CL_INTENSITY, CL_FLOAT), width, height, 0, (void*)src, &err);
    Image2D clKern = Image2D(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, ImageFormat(CL_INTENSITY, CL_FLOAT), ksize, ksize, 0, (void*)kernel_img, &err);

    // Create a buffer for the result
    Buffer clResult = Buffer(context, CL_MEM_WRITE_ONLY, sizeof(float)*width*height);

    // Create and run kernel
    Kernel kernel = Kernel(program, "convolution2D");
    kernel.setArg(0, clImage);
    kernel.setArg(1, (cl_int)ksize);
    kernel.setArg(2, clKern);
    kernel.setArg(3, clResult);

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
    queue.enqueueReadBuffer(clResult, CL_TRUE, 0, sizeof(float)*height*width, dst);
}
