//
// Created by Daro on 23/09/2018.
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
#include <stdio.h>
extern "C" {
#include "../c/cutils.h"
#include "../c/c-filters.h"
}
#include "opencl/opencl-filters.hpp"

//using namespace std; // makes some math definition ambiguous in my compiler
using namespace std::chrono;
using namespace cl;


// TEST MACROS

static char _err[1000];

#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_RESET   "\x1b[0m"

#define RUN_TEST(test) do { printf("Running " #test "... \n"); test(); _SUCCESS;} while (0)
#define _SUCCESS printf("%s ✓ Success!%s\n", ANSI_GREEN, ANSI_RESET)
#define _FAILURE do { printf("%s ✕ Failed assertion: %s (line %d)%s\n", ANSI_RED, _err, __LINE__, ANSI_RESET); exit(0); } while (0)

#define ASSERT(cond)     _ASSERT( _CONDITION(cond)                   , _MESSAGE(#cond)                                            )
#define ASSERT_EQI(a, b) _ASSERT( _CONDITION((a) == (b))             , _MESSAGE("Expected %d but got %d", (int)b, (int)a)         )
#define ASSERT_EQF(a, b) _ASSERT( _CONDITION(abs((a) - (b)) < 1e-1)  , _MESSAGE("Expected %.3f but got %.3f", (float)b, (float)a) )
#define _ASSERT(cond, ...) do { sprintf((char *) _err, __VA_ARGS__); if (!(cond)) _FAILURE; } while (0)
#define _CONDITION(...) __VA_ARGS__
#define _MESSAGE(...) __VA_ARGS__

// TEST MACROS




static Kernel k_calculate_flow;
static Kernel k_convolution_x;
static Kernel k_convolution_y;
static Kernel k_convolution_blur;
static Kernel k_subsample;
static Kernel k_calculate_intensity;
static Kernel k_calculate_tensor_and_mineigen;

static Kernel k_test_get_gradient_t;

static cl_int err = 0;

static void init() {

    std::cout << "Initializing OpenCL model for Optical Flow\n";

    // OpenCL initialization
    if (!openCL_initialized) initCL();

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
    k_test_get_gradient_t = Kernel(program, "test_get_gradient_t");
}

/*
void intensity_to_img(unsigned char * intensity, char * img, int width, int height) {
    for (int i = 0; i < height * width; i++) {
        for (int k = 0; k < 3; k++) {
            img[3*i+k] = intensity[i];
        }
    }
}

static char * test_kanade() {
    int width = 5;
    int height = 5;

    unsigned char intensity_old[5*5] = {
              0,   0, 200,   0,   0,
              0,   0, 225,   0,   0,
              0,   0, 250, 200, 150,
              0,   0,   0,   0,   0,
              0,   0,   0,   0,   0
    };
    char img_old[5*5*3];
    intensity_to_img(intensity_old, img_old, width, height);

    unsigned char intensity_new[5*5] = {
              0, 175,   0,   0,   0,
              0, 200,   0,   0,   0,
              0, 225,   0,   0,   0,
              0, 250, 200, 150, 100,
              0,   0,   0,   0,   0
    };
    char img_new[5*5*3];
    intensity_to_img(intensity_new, img_new, width, height);

    vec flow[5*5];

    int levels = 1;

    kanade(width, height, img_old, img_new, flow, levels);

    cout << flow[2*width+2].x << "\n";
    ASSERT_EQF(flow[2*width+2].x, -0.127);
    ASSERT_EQF(flow[2*width+2].y, 0.093);
    //mu_assert(abs(flow[2*width+2].x - (-0.127)) < EPS);   Nope, flow is integer
    //mu_assert(abs(flow[2*width+2].y - (0.093)) < EPS);    Nope, flow is integer

    return 0;
}
 */

