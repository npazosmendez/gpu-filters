
#define LINEAR(x,y) ((x)+(y)*get_global_size(0))

__constant int PATCH_RADIUS = 4;

int within(int value, int minimum, int maximum);
float squared_distance3(__global uchar * p, __global uchar * q);

// Checks if value is inside the range [minimum, maximum)
int within(int value, int minimum, int maximum) {
    return value >= minimum && value < maximum;
}

// Calculates squared distance of two vectors in three dimensions
float squared_distance3(__global uchar * p, __global uchar * q) {
    return (p[0] - q[0]) * (p[0] - q[0]) +  \
           (p[1] - q[1]) * (p[1] - q[1]) +  \
           (p[2] - q[2]) * (p[2] - q[2]);
}


__kernel void target_diffs(
        __global uchar * img,
        __global uchar * mask,
        int2 best_target,
        __global float * diffs) {

    int width = get_global_size(0);
    int height = get_global_size(1);
    int x = get_global_id(0);
    int y = get_global_id(1);

    // Sum
    float squared_diff = 0;
    for (int px = -PATCH_RADIUS; px <= PATCH_RADIUS; px++) {
        for (int py = -PATCH_RADIUS; py <= PATCH_RADIUS; py++) {

            int real_target_x = best_target.x + px;
            int real_target_y = best_target.y + py;
            int real_x = x + px;
            int real_y = y + py;

            if (!within(real_x, 0, width) ||
                !within(real_y, 0, height) ||
                mask[LINEAR(real_x, real_y)]) {
                squared_diff += 100000000.0;
                // NOT WORKING, WEIRD
                //diffs[LINEAR(x, y)] = 1.0;
                //return;
            }

            //assert()

            if (within(real_target_x, 0, width) &&  \
                within(real_target_y, 0, height) && \
                !mask[LINEAR(real_target_x, real_target_y)]) {
                squared_diff += squared_distance3(img + 3*LINEAR(real_target_x, real_target_y), img + 3*LINEAR(real_x, real_y));
            }
        }
    }


    // Output
    diffs[LINEAR(x, y)] = squared_diff;
}
