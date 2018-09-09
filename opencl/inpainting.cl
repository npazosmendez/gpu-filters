
__constant int PATCH_RADIUS = 4;

__constant float ALPHA = 255;

#define PI 3.14159265358979323846

#define LINEAR3(position) (3*(((position).y)*get_global_size(0)+((position).x))+((position).z))
#define LINEAR(position) (((position).x)+((position).y)*get_global_size(0))
#define forn(i,n) for(int i=0; i<(n); i++)


// AUXILIARS
// +++++++++

typedef struct point {
    float x, y;
} point;

int within(int2 position, int2 size);
float squared_distance3(__global uchar * p, __global uchar * q);
float norm(float x, float y);
float masked_convolute(int width, int height, __global uchar * img, int i, int j, float kern[], global uchar * mask);
point vector_bisector(float ax, float ay, float bx, float by);

// Checks if 'position' is valid inside a matrix of 'size'
int within(int2 position, int2 size) {
    int2 is_within = 0 <= position && position < size;
    return is_within.x && is_within.y;
}

// Calculates norm of vector
float norm(float x, float y) {
    return sqrt(x * x + y * y);
}

// Calculates squared distance of two vectors in three dimensions
float squared_distance3(__global uchar * p, __global uchar * q) {
    return (p[0] - q[0]) * (p[0] - q[0]) +  \
           (p[1] - q[1]) * (p[1] - q[1]) +  \
           (p[2] - q[2]) * (p[2] - q[2]);
}

// Calculates angle of vectors between vectors (ax, ay) and (bx, by)
point vector_bisector(float ax, float ay, float bx, float by) {
    float b_angle = atan2(ay, ax);
    float a_angle = atan2(by, bx);
    if (b_angle < a_angle) b_angle += 2 * PI;
    float diff_angle = b_angle - a_angle;
    float mid_angle = (diff_angle / 2) + a_angle;
    return (point) { cos(mid_angle), sin(mid_angle) };
}

float masked_convolute(int width, int height, __global uchar * img, int i, int j, float kern[], global uchar * mask) {
    float acc = 0;

    int kernel_radius = 1;
    int kernel_diameter = kernel_radius * 2 + 1;
    for (int ki = 0; ki < kernel_diameter; ki++) {
        for (int kj = 0; kj < kernel_diameter; kj++) {
            int inner_i = clamp(i + ki - kernel_radius, 0, height);
            int inner_j = clamp(j + kj - kernel_radius, 0, width);
            float avg = 0;
            for (int ci = 0; ci < 3; ci++) {
                avg += img[LINEAR3((int3)(inner_j, inner_i, ci))];
            }
            avg /= 3;
            acc += avg * kern[LINEAR((int2)(kj, ki))];
        }
    }

    return acc;
}


// KERNELS
// +++++++

/*
 * Saves in 'contour_mask' which pixels are part of the contour between the masked
 * and unmasked pixels, and which aren't. These will be the next pixels who will be
 * filled.
 */
__kernel void contour(
        __global uchar * mask,
        __global uchar * contour_mask) {

    int2 size = (int2)(get_global_size(0), get_global_size(1));
    int2 pos = (int2)(get_global_id(0), get_global_id(1));

    if (mask[LINEAR(pos)]) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int2 local_pos = pos + (int2)(dx, dy);
                if (within(local_pos, size) && !mask[LINEAR(local_pos)]) {
                    contour_mask[LINEAR(pos)] = true;
                }
            }
        }
    }
}

/*
 * Sets the output buffer 'priorities' with the priority of each patch in the
 * image as potential 'target' patch in a given step of the inpainting algorithm.
 * The patch with the highest priority will be the next patch to be filled.
 */
