#include <cl2.hpp>
#include <iostream>
#include "stdio.h"
#include "stdbool.h"
#include "string.h"
#include "math.h"
#include "time.h"
#include "float.h"
extern "C" {
    #include "../c/cutils.h"
    #include "../c/c-filters.h"
}
#include "opencl-filters.hpp"

using namespace std;
using namespace cl;

static int PATCH_RADIUS = 4;
static float ALPHA = 255;

#define LINEAR3(y,x,z) 3*((y)*width+(x))+(z)
#define LINEAR(y,x) (y)*width+(x)

point vector_bisector(float ax, float ay, float bx, float by);
float masked_convolute(int width, int height, char * img, int i, int j, float kernel[3][3], bool * mask);

#define MAX_LEN 2000
bool contour_mask[MAX_LEN*MAX_LEN];
float confidence[MAX_LEN*MAX_LEN];
point gradient_t[MAX_LEN*MAX_LEN];
point n_t[MAX_LEN*MAX_LEN];


// OPENCL SHIT
// +++++++++++

Kernel k_find_source;

void initCLInpainting(int width, int height){
    cout << "Initializing OpenCL model for Inpainting\n";

    /* 1. Build PROGRAM from source, for specific context */
    //createProgram("inpainting.cl");

    /*2. Create kernels */
    //k_find_source = Kernel(program, "find_source");
}

/*
    Outline:
    - Calculate border
    - Calculate priority for all pixels in the border
    - Find pixel with most priority
    - Find patch most similar with the patch of the previous pixel
    - Copy patches
    - Loop until image is filled
*/

clock_t tstart, tend;
float tcount;

void CL_inpaint_init(int width, int height, char * img, bool * mask) {

    cout << "HOLAAAAAAAAAA\n";
    cout << "HOLAAAAAAAAAA\n";
    cout << "HOLAAAAAAAAAA\n";

    if(!openCL_initialized) initCL();
    initCLInpainting(width, height);

    memset(confidence, 0, MAX_LEN*MAX_LEN*sizeof(float));
    memset(contour_mask, 0, MAX_LEN*MAX_LEN*sizeof(bool));

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            confidence[LINEAR(i,j)] = mask[LINEAR(i,j)] ? 0.0 : 1.0;
        }
    }
}

