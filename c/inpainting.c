#include "c-filters.h"
#include "stdio.h"
#include "stdbool.h"
#include "string.h"
#include "math.h"
#include "time.h"
#include "float.h"
#include "cutils.h"

int PATCH_RADIUS = 4;
float ALPHA = 255;

point vector_bisector(float ax, float ay, float bx, float by);
float masked_convolute(int width, int height, char (*img)[width][3], int i, int j, float kernel[3][3], bool (*mask)[width]);

#define MAX_LEN 2000
bool contour_mask[MAX_LEN][MAX_LEN];
float confidence[MAX_LEN][MAX_LEN];
point gradient_t[MAX_LEN][MAX_LEN];
point n_t[MAX_LEN][MAX_LEN];

/*
    Outline:
    - Calculate border
    - Calculate priority for all pixels in the border
    - Find pixel with most priority
    - Find patch most similar with the patch of the previous pixel
    - Copy patches
    - Loop until image is filled
*/


clock_t start, end;
float count;


void init(int width, int height, char (*img)[width][3], bool (*mask)[width]) {
    memset(confidence, 0, MAX_LEN*MAX_LEN*sizeof(float));
    memset(contour_mask, 0, MAX_LEN*MAX_LEN*sizeof(bool));

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            confidence[i][j] = mask[i][j] ? 0.0 : 1.0;
        }
    }
}

bool step(int width, int height, char (*img)[width][3], bool (*mask)[width]) {
     memset(contour_mask, 0, MAX_LEN*MAX_LEN*sizeof(bool));
     // TODO: This is just for exposing gradient terms outside
     memset(gradient_t, 0, MAX_LEN*MAX_LEN*sizeof(point));
     memset(n_t, 0, MAX_LEN*MAX_LEN*sizeof(point));

     // 1. Calculate contour
     start = clock();

     for (int i = 0; i < height; i++) {
         for (int j = 0; j < width; j++) {
             if (mask[i][j]) {
                 for (int di = -1; di <= 1; di++) {
                     for (int dj = -1; dj <= 1; dj++) {
                         if (within(i + di, 0, height) && within(j + dj, 0, width)) {
                             if (!mask[i + di][j + dj]) {
                                 contour_mask[i][j] = true;
                             }
                         }
                     }
                 }
             }
         }
     }

     int contour_size = 0;
     for (int i = 0; i < height; i++) for (int j = 0; j < width; j++) {
         if (contour_mask[i][j] == true) contour_size++;
     }

     if (contour_size == 0) {
         return 0;
     }

     end = clock();
     count = (float)(end - start) / CLOCKS_PER_SEC;
     printf("Contour (size = %d): %f\n", contour_size, count);


     // 2. Find target patch 
     start = clock();

     int max_i = -1;
     int max_j = -1;
     float max_priority = -1.0;

     for (int i = 0; i < height; i++) {
         for (int j = 0; j < width; j++) {

             if (!contour_mask[i][j]) {
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

             // TODO: This is just for exposing gradient terms outside
             gradient_t[i][j] = (point) { .x = gx_t, .y = gy_t };
             
             
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

             // TODO: Debugging purposes
             n_t[i][j] = (point) { .x = nx_max, .y = ny_max };

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

     end = clock();
     count = (float)(end - start) / CLOCKS_PER_SEC;
     printf("Target patch (%d, %d): %f\n", max_i, max_j, count);

     // 3. Find source patch
     start = clock();

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
                         mask[source_i][source_j]) {
                         valid = false;
                         goto out;
                     }

                     if (within(target_i, 0, height) &&  \
                         within(target_j, 0, width) &&   \
                         !mask[target_i][target_j]) {
                         squared_diff += squared_distance3(img[target_i][target_j], img[source_i][source_j]);
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

     end = clock();
     count = (float)(end - start) / CLOCKS_PER_SEC;
     printf("Source patch(%d, %d): %f\n", max_source_i, max_source_j, count);

     // 4. Copy
     start = clock();

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
                 mask[target_i][target_j]) {
                 img[target_i][target_j][0] = img[source_i][source_j][0];
                 img[target_i][target_j][1] = img[source_i][source_j][1];
                 img[target_i][target_j][2] = img[source_i][source_j][2];
                 mask[target_i][target_j] = false;
             }
         }
     }

     end = clock();
     count = (float)(end - start) / CLOCKS_PER_SEC;
     printf("Copy: %f\n", count);

     return 1;
}


void inpainting(char * ptr, int width, int height, bool * mask_ptr) {

    char (*img)[width][3] = (char (*)[width][3]) ptr;
    bool (*mask)[width] = (bool (*)[width]) mask_ptr;

    init(width, height, img, mask);

    while (true) {
        bool is_more = step(width, height, img, mask);
        if (!is_more) break;
    }
}



void generate_arbitrary_mask(bool * dst, int width, int height) {
    memset(dst, 0, width * height * sizeof(bool));

    bool (*mask)[width] = (bool (*)[width]) dst;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (i >= height * 5.0/11.0 && i < height * 6.0/11.0) {
                if (j >= width * 5.0/11.0 && j < width * 6.0/11.0) {
                    mask[i][j] = true;
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