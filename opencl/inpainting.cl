
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
    priorities[LINEAR((int2)(get_global_id(0), get_global_id(1)))] = 3.0f;
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
