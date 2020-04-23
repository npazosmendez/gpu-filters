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
#include "c/cutils.h"
#include "c/c-filters.h"
}
#include "opencl/opencl-filters.hpp"

//using namespace std; // makes some math definition ambiguous in my compiler. Ur ambiguous.
using namespace std::chrono;
using namespace cl;


// TEST MACROS
__attribute__((unused)) static char _err[1000];

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

int within(int x, int y, int width, int height) {
    return x >= 0 && y >= 0 && x < width && y < height;
}

point vector_bisector(float ax, float ay, float bx, float by) {
    float b_angle = atan2(ay, ax);
    float a_angle = atan2(by, bx);
    if (b_angle < a_angle) b_angle += 2 * PI;
    float diff_angle = b_angle - a_angle;
    float mid_angle = (diff_angle / 2) + a_angle;
    return (point) { cos(mid_angle), sin(mid_angle) };
}

point get_ortogonal_to_contour(int x, int y, bool * mask, int width, int height) {
    int dx[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
    int dy[8] = {1, 1, 1, 0, -1, -1, -1, 0};

    int first_neighbour_contour_di = -1;

    // Find first masked pixel
    for (int di = 0; di < 8; di++) {
        int in_x = x + dx[di];
        int in_y = y + dy[di];
        int in_index = in_y * width + in_x;

        if (within(in_x, in_y, width, height) && mask[in_index]) {
            first_neighbour_contour_di = di;
        }
    }

    // Find orthogonal vector
    point orthogonal = (point) {0, -1};

    if (first_neighbour_contour_di != -1) {

        // Loop through all the pairs of contour pixels in the (x, y)
        // neighbourhood without any contour pixels in between
        int prev_neighbour_contour_di = first_neighbour_contour_di;

        for (int i = 0; i < 8; i++) {

            int di = ((first_neighbour_contour_di + 1) + i) % 8;

            int in_x = x + dx[di];
            int in_y = y + dy[di];
            int in_index = in_y * width + in_x;


            if (within(in_x, in_y, width, height) && mask[in_index]) {
                int cur_neighbour_contour_di = di;

                float ax = (float) dx[prev_neighbour_contour_di];
                float ay = (float) dy[prev_neighbour_contour_di];
                float bx = (float) dx[cur_neighbour_contour_di];
                float by = (float) dy[cur_neighbour_contour_di];

                point mid = vector_bisector(ax, ay, bx, by);

                // Avoid double borders
                bool are_adjacent = (cur_neighbour_contour_di == prev_neighbour_contour_di + 1) ||  \
                                    ((cur_neighbour_contour_di == 0) && (prev_neighbour_contour_di == 7));
                if (!are_adjacent) {
                    // TODO: Maybe maximize another criterium?
                    orthogonal = mid;
                }

                prev_neighbour_contour_di = di;
            }
        }
    } else {
        // I take default vector as fallback
        // TODO: Unlikely case, but this should be parallel to gradient so it gets filled really fast
    }

    return orthogonal;
}

void try_mask(bool * mask) {
    point orthogonal = get_ortogonal_to_contour(1, 1, mask, 3, 3);
    printf("Result: (%f, %f)\n", orthogonal.x , orthogonal.y);
}

void test_get_contour() {
    bool maskA[9] = {
        1, 0, 0,
        0, 1, 0,
        1, 1, 0,
    };
    try_mask(maskA);

    bool maskB[9] = {
            0, 1, 0,
            0, 1, 0,
            0, 1, 0
    };
    try_mask(maskB);

    bool maskC[9] = {
            1, 0, 0,
            0, 1, 0,
            0, 0, 1
    };
    try_mask(maskC);

    bool maskD[9] = {
            1, 0, 0,
            1, 1, 0,
            0, 0, 1
    };
    try_mask(maskD);
}



int main() {
    RUN_TEST(test_get_contour);
    return 0;
}