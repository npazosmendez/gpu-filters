#include <chrono>
using namespace std::chrono;
static steady_clock::time_point tstart;
static steady_clock::time_point tend;
static duration<double> time_span;

#define START_TIMER tstart = steady_clock::now();

#define PRINT_AND_RESET(name)  \
        do {tend = steady_clock::now(); \
 			time_span = duration_cast<chrono::microseconds>(tend - tstart); \
 			printf("TIMER: %s\t%f\n", name, time_span.count() * 1000.0); \
 			tstart = steady_clock::now(); } while (0);

