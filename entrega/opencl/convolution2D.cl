__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

__kernel void convolution2D(
        __read_only image2d_t image,
        __private int ksize,
        __read_only image2d_t kern,
        __global float * dst
    ) {
    int i = get_global_id(0);
    int j = get_global_id(1);

    bool borde = j-ksize/2 < 0 | (j + ksize/2) > (get_global_size(1)-1);
    if ( ! borde){
        /* kernel loop */
        float temp = 0;
        for (int ik = 0; ik < ksize; ik++) {
            for (int jk = 0; jk < ksize; jk++) {
                const int2 imcoor = {i+ik-ksize/2, j+jk-ksize/2};
                const int2 kcoor = {ik, jk};
                temp = temp + read_imagef(image, sampler, imcoor).x*read_imagef(kern, sampler, kcoor).x;
            }
        }
        dst[i+j*get_global_size(0)] = temp;
    }else{
        // TODO
    }
}