void test_intensity() {
    int width = 5;
    int height = 1;

    unsigned char img[5*3] = {
            100, 50, 100,
            20, 20, 20,
            0, 0, 0,
            255, 255, 255,
            0, 255, 10
    };

    float dst[5];

    Buffer b_img = Buffer(context, CL_MEM_USE_HOST_PTR, sizeof(img), img, &err);
    Buffer b_dst = Buffer(context, CL_MEM_USE_HOST_PTR, sizeof(dst), dst, &err);

    k_calculate_intensity.setArg(0, b_img);
    k_calculate_intensity.setArg(1, b_dst);
    err = queue.enqueueNDRangeKernel(
            k_calculate_intensity,
            NullRange,
            NDRange(width, height),
            NullRange // default
    );
    queue.finish();
    clHandleError(__FILE__,__LINE__,err);

    ASSERT_EQF(dst[0], 83.333);
    ASSERT_EQF(dst[1], 20);
    ASSERT_EQF(dst[2], 0);
    ASSERT_EQF(dst[3], 255);
    ASSERT_EQF(dst[4], 88.333);
}

void test_get_gradient_t() {
    float intensity_old[5*5] = {
            1,  2,  3,  4,  5,
            6,  7,  8,  9, 10,
            11, 12, 13, 14, 15,
            16, 17, 18, 19, 20,
            21, 22, 23, 24, 25
    };

    float intensity_new[5*5] = {
            50,  48,  46,  44,  42,
            40,  38,  36,  34,  32,
            100, 105, 110, 115, 120,
            30, 28, 26, 24, 22,
            200, 205, 210, 215, 220
    };

    float output[1];

    Buffer b_intensity_old = Buffer(context, CL_MEM_USE_HOST_PTR, sizeof(intensity_old), intensity_old, &err);
    Buffer b_intensity_new = Buffer(context, CL_MEM_USE_HOST_PTR, sizeof(intensity_new), intensity_new, &err);
    Buffer b_output = Buffer(context, CL_MEM_USE_HOST_PTR, sizeof(output), output, &err);

    cl_int2 size = {5, 5};

    cl_int2 in_pos;
    cl_float2 previous_guess;
    cl_float2 iter_guess;

    // Test 1

    in_pos = {2, 2};
    previous_guess = {0, 0};
    iter_guess = {0, 0};

    k_test_get_gradient_t.setArg(0, in_pos);
    k_test_get_gradient_t.setArg(1, previous_guess);
    k_test_get_gradient_t.setArg(2, iter_guess);
    k_test_get_gradient_t.setArg(3, size);
    k_test_get_gradient_t.setArg(4, b_intensity_new);
    k_test_get_gradient_t.setArg(5, b_intensity_old);
    k_test_get_gradient_t.setArg(6, b_output);
    err = queue.enqueueNDRangeKernel(
            k_test_get_gradient_t,
            NullRange,
            NDRange(1, 1),
            NullRange // default
    );
    queue.finish();
    clHandleError(__FILE__,__LINE__,err);

    ASSERT_EQF(output[0], 110 - 13);

    // Test 2

    in_pos = {0, 0};
    previous_guess = {0, 0};
    iter_guess = {0, 0};

    k_test_get_gradient_t.setArg(0, in_pos);
    k_test_get_gradient_t.setArg(1, previous_guess);
    k_test_get_gradient_t.setArg(2, iter_guess);
    k_test_get_gradient_t.setArg(3, size);
    k_test_get_gradient_t.setArg(4, b_intensity_new);
    k_test_get_gradient_t.setArg(5, b_intensity_old);
    k_test_get_gradient_t.setArg(6, b_output);
    err = queue.enqueueNDRangeKernel(
            k_test_get_gradient_t,
            NullRange,
            NDRange(1, 1),
            NullRange // default
    );
    queue.finish();
    clHandleError(__FILE__,__LINE__,err);

    ASSERT_EQF(output[0], 49);

    // Test 3
    in_pos = {2, 2};
    previous_guess = {1, 1};
    iter_guess = {0, 0};

    k_test_get_gradient_t.setArg(0, in_pos);
    k_test_get_gradient_t.setArg(1, previous_guess);
    k_test_get_gradient_t.setArg(2, iter_guess);
    k_test_get_gradient_t.setArg(3, size);
    k_test_get_gradient_t.setArg(4, b_intensity_new);
    k_test_get_gradient_t.setArg(5, b_intensity_old);
    k_test_get_gradient_t.setArg(6, b_output);
    err = queue.enqueueNDRangeKernel(
            k_test_get_gradient_t,
            NullRange,
            NDRange(1, 1),
            NullRange // default
    );
    queue.finish();
    clHandleError(__FILE__,__LINE__,err);

    ASSERT_EQF(output[0], 24 - 13);

    // Test 4
    in_pos = {2, 2};
    previous_guess = {0.5, 0.5};
    iter_guess = {0.5, 0.5};

    k_test_get_gradient_t.setArg(0, in_pos);
    k_test_get_gradient_t.setArg(1, previous_guess);
    k_test_get_gradient_t.setArg(2, iter_guess);
    k_test_get_gradient_t.setArg(3, size);
    k_test_get_gradient_t.setArg(4, b_intensity_new);
    k_test_get_gradient_t.setArg(5, b_intensity_old);
    k_test_get_gradient_t.setArg(6, b_output);
    err = queue.enqueueNDRangeKernel(
            k_test_get_gradient_t,
            NullRange,
            NDRange(1, 1),
            NullRange // default
    );
    queue.finish();
    clHandleError(__FILE__,__LINE__,err);

    ASSERT_EQF(output[0], 24 - 13);

    // Test 5
    in_pos = {2, 2};
    previous_guess = {0.1, 0.1};
    iter_guess = {0.1, 0.1};

    k_test_get_gradient_t.setArg(0, in_pos);
    k_test_get_gradient_t.setArg(1, previous_guess);
    k_test_get_gradient_t.setArg(2, iter_guess);
    k_test_get_gradient_t.setArg(3, size);
    k_test_get_gradient_t.setArg(4, b_intensity_new);
    k_test_get_gradient_t.setArg(5, b_intensity_old);
    k_test_get_gradient_t.setArg(6, b_output);
    err = queue.enqueueNDRangeKernel(
            k_test_get_gradient_t,
            NullRange,
            NDRange(1, 1),
            NullRange // default
    );
    queue.finish();
    clHandleError(__FILE__,__LINE__,err);

    ASSERT_EQF(output[0], (70.4 + 18.4 + 4.16 + 0.96) - 13);
}

