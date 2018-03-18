__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

inline float Gx(__read_only image2d_t image, int i, int j){
    const int2 coor = {i,j};
    return 255 - read_imagef(image, sampler, coor).x;
}

__kernel void canny(
        __read_only image2d_t image,
        __global float * dst
    ) {
    int i = get_global_id(0);
    int j = get_global_id(1);
    float temp = Gx(image, i, j);

    dst[i+j*get_global_size(0)] = temp;

}
