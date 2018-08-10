
#define LINEAR(x,y) (x)+(y)*get_global_size(0)

__kernel void add (__global const int* v1, __global int* v2) {
    const int id = get_global_id(0);
    v2[id] = 2 * v1[id];
}

__kernel void target_diffs(
        __global uchar * img,
        __global uchar * mask,
        const int target_i,
        const int target_j,
        __global int * best_source) { // is it really __global? it's on the host...

    int width = get_global_size(0);
    int height = get_global_size(1);
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x == 10 && y == 10) {
        int img_r = img[3*(y*width+x)+0];
        int img_g = img[3*(y*width+x)+1];
        int img_b = img[3*(y*width+x)+2];
        int mask_c = mask[y*width+x];
        printf("IN KERNEL i:%i, j:%i, img:(%i,%i,%i), mask:%i\n", target_i, target_j, img_r, img_g, img_b, mask_c);
    }

    best_source[0] = 3;
    best_source[1] = 4;
}
