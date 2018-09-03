
__constant int PATCH_RADIUS = 4;

#define LINEAR(position) (((position).x)+((position).y)*get_global_size(0))

// AUXILIARS
// +++++++++

// Checks if 'position' is valid inside a matrix of 'size'
int within(int2 position, int2 size) {
    int2 is_within = 0 <= position && position < size;
    return is_within.x && is_within.y;
}

// Calculates squared distance of two vectors in three dimensions
float squared_distance3(__global uchar * p, __global uchar * q) {
    return (p[0] - q[0]) * (p[0] - q[0]) +  \
           (p[1] - q[1]) * (p[1] - q[1]) +  \
           (p[2] - q[2]) * (p[2] - q[2]);
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
        __global float * priorities) {

    int width = get_global_size(0);
    int height = get_global_size(1);
    int i = get_global_id(1);
    int j = get_global_id(0);

    bool is_contour = true;

    if (!contour_mask[LINEAR(i,j)]) {
        is_contour = false; // return?
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
    priorities[LINEAR(i,j)] = confidence[LINEAR(i,j)] * data;
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

            if (!within(local_pos, size) || mask[LINEAR(local_pos)]) full_patch = false;

            if (within(target_local_pos, size) && !mask[LINEAR(target_local_pos)]) {
                squared_diff += squared_distance3(img + 3*LINEAR(target_local_pos),  \
                                                  img + 3*LINEAR(local_pos));
            }
        }
    }

    diffs[LINEAR(pos)] = full_patch ? squared_diff : FLT_MAX;
}
