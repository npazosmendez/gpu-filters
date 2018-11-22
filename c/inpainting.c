#include "c-filters.h"
#include "stdio.h"
#include "stdbool.h"
#include "string.h"
#include "math.h"
#include "time.h"
#include "float.h"
#include "cutils.h"

static int PATCH_RADIUS = 4;
static float ALPHA = 255;

#define LINEAR3(y,x,z) 3*((y)*width+(x))+(z)
#define LINEAR(y,x) (y)*width+(x)

static point vector_bisector(float ax, float ay, float bx, float by);
static float masked_convolute(int width, int height, char * img, int i, int j, float kernel[3][3], bool * mask);

#define MAX_LEN 2000
static bool contour_mask[MAX_LEN*MAX_LEN];
static float confidence[MAX_LEN*MAX_LEN];
static point gradient_t[MAX_LEN*MAX_LEN];
static point n_t[MAX_LEN*MAX_LEN];

/*
    Outline:
    - Calculate border
    - Calculate priority for all pixels in the border
    - Find pixel with most priority
    - Find patch most similar with the patch of the previous pixel
    - Copy patches
    - Loop until image is filled
*/

// TODO: Don't use clock it's not clock time!!
//clock_t start, end;
//float count;

void inpaint_init(int width, int height, char * img, bool * mask, int * debug) {

    memset(confidence, 0, MAX_LEN*MAX_LEN*sizeof(float));
    memset(contour_mask, 0, MAX_LEN*MAX_LEN*sizeof(bool));

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            confidence[LINEAR(i,j)] = mask[LINEAR(i,j)] ? 0.0 : 1.0;
        }
    }
}

int within_c(int x, int y, int width, int height) {
    return x >= 0 && y >= 0 && x < width && y < height;
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

        if (within_c(in_x, in_y, width, height) && mask[in_index]) {
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


            if (within_c(in_x, in_y, width, height) && mask[in_index]) {
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

bool inpaint_step(int width, int height, char * img, bool * mask, int * debug) {

    memset(contour_mask, 0, MAX_LEN*MAX_LEN*sizeof(bool));
    memset(gradient_t, 0, MAX_LEN*MAX_LEN*sizeof(point)); // TODO: Debug
    memset(n_t, 0, MAX_LEN*MAX_LEN*sizeof(point)); // TODO: Debug
    memset(debug, 0, width*height*sizeof(int));

    // 1. CALCULATE CONTOUR
    // ++++++++++++++++++++

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

    // 2. FIND TARGET PATCH
    // ++++++++++++++++++++

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
            int di[8] = {-1, 0, 1, 1,  1,  0, -1, -1};
            int dj[8] = { 1, 1, 1, 0, -1, -1, -1, 0};
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
            point nt = get_ortogonal_to_contour(j, i, mask, width, height);

            // draw the n vector every 6 pixels
            if (i % 3 == 0){
                for(float s = 1; s < 10; s+=0.5){
                    int nni = i + nt.y*s;
                    int nnj = j + nt.x*s;
                    debug[LINEAR(nni,nnj)] = 2;
                }
            }

            // Data
            float data = fabsf(gx_t * nt.x + gy_t * nt.y) / ALPHA;

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

    // 3. FIND SOURCE PATCH
    // ++++++++++++++++++++

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
                        squared_diff += squared_distance3(img + 3*LINEAR(target_i, target_j), img + 3*LINEAR(source_i, source_j));
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

    // 4. COPY
    // +++++++

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

    return 1;
}


void inpainting(char * ptr, int width, int height, bool * mask_ptr, int * debug) {
    inpaint_init(width, height, ptr, mask_ptr, debug);

    while (true) {
        bool is_more = inpaint_step(width, height, ptr, mask_ptr, debug);
        if (!is_more) break;
    }
}



void inpaint_generate_arbitrary_mask(bool * mask, int width, int height) {
    memset(mask, 0, width * height * sizeof(bool));

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (i >= height * 5.0/11.0 && i < height * 6.0/11.0) {
                if (j >= width * 5.0/11.0 && j < width * 6.0/11.0) {
                    mask[LINEAR(i, j)] = true;
                }
            }
        }
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
            int inner_i = clamp(i + ki - kernel_radius, 0, height-1);
            int inner_j = clamp(j + kj  - kernel_radius, 0, width-1);
            float avg = 0;
            for (int ci = 0; ci < 3; ci++) {
                avg += (unsigned char)img[LINEAR3(inner_i, inner_j, ci)]; // Isn't backwards?
            }
            avg /= 3;
            acc += avg * kernel[ki][kj]; // Isn't backwards?
        }
    }

    return acc;
}