__kernel void patch_priorities(
        __global uchar * img,
        __global uchar * mask,
        __global uchar * contour_mask,
        __global uchar * confidence,
        __global float * priorities) {

    int width = get_global_size(0);
    int height = get_global_size(1);
    int i = get_global_id(1);
    int j = get_global_id(0);

    bool is_contour = true;

    if (!contour_mask[LINEAR((int2)(j,i))]) {
        is_contour = false; // return?
        goto prioriret;
    }

    /*
     * Every patch has a certain 'priority'. At each step of the algorithm, the patch with
     * the highest priority of the contour is selected as the next patch to be filled.
     * The priority is equal to a 'data' coefficient multiplied by a 'confidence' coefficient
     */

    // Confidence
    // The confidence is the normalized sum of the confidence of all pixels inside the patch
    // Masked pixels start with 0 confidence, while the other pixels start with 1 confidence
    float sum = 0;

    int bot_l = max(j - PATCH_RADIUS, 0);
    int top_l = min(j + PATCH_RADIUS + 1, width);
    int bot_k = max(i - PATCH_RADIUS, 0);
    int top_k = min(i + PATCH_RADIUS + 1, height);

    for (int k = bot_k; k < top_k; k++) {
        for (int l = bot_l; l < top_l; l++) {
            sum += confidence[LINEAR((int2)(l,k))];
        }
    }

    confidence[LINEAR((int2)(j,i))] = sum / (2 * PATCH_RADIUS + 1) * (2 * PATCH_RADIUS + 1);

    // Gradient
    // According to impl I should take one gradient, according to paper I should take the max in the patch.
    // Also, according to both I need to look in the whole image, not just in the source. That's weird.
    // Following my gut, I'll calculate only one gradient
    float sobel_kernel_x[9] = { \
                    -1, 0, 1, \
                    -2, 0, 2, \
                    -1, 0, 1  \
                };
    float sobel_kernel_y[9] = { \
                    -1, -2, -1, \
                     0,  0,  0, \
                     1,  2,  1  \
                };
    float gx = masked_convolute(width, height, img, i, j, sobel_kernel_x, mask);
    float gy = masked_convolute(width, height, img, i, j, sobel_kernel_y, mask);
    float gx_t = -gy;
    float gy_t = gx;

    //point gradient_t = (point) { .x = gx_t, .y = gy_t }; // TODO: I'm not using this value, lol


    // Normal
    //  Easy way: Take spaces between edges (take the one who yields higher data)
    int di[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
    int dj[8] = {1, 1, 1, 0, -1, -1, -1, 0};
    int k_border = -1;

    // Find first edge/masked pixel
    for (int k = 0; k < 8; k++) {
        if (!within((int2)(j + dj[k], i + di[k]), (int2)(width, height)) ||  \
            mask[LINEAR((int2)(j + dj[k], i + di[k]))]) {
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
            bool is_out = !within((int2)(j + dj[k], i + di[k]), (int2)(width, height));
            if ((is_out && !is_out_prev) || (mask[LINEAR((int2)(j + dj[k], i + di[k]))])){
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

    //point n_t = (point) { .x = nx_max, .y = ny_max }; // TODO: Debug

    // Data
    float data = fabs(gx_t * nx_max + gy_t * ny_max) / ALPHA;

    // Priority
prioriret:
    priorities[LINEAR((int2)(j,i))] = is_contour ? confidence[LINEAR((int2)(j,i))] * data : -1.0f;
}


/*
 * Sets the output buffer 'diffs' so that 'diffs(x, y)' is equal to the difference
 * between the patch centered at '(x, y)' and the patch centered at 'target_pos'.
 * For incomplete patches, it sets FLT_MAX (so that it doesn't affect the minimum
 * difference).
 */
__kernel void target_diffs(
        __global uchar * img,
        __global uchar * mask,
        int2 target_pos,
        __global float * diffs) {

    int2 size = (int2)(get_global_size(0), get_global_size(1));
    int2 pos = (int2)(get_global_id(0), get_global_id(1));

    float squared_diff = 0;
    bool full_patch = true;

    for (int px = -PATCH_RADIUS; px <= PATCH_RADIUS; px++) {
        for (int py = -PATCH_RADIUS; py <= PATCH_RADIUS; py++) {

            int2 local_pos        = (int2)(pos.x + px       , pos.y + py);
            int2 target_local_pos = (int2)(target_pos.x + px, target_pos.y + py);

            // TODO: Ideally, I'd early return here, but it wouldn't be working
            full_patch = full_patch && (within(local_pos, size) && !mask[LINEAR(local_pos)]);

            if (within(target_local_pos, size) && !mask[LINEAR(target_local_pos)]) {
                squared_diff += squared_distance3(img + 3*LINEAR(target_local_pos),  \
                                                  img + 3*LINEAR(local_pos));
            }
        }
    }

    diffs[LINEAR(pos)] = full_patch ? squared_diff : FLT_MAX;
}
