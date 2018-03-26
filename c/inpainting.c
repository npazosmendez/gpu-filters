#include "c-filters.h"
#include "stdio.h"
#include "stdbool.h"
#include "string.h"
#include "math.h"

#define PI 3.14159265358979323846

int PATCH_RADIUS = 4;
float ALPHA = 255;

// Checks if value is inside the range [minimum, maximum]
int within(int value, int minimum, int maximum) {
    return value >= minimum && value <= maximum;
}

int max(int a, int b) {
    return a > b ? a : b;
}

int min(int a, int b) {
    return a < b ? a : b;
}

// Returns value restricted to range [minimum, maximum]
int clamp(int value, int minimum, int maximum) {
    return value < minimum ? minimum : value > maximum ? maximum : value;
}

typedef struct point {
    float x;
    float y;
} point;

// Calculates angle of vectors between vectors (ax, ay) and (bx, by)
point vector_bisector(float ax, float ay, float bx, float by) {
    float b_angle = atan2f(ay, ax);
    float a_angle = atan2f(by, bx);
    if (b_angle < a_angle) b_angle += 2 * PI;
    float diff_angle = b_angle - a_angle;
    float mid_angle = (diff_angle / 2) + a_angle;
    return (point) { cos(mid_angle), sin(mid_angle) };
}

// Calculates norm of vector
float norm(float x, float y) {
    return sqrt(x * x + y * y);
}

// Calculates squared distance of two vectors in three dimensions
float squared_distance3(char p[3], char q[3]) {
    return (p[0] - q[0]) * (p[0] - q[0]) +  \
           (p[1] - q[1]) * (p[1] - q[1]) +  \
           (p[2] - q[2]) * (p[2] - q[2]);
}

// Convolution with a 3x3 kernel that handles edges via mirroring (ignores mask)
// TODO: Mask ignore
// TODO: Linus is a meanie: https://lkml.org/lkml/2015/9/3/428
float masked_convolute(int width, int height, char (*img)[width][3], int i, int j, float kernel[3][3], bool (*mask)[width]) {
    float acc = 0;

    int kernel_radius = 1;
    int kernel_diameter = kernel_radius * 2 + 1;
    for (int ki = 0; ki < kernel_diameter; ki++) {
        for (int kj = 0; kj < kernel_diameter; kj++) {
            int inner_i = clamp(i + ki - kernel_radius, 0, height);
            int inner_j = clamp(j + kj - kernel_radius, 0, width);
            float avg = 0;
            for (int ci = 0; ci < 3; ci++) {
                avg += img[inner_i][inner_j][ci];
            }
            avg /= 3;
            acc += avg * kernel[ki][kj];
        }
    }

    return acc;
}


    /*
        Outline:
        - Calculate border
        - Calculate priority for all pixels in the border
        - Find pixel with most priority
        - Find patch most similar with the patch of the previous pixel
        - Copy patchs
        - Loop (until no border)

        Dummy Algorithm:
        Make all steps iterating through all the image/arrays

        GPU Version:
        Border calculation can distribute over pixels
        Pixel with most priority is a reduce operation
        Find most similar patch is a reduce operation
        Patch copy can distribute over pixels (although its cost is negligent)

        OP Algorithm:
        Maintain a Heap linked with the mask Matrix (heap is a max heap on priority over pixel borders).
        Take advantange of the fact that at the end of the loop, most info is the same.
        When we copy a patch, we look for the pixels the change affected.
        For filled pixels, we use the matrix values to remove said pixels from the heap.
        For the new border pixels, we insert them into the heap.
        In every heap operation we update the matrix so that each element in the border points to the index of said pixel in the heap.
        We recalculate priorities in a similar fashion (around the patch).
        Once we're done, we pop from the heap and find our pixel with most priority.
        Then we find the minimum iterating through all the image ( =( ).
        We copy the patch and start over.
    */

