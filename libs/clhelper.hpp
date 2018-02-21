#include <CL/cl.hpp>

extern cl::Platform platform;
extern cl::Device device;
extern cl::Context context;
extern cl::CommandQueue queue;
extern bool openCL_initialized;

void initCL();
char *getCLErrorString(cl_int err);
