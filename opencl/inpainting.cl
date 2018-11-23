
#define PATCH_RADIUS 4
__constant int2 POSITIVE_PATCH_CORNER = (int2)(PATCH_RADIUS, PATCH_RADIUS);
__constant int2 NEGATIVE_PATCH_CORNER = (int2)(-PATCH_RADIUS, -PATCH_RADIUS);

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
int within_c(int x, int y, int width, int height);
float squared_distance3(__global uchar * p, __global uchar * q);
float norm(float x, float y);
float masked_convolute(int width, int height, __global uchar * img, int i, int j, float kern[], global uchar * mask);
point vector_bisector(float ax, float ay, float bx, float by);

// Checks if 'position' is valid inside a matrix of 'size'
int within(int2 position, int2 size) {
    int2 is_within = 0 <= position && position < size;
    return is_within.x && is_within.y;
}

int within_c(int x, int y, int width, int height) {
    return x >= 0 && y >= 0 && x < width && y < height;
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
            int inner_i = clamp(i + ki - kernel_radius, 0, height-1);
            int inner_j = clamp(j + kj - kernel_radius, 0, width-1);
            float avg = 0;
            for (int ci = 0; ci < 3; ci++) {
                avg += img[LINEAR3((int3)(inner_j, inner_i, ci))];
            }
            avg /= 3;
            acc += avg * kern[kj+ki*kernel_diameter];
        }
    }

    return acc;
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


// KERNELS
// +++++++

/*
 * Sets the output buffer 'priorities' with the priority of each patch in the
 * image as potential 'target' patch in a given step of the inpainting algorithm.
 * The patch with the highest priority will be the next patch to be filled.
 */
__kernel void patch_priorities(
        __global uchar * img,
        __global uchar * mask,
        __global float * confidence,
        __global float * priorities) {

    // STEP 1

    int2 size = (int2)(get_global_size(0), get_global_size(1));
    int2 pos = (int2)(get_global_id(0), get_global_id(1));

    bool is_contour = false;
    if (mask[LINEAR(pos)]) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int2 local_pos = pos + (int2)(dx, dy);
                if (within(local_pos, size) && !mask[LINEAR(local_pos)]) {
                    is_contour = true;
                }
            }
        }
    }

    if (!is_contour) {
        priorities[LINEAR(pos)] = -1.0f;
        return;
    }

    // STEP 2

    int width = get_global_size(0);
    int height = get_global_size(1);
    int i = get_global_id(1);
    int j = get_global_id(0);

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

    confidence[LINEAR((int2)(j,i))] = sum / ((2 * PATCH_RADIUS + 1) * (2 * PATCH_RADIUS + 1));

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

    // Normal
    point nt = get_ortogonal_to_contour(pos.x, pos.y, mask, size.x, size.y);

    // Data
    float data = fabs(gx_t * nt.x + gy_t * nt.y) / ALPHA;

    // Priority
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

/*
 * Fills the target patch with the source patch
 */
__kernel void copy(
        __global uchar * img,
        __global uchar * mask,
        int2 target_pos,
        int2 source_pos,
        __global float * confidence) {

    int2 size = (int2)(get_global_size(0), get_global_size(1));
    int2 target_local_pos = (int2)(get_global_id(0), get_global_id(1));

    int2 diff = target_local_pos - target_pos;
    int2 source_local_pos = source_pos + diff;

    int2 in_patch = NEGATIVE_PATCH_CORNER <= diff && diff <= POSITIVE_PATCH_CORNER;

    if (within(source_local_pos, size) &&  \
        mask[LINEAR(target_local_pos)] &&  \
        in_patch.x && in_patch.y) {
        img[3*LINEAR(target_local_pos)+0] = img[3*LINEAR(source_local_pos)+0];
        img[3*LINEAR(target_local_pos)+1] = img[3*LINEAR(source_local_pos)+1];
        img[3*LINEAR(target_local_pos)+2] = img[3*LINEAR(source_local_pos)+2];
        mask[LINEAR(target_local_pos)] = false;
        confidence[LINEAR(target_local_pos)] = confidence[LINEAR(target_pos)];
    }
}