void inpainting(char * ptr, int width, int height, bool * mask_ptr) {

    char (*img)[width][3] = (char (*)[width][3]) ptr;
    bool (*mask)[width] = (bool (*)[width]) mask_ptr;

    float confidence[height][width];
    memset(confidence, 0, height*width*sizeof(float));
    bool border_mask[height][width];
    memset(border_mask, 0, height*width*sizeof(bool));

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            confidence[i][j] = mask[i][j] ? 0.0 : 1.0;
        }
    }

    while (true) {

        // 1. Calculate contour
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                if (mask[i][j]) {
                    for (int di = -1; di <= 1; di++) {
                        for (int dj = -1; dj <= 1; dj++) {
                            if (0 >= i + di && i + di < height && 0 >= j + dj && j + dj < width) {
                                if (!mask[i + di][j + dj]) {
                                    border_mask[i][j] = true;
                                }
                            }
                        }
                    }
                }
            }
        }

        // 2. Find target patch 
        int max_i = -1;
        int max_j = -1;
        float max_priority = -1.0;

        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {

                if (!border_mask[i][j]) {
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
                        sum += confidence[k][l];
                    }
                }
                confidence[i][j] = sum / (2 * PATCH_RADIUS + 1) * (2 * PATCH_RADIUS + 1);

                // Gradient:
                // Average channels and Sobel that shit up
                // Now... According to impl I should take one gradient, according to paper I should take the max in the patch.
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
                
                // Normal:
                //  Regular: Neighbours in 8 directions only
                //  Take spaces between edges (take the one who yields higher data)
                //  Pro: LS to estimate line, then normal of that

                int di[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
                int dj[8] = {1, 1, 1, 0, -1, -1, -1, 0};
                int k_border = -1;
                // Find first edge/masked pixel
                for (int k = 0; k < 8; k++) {
                    if (!within(i + di[k], 0, height) || \
                        !within(j + dj[k], 0, width) ||  \
                        mask[i + di[k]][j + dj[k]]) {
                        k_border = k;
                        break;
                    }
                }
                // Loop around and for every pair of continuous edges/masked pixels
                // take the middle vector as candidate for normal to edge (take the best)
                float nx_max = -1;
                float ny_max = -1;

                if (k_border != -1) {
                    bool is_out_prev = false;
                    int k_prev = k_border;
                    for (int l = 0; l < 8; l++) {
                        int k = ((k_border + 1) + l) % 8;
                        bool is_out = !within(i + di[k], 0, height) ||  \
                                      !within(j + dj[k], 0, width);
                        if ((is_out && !is_out_prev) || (mask[i + di[k]][j + dj[k]])){
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
                    // Since very vectory is equally possible, I take the best
                    float g_norm = norm(gx_t, gy_t);
                    nx_max = gx_t / g_norm;
                    ny_max= gy_t / g_norm;
                }

                // Data
                float data = fabsf(gx_t * nx_max + gy_t * ny_max) / ALPHA;

                // Priority
                float priority = confidence[i][j] * data;

                // Update max
                if (priority > max_priority) {
                    max_priority = priority;
                    max_i = i;
                    max_j = j;
                }

            }
        }

        // 3. Find source patch
        // (max_i, max_j) is the target patch
        float min_squared_diff = -1;
        int max_target_i = -1; // TODO: THIS IS THE SOURCE NOT THE TARGET YOU DUMBDUMB
        int max_target_j = -1;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {

                // Is valid source patch
                bool valid = true;
                for (int ki = -PATCH_RADIUS; ki <= PATCH_RADIUS; ki++) {
                    for (int kj = -PATCH_RADIUS; kj <= PATCH_RADIUS; kj++) {
                        int source_i = i + ki;
                        int source_j = j + kj;
                        if (!within(source_i, 0, height) ||  \
                            !within(source_j, 0, width) ||   \
                            mask[source_i][source_j]) {
                            valid = false;
                            goto out;
                        }
                    }
                }
                out: if (!valid) continue;

                // Sum
                float squared_diff = 0;
                for (int ki = -PATCH_RADIUS; ki <= PATCH_RADIUS; ki++) {
                    for (int kj = -PATCH_RADIUS; kj <= PATCH_RADIUS; kj++) {
                        int target_i = max_i + ki;
                        int target_j = max_j + kj;
                        int source_i = i + ki;
                        int source_j = j + kj;

                        if (within(target_i, 0, height) &&  \
                            within(target_j, 0, width) &&   \
                            !mask[target_i][target_j]) {
                            squared_diff += squared_distance3(img[target_i][target_j], img[source_i][source_j]);
                        }
                    }
                }

                // Update
                if (squared_diff < min_squared_diff) {
                    min_squared_diff = squared_diff;
                    max_target_i = i;
                    max_target_j = j;
                }
            }
        }

        // 4. Copy
        if (min_squared_diff == -1) {
            break; // Abort mission
        }

        for (int ki = -PATCH_RADIUS; ki <= PATCH_RADIUS; ki++) {
            for (int kj = -PATCH_RADIUS; kj <= PATCH_RADIUS; kj++) {
                int target_i = max_i + ki;
                int target_j = max_j + kj;
                int source_i = max_target_i + ki;
                int source_j = max_target_j + kj;

                if (within(target_i, 0, height) &&  \
                    within(target_j, 0, width) &&   \
                    mask[target_i][target_j]) {
                    img[target_i][target_j][0] = img[source_i][source_j][0];
                    img[target_i][target_j][1] = img[source_i][source_j][1];
                    img[target_i][target_j][2] = img[source_i][source_j][2];
                    mask[target_i][target_j] = false;
                }
            }
        }

        break; // TODO: Remove when things are... Sort of working
    }
}

void generate_arbitrary_mask(bool * dst, int width, int height) {
    memset(dst, 0, width * height * sizeof(bool));

    bool (*mask)[width] = (bool (*)[width]) dst;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (i >= height * 4.0/10.0 && i < height * 5.0/10.0) {
                if (j >= width * 4.0/10.0 && j < width * 5.0/10.0) {
                    mask[i][j] = true;
                }
            }
        }
    }
}