bool CL_inpaint_step(int width, int height, char * img, bool * mask) {

    memset(contour_mask, 0, MAX_LEN*MAX_LEN*sizeof(bool));
    memset(gradient_t, 0, MAX_LEN*MAX_LEN*sizeof(point)); // TODO: Debug
    memset(n_t, 0, MAX_LEN*MAX_LEN*sizeof(point)); // TODO: Debug

    // 1. CALCULATE CONTOUR
    // ++++++++++++++++++++
    tstart = clock();

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (mask[LINEAR(i,j)]) {
                for (int di = -1; di <= 1; di++) {
                    for (int dj = -1; dj <= 1; dj++) {
                        if (within(i + di, 0, height) && within(j + dj, 0, width)) {
                            if (!mask[LINEAR(i + di, j + dj)]) {
                                contour_mask[LINEAR(i,j)] = true;
                            }
                        }
                    }
                }
            }
        }
    }

    int contour_size = 0;
    for (int i = 0; i < height; i++) for (int j = 0; j < width; j++) {
        if (contour_mask[LINEAR(i,j)] == true) contour_size++;
    }

    if (contour_size == 0) {
        return 0;
    }

    tend = clock();
    tcount = (float)(tend - tstart) / CLOCKS_PER_SEC;
    printf("Contour (size = %d): %f\n", contour_size, tcount);

    // 2. FIND TARGET PATCH
    // ++++++++++++++++++++
    tstart = clock();

    int max_i = -1;
    int max_j = -1;
    float max_priority = -1.0;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {

            if (!contour_mask[LINEAR(i,j)]) {
                continue;
            }

            // Calculate confidence
            float sum = 0;
            int bot_k = max(i - PATCH_RADIUS, 0);
            int top_k = min(i + PATCH_RADIUS + 1, height);
            int bot_l = max(j - PATCH_RADIUS, 0);
            int top_l = min(j + PATCH_RADIUS + 1, width);
            for (int k = bot_k; k < top_k; k++) {
                for (int l = bot_l; l < top_l; l++) {
                    sum += confidence[LINEAR(k,l)];
                }
            }

            confidence[LINEAR(i,j)] = sum / (2 * PATCH_RADIUS + 1) * (2 * PATCH_RADIUS + 1);


            // Gradient:
            // According to impl I should take one gradient, according to paper I should take the max in the patch.
            // Also, according to both I need to look in the whole image, not just in the source. That's weird.
            // Following my gut, I'll calculate only one gradient
            float sobel_kernel_x[3][3] = { \
                {-1, 0, 1}, \
                {-2, 0, 2}, \
                {-1, 0, 1}  \
            };
            float sobel_kernel_y[3][3] = { \
                {-1, -2, -1}, \
                { 0,  0,  0}, \
                { 1,  2,  1}  \
            };
            float gx = masked_convolute(width, height, img, i, j, sobel_kernel_x, mask);
            float gy = masked_convolute(width, height, img, i, j, sobel_kernel_y, mask);
            float gx_t = -gy;
            float gy_t = gx;

            gradient_t[LINEAR(i,j)] = (point) { .x = gx_t, .y = gy_t }; // TODO: Debug
            
            
            // Normal:
            //  Easy way: Take spaces between edges (take the one who yields higher data)
            int di[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
            int dj[8] = {1, 1, 1, 0, -1, -1, -1, 0};
            int k_border = -1;

            // Find first edge/masked pixel
            for (int k = 0; k < 8; k++) {
                if (!within(i + di[k], 0, height) || \
                    !within(j + dj[k], 0, width) ||  \
                    mask[LINEAR(i + di[k], j + dj[k])]) {
                    k_border = k;
                    break;
                }
            }

            // Loop around and for every pair of edges/masked pixels
            // take the middle vector as candidate for normal
            float nx_max = -1;
            float ny_max = -1;

            if (k_border != -1) {
                bool is_out_prev = false;
                int k_prev = k_border;
                for (int l = 0; l < 8; l++) {
                    int k = ((k_border + 1) + l) % 8;
                    bool is_out = !within(i + di[k], 0, height) ||  \
                                  !within(j + dj[k], 0, width);
                    if ((is_out && !is_out_prev) || (mask[LINEAR(i + di[k], j + dj[k])])){
                        int ax = di[k_prev];
                        int ay = dj[k_prev];
                        int bx = di[k];
                        int by = dj[k];

                        point mid = vector_bisector(ax, ay, bx, by);
                        int nx = mid.x;
                        int ny = mid.y;

                        if (norm(nx, ny) > norm(nx_max, ny_max)) {
                            nx_max = nx;
                            ny_max = ny;
                        }
                        k_prev = k;

                        if (is_out) is_out_prev = true;
                    }
                }
            } else {
                // Since every vector is equally possible, I take the best
                float g_norm = norm(gx_t, gy_t);
                nx_max = gx_t / g_norm;
                ny_max= gy_t / g_norm;
            }

            n_t[LINEAR(i,j)] = (point) { .x = nx_max, .y = ny_max }; // TODO: Debug

            // Data
            float data = fabsf(gx_t * nx_max + gy_t * ny_max) / ALPHA;

            // Priority
            float priority = confidence[LINEAR(i,j)] * data;

            // Update max
            if (priority > max_priority) {
                max_priority = priority;
                max_i = i;
                max_j = j;
            }

        }
    }

    tend = clock();
    tcount = (float)(tend - tstart) / CLOCKS_PER_SEC;
    printf("Target patch (%d, %d): %f\n", max_i, max_j, tcount);

    // 3. FIND SOURCE PATCH
    // ++++++++++++++++++++
    tstart = clock();

    // (max_i, max_j) is the target patch
    float min_squared_diff = FLT_MAX;
    int max_source_i = -1;
    int max_source_j = -1;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {

            bool valid = true;

            // Sum
            float squared_diff = 0;
            for (int ki = -PATCH_RADIUS; ki <= PATCH_RADIUS; ki++) {
                for (int kj = -PATCH_RADIUS; kj <= PATCH_RADIUS; kj++) {
                    int target_i = max_i + ki;
                    int target_j = max_j + kj;
                    int source_i = i + ki;
                    int source_j = j + kj;

                    if (!within(source_i, 0, height) ||  \
                        !within(source_j, 0, width) ||   \
                        mask[LINEAR(source_i, source_j)]) {
                        valid = false;
                        goto out;
                    }

                    if (within(target_i, 0, height) &&  \
                        within(target_j, 0, width) &&   \
                        !mask[LINEAR(target_i, target_j)]) {
                        squared_diff += squared_distance3(img + LINEAR(target_i, target_j), img + LINEAR(source_i, source_j));
                    }
                }
            }
            out: if (!valid) continue;

            // Update
            if (squared_diff < min_squared_diff) {
                min_squared_diff = squared_diff;
                max_source_i = i;
                max_source_j = j;
            }
        }
    }

    tend = clock();
    tcount = (float)(tend - tstart) / CLOCKS_PER_SEC;
    printf("Source patch(%d, %d): %f\n", max_source_i, max_source_j, tcount);

    // 4. COPY
    // +++++++
    tstart = clock();

    if (min_squared_diff == -1) {
        return 0; // Abort mission
    }

    for (int ki = -PATCH_RADIUS; ki <= PATCH_RADIUS; ki++) {
        for (int kj = -PATCH_RADIUS; kj <= PATCH_RADIUS; kj++) {
            int target_i = max_i + ki;
            int target_j = max_j + kj;
            int source_i = max_source_i + ki;
            int source_j = max_source_j + kj;

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

    tend = clock();
    tcount = (float)(tend - tstart) / CLOCKS_PER_SEC;
    printf("Copy: %f\n", tcount);

    return 1;
}


void CL_inpainting(char * ptr, int width, int height, bool * mask_ptr) {
    inpaint_init(width, height, ptr, mask_ptr);

    while (true) {
        bool is_more = inpaint_step(width, height, ptr, mask_ptr);
        if (!is_more) break;
    }
}


// Calculates angle of vectors between vectors (ax, ay) and (bx, by)
point vector_bisector(float ax, float ay, float bx, float by) {
    float b_angle = atan2f(ay, ax);
    float a_angle = atan2f(by, bx);
    if (b_angle < a_angle) b_angle += 2 * PI;
    float diff_angle = b_angle - a_angle;
    float mid_angle = (diff_angle / 2) + a_angle;
    return (point) { cos(mid_angle), sin(mid_angle) };
}

// Convolution with a 3x3 kernel that handles edges via mirroring (ignores mask)
// TODO: Mask ignore
// TODO: Linus is a meanie: https://lkml.org/lkml/2015/9/3/428
float masked_convolute(int width, int height, char * img, int i, int j, float kernel[3][3], bool * mask) {
    float acc = 0;

    int kernel_radius = 1;
    int kernel_diameter = kernel_radius * 2 + 1;
    for (int ki = 0; ki < kernel_diameter; ki++) {
        for (int kj = 0; kj < kernel_diameter; kj++) {
            int inner_i = clamp(i + ki - kernel_radius, 0, height);
            int inner_j = clamp(j + kj - kernel_radius, 0, width);
            float avg = 0;
            for (int ci = 0; ci < 3; ci++) {
                avg += img[LINEAR3(inner_i, inner_j, ci)];
            }
            avg /= 3;
            acc += avg * kernel[ki][kj];
        }
    }

    return acc;
}