void test_functional_horizontal_movement() {
    int width = 5;
    int height = 5;

    unsigned char img_old[5*5*3] = {};
    img_old[3*(2 * 5 + 2) + 0] = 255;
    img_old[3*(2 * 5 + 2) + 1] = 255;
    img_old[3*(2 * 5 + 2) + 2] = 255;

    unsigned char img_new[5*5*3] = {};
    img_new[3*(2 * 5 + 3) + 0] = 254;
    img_new[3*(2 * 5 + 3) + 1] = 254;
    img_new[3*(2 * 5 + 3) + 2] = 254;

    vecf output_flow[5*5];

    int levels = 2;

    CL_kanade(width, height, (char*) img_old, (char*)img_new, output_flow, levels);

    ASSERT_EQF(output_flow[2 * 5 + 2].x, 1);
    ASSERT_EQF(output_flow[2 * 5 + 2].y, 0);
}

void test_functional_vertical_movement() {
    int width = 5;
    int height = 5;

    // A red dot moves one to the right
    unsigned char img_old[5*5*3] = {};
    img_old[3*(2 * 5 + 2) + 0] = 255;
    img_old[3*(2 * 5 + 2) + 1] = 255;
    img_old[3*(2 * 5 + 2) + 2] = 255;

    unsigned char img_new[5*5*3] = {};
    img_new[3*(3 * 5 + 2) + 0] = 254;
    img_new[3*(3 * 5 + 2) + 1] = 254;
    img_new[3*(3 * 5 + 2) + 2] = 254;

    vecf output_flow[5*5];

    int levels = 2;

    CL_kanade(width, height, (char*) img_old, (char*)img_new, output_flow, levels);

    ASSERT_EQF(output_flow[2 * 5 + 2].x, 0);
    ASSERT_EQF(output_flow[2 * 5 + 2].y, 1);
}

void test_functional_diagonal_movement() {
    int width = 5;
    int height = 5;

    // A red dot moves one to the right
    unsigned char img_old[5*5*3] = {};
    img_old[3*(2 * 5 + 2) + 0] = 255;
    img_old[3*(2 * 5 + 2) + 1] = 255;
    img_old[3*(2 * 5 + 2) + 2] = 255;

    unsigned char img_new[5*5*3] = {};
    img_new[3*(3 * 5 + 3) + 0] = 254;
    img_new[3*(3 * 5 + 3) + 1] = 254;
    img_new[3*(3 * 5 + 3) + 2] = 254;

    vecf output_flow[5*5];

    int levels = 2;

    CL_kanade(width, height, (char*) img_old, (char*)img_new, output_flow, levels);

    ASSERT_EQF(output_flow[2 * 5 + 2].x, 1);
    ASSERT_EQF(output_flow[2 * 5 + 2].y, 1);
}

class Figure {
public:
    unsigned char * img;
    int width, height;

    Figure(int width, int height) {
        this->width = width;
        this->height = height;
        img = (unsigned char *) malloc(width * height * 3 * sizeof(unsigned char));
        memset(img, 0, width * height * 3 * sizeof(unsigned char));
    }

    ~Figure() {
        free(img);
    }


    char * get_img() {
        return (char *) img;
    }

    void paint(int x, int y, float intensity) {
        img[3*(y * width + x) + 0] = intensity;
        img[3*(y * width + x) + 1] = intensity;
        img[3*(y * width + x) + 2] = intensity;
    }

    void paint(int x, int y) {
        img[3*(y * width + x) + 0] = 255;
        img[3*(y * width + x) + 1] = 255;
        img[3*(y * width + x) + 2] = 255;
    }
};

void test_functional_long_horizontal_movement() {
    int width = 16;
    int height = 16;

    Figure fig_old = Figure(width, height);
    fig_old.paint(8, 8);

    Figure fig_new = Figure(width, height);
    fig_new.paint(11, 8);

    vecf * output_flow = (vecf*) malloc(width*height*sizeof(vecf));

    int levels = 2;

    CL_kanade(width, height, fig_old.get_img(), fig_new.get_img(), output_flow, levels);

    ASSERT_EQF(output_flow[8 * width + 8].x, 3);
    ASSERT_EQF(output_flow[8 * width + 8].y, 0);

    free(output_flow);
}

void test_functional_long_vertical_movement() {
    int width = 16;
    int height = 16;

    Figure fig_old = Figure(width, height);
    fig_old.paint(8, 8);

    Figure fig_new = Figure(width, height);
    fig_new.paint(8, 11);

    vecf * output_flow = (vecf*) malloc(width*height*sizeof(vecf));

    int levels = 2;

    CL_kanade(width, height, fig_old.get_img(), fig_new.get_img(), output_flow, levels);

    ASSERT_EQF(output_flow[8 * width + 8].x, 0);
    ASSERT_EQF(output_flow[8 * width + 8].y, 3);

    free(output_flow);
}

void test_functional_super_long_diagonal_movement() {
    int width = 32;
    int height = 32;

    Figure fig_old = Figure(width, height);
    fig_old.paint(16, 16);

    Figure fig_new = Figure(width, height);
    fig_new.paint(20, 20);

    vecf * output_flow = (vecf*) malloc(width*height*sizeof(vecf));

    int levels = 3;

    CL_kanade(width, height, fig_old.get_img(), fig_new.get_img(), output_flow, levels);

    ASSERT_EQF(output_flow[16 * width + 16].x, 4);
    ASSERT_EQF(output_flow[16 * width + 16].y, 4);

    free(output_flow);
}

void test_functional_two_disjoint_points() {
    int width = 32;
    int height = 32;

    Figure fig_old = Figure(width, height);
    fig_old.paint(20, 20);
    fig_old.paint(15, 15);

    Figure fig_new = Figure(width, height);
    fig_new.paint(19, 20);
    fig_new.paint(16, 16);

    vecf * output_flow = (vecf*) malloc(width*height*sizeof(vecf));

    int levels = 1;

    CL_kanade(width, height, fig_old.get_img(), fig_new.get_img(), output_flow, levels);

    ASSERT_EQF(output_flow[20 * width + 20].x, -1);
    ASSERT_EQF(output_flow[20 * width + 20].y, 0);

    ASSERT_EQF(output_flow[15 * width + 15].x, 1);
    ASSERT_EQF(output_flow[15 * width + 15].y, 1);

    free(output_flow);
}

void test_functional_line_moving() {
    int width = 32;
    int height = 32;

    Figure fig_old = Figure(width, height);
    fig_old.paint(15, 15,  50);
    fig_old.paint(16, 15,  200);

    Figure fig_new = Figure(width, height);
    fig_new.paint(16, 15,  50);
    fig_new.paint(17, 15,  200);

    vecf * output_flow = (vecf*) malloc(width*height*sizeof(vecf));

    int levels = 1;

    CL_kanade(width, height, fig_old.get_img(), fig_new.get_img(), output_flow, levels);

    ASSERT_EQF(output_flow[15 * width + 15].x, 1);
    ASSERT_EQF(output_flow[15 * width + 15].y, 0);

    ASSERT_EQF(output_flow[15 * width + 16].x, 1);
    ASSERT_EQF(output_flow[15 * width + 16].y, 0);

    free(output_flow);
}

void test_functional_snake_moving() {
    int width = 32;
    int height = 32;

    Figure fig_old = Figure(width, height);
    fig_old.paint(15, 15,  50);
    fig_old.paint(16, 16,  200);

    Figure fig_new = Figure(width, height);
    fig_new.paint(16, 15,  50);
    fig_new.paint(17, 16,  200);

    vecf * output_flow = (vecf*) malloc(width*height*sizeof(vecf));

    int levels = 1;

    CL_kanade(width, height, fig_old.get_img(), fig_new.get_img(), output_flow, levels);

    ASSERT_EQF(output_flow[15 * width + 15].x, 1);
    ASSERT_EQF(output_flow[15 * width + 15].y, 0);

    ASSERT_EQF(output_flow[16 * width + 16].x, 1);
    ASSERT_EQF(output_flow[16 * width + 16].y, 0);

    free(output_flow);
}


int main(int argc, char** argv) {
    init();
    printf("\n");

    printf("UNIT TESTS\n");
    RUN_TEST(test_intensity);
    RUN_TEST(test_get_gradient_t);
    printf("\n");

    printf("FUNCTIONAL TESTS\n");
    /*
    RUN_TEST(test_functional_horizontal_movement);
    RUN_TEST(test_functional_vertical_movement);
    RUN_TEST(test_functional_diagonal_movement);
    RUN_TEST(test_functional_long_horizontal_movement);
    RUN_TEST(test_functional_long_vertical_movement);
    RUN_TEST(test_functional_super_long_diagonal_movement);
    RUN_TEST(test_functional_two_disjoint_points);

    RUN_TEST(test_functional_snake_moving);
    */

    RUN_TEST(test_functional_line_moving);

    printf("\n");

    return 0;
}

