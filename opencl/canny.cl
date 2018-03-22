
#define LINEAR(i,j) (i)+(j)*get_global_size(0)

__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

/******************************************************************/
/******************** Misc auxiliary functions ********************/
/******************************************************************/

inline float buff_rgb_intensity(__global const uchar * src, int i, int j) {
    // Reads RGB pixel from src and returns its intensity as float
    uchar3 rgb = vload3(LINEAR(i,j), src);
    /* NOTE: letting 'src' be uchar3* doesn't work as expected, as a
    3-component vector size is 4*sizeof(component). That's what vload3
    is for.
    https://stackoverflow.com/questions/30421904/how-vector-pointers-work-in-opencl
    */
    return (float)(rgb.x + rgb.y + rgb.z)/3;
}

inline float clipf(float x, float L, float H){
    // Clips float value in range [L,H]
    float res = (x < L) ? L : x;
    res = (res > H) ? H : res;
    return res;
}

inline float norm2(float x, float y){
    // Returns norm2 of vector (x,y)
    return native_sqrt(pow(x,2) + pow(y,2));
}

inline float x_gradient(__read_only image2d_t image, int i, int j){
    // Returns the horizontal gradient of 'image' at (i,j) (Sobel filter)
    // NOTE: too many reads. Consider using shared memory in caller function.
    float Gx = 0;
    Gx -= 2*read_imagef(image, sampler, (int2){i-1,j}).x;
    Gx -= 1*read_imagef(image, sampler, (int2){i-1,j+1}).x;
    Gx -= 1*read_imagef(image, sampler, (int2){i-1,j-1}).x;

    Gx += 2*read_imagef(image, sampler, (int2){i+1,j}).x;
    Gx += 1*read_imagef(image, sampler, (int2){i+1,j+1}).x;
    Gx += 1*read_imagef(image, sampler, (int2){i+1,j-1}).x;

    return Gx;
}

inline float y_gradient(__read_only image2d_t image, int i, int j){
    // Returns the vertical gradient of 'image' at (i,j) (Sobel filter)
    // NOTE: too many reads. Consider using shared memory in caller function.
    float Gy = 0;
    Gy -= 2*read_imagef(image, sampler, (int2){i,j-1}).x;
    Gy -= 1*read_imagef(image, sampler, (int2){i-1,j-1}).x;
    Gy -= 1*read_imagef(image, sampler, (int2){i+1,j-1}).x;

    Gy += 2*read_imagef(image, sampler, (int2){i,j+1}).x;
    Gy += 1*read_imagef(image, sampler, (int2){i-1,j+1}).x;
    Gy += 1*read_imagef(image, sampler, (int2){i+1,j+1}).x;

    return Gy;
}

inline int2 gradient_neighbour1(int i, int j, int angle){
    // Returns (i,j)'s neighbour1's coordinate in 'angle' direction
    // NOTE 1: angle is codificated as follows: 0=-90°, 1=-45°, 2=0°, 3=45°
    // NOTE 2: neighbours are divided in two groups. This function aims to group 1
    // 2  2  1
    // 2 (X) 1
    // 2  1  1
    // NOTE 3: Yes, this is horrible. Will probably fix in the future. For now,
    // at least the caller function stays cleaner.
    int res_i, res_j;
    switch(angle){
        case 0:
        res_i = i;
        res_j = j+1;
        break;
        case 1:
        res_i = i+1;
        res_j = j+1;
        break;
        case 2:
        res_i = i+1;
        res_j = j;
        break;
        case 3:
        res_i = i+1;
        res_j = j-1;
        break;
        default:
        break;
    }
    return (int2){res_i,res_j};
}

inline int2 gradient_neighbour2(int i, int j, int angle){
    // Returns (i,j)'s neighbour2's coordinate in 'angle' direction
    int res_i, res_j;
    switch(angle){
        case 0:
        res_i = i;
        res_j = j-1;
        break;
        case 1:
        res_i = i-1;
        res_j = j-1;
        break;
        case 2:
        res_i = i-1;
        res_j = j;
        break;
        case 3:
        res_i = i-1;
        res_j = j+1;
        break;
        default:
        break;
    }
    return (int2){res_i,res_j};
}


/***********************************************************/
/******************** Kernel functions  ********************/
/***********************************************************/

__kernel void intensity_gauss_filter(
        __global const uchar * src,
        __write_only image2d_t image
    ) {
    /* Takes the RGB image from buffer 'src' and stores its intensity
    channel in the texture 'image' as floats, smoothened with
    a gaussian filter. */

    const int i = get_global_id(0);
    const int j = get_global_id(1);

    float intensity = 0;

    /* ********* 3x3 gaussian kernel ******** */
    /*
    intensity += 4*buff_rgb_intensity(src,i,j);

    intensity += 2*buff_rgb_intensity(src,i-1,j);
    intensity += 2*buff_rgb_intensity(src,i+1,j);
    intensity += 2*buff_rgb_intensity(src,i,j-1);
    intensity += 2*buff_rgb_intensity(src,i,j+1);

    intensity += 1*buff_rgb_intensity(src,i-1,j-1);
    intensity += 1*buff_rgb_intensity(src,i-1,j+1);
    intensity += 1*buff_rgb_intensity(src,i+1,j-1);
    intensity += 1*buff_rgb_intensity(src,i+1,j+1);

    intensity /= 16;
    */
    /* ****************************************** */

    /* ********* 5x5 gaussian kernel ******** */
    // NOTE: too much global memory accesses. Definitely optimizable.
    intensity += 41*buff_rgb_intensity(src,i,j);

    intensity += 26*buff_rgb_intensity(src,i-1,j);
    intensity += 26*buff_rgb_intensity(src,i+1,j);
    intensity += 26*buff_rgb_intensity(src,i,j-1);
    intensity += 26*buff_rgb_intensity(src,i,j+1);

    intensity += 16*buff_rgb_intensity(src,i-1,j-1);
    intensity += 16*buff_rgb_intensity(src,i-1,j+1);
    intensity += 16*buff_rgb_intensity(src,i+1,j-1);
    intensity += 16*buff_rgb_intensity(src,i+1,j+1);

    intensity += 7*buff_rgb_intensity(src,i-2,j);
    intensity += 7*buff_rgb_intensity(src,i+2,j);
    intensity += 7*buff_rgb_intensity(src,i,j-2);
    intensity += 7*buff_rgb_intensity(src,i,j+2);

    intensity += 4*buff_rgb_intensity(src,i-2,j+1);
    intensity += 4*buff_rgb_intensity(src,i-2,j-1);
    intensity += 4*buff_rgb_intensity(src,i+2,j+1);
    intensity += 4*buff_rgb_intensity(src,i+2,j-1);
    intensity += 4*buff_rgb_intensity(src,i+1,j-2);
    intensity += 4*buff_rgb_intensity(src,i-1,j-2);
    intensity += 4*buff_rgb_intensity(src,i+1,j+2);
    intensity += 4*buff_rgb_intensity(src,i-1,j+2);


    intensity += 1*buff_rgb_intensity(src,i+2,j+2);
    intensity += 1*buff_rgb_intensity(src,i+2,j-2);
    intensity += 1*buff_rgb_intensity(src,i-2,j+2);
    intensity += 1*buff_rgb_intensity(src,i-2,j-2);

    intensity /= 273;
    /* ****************************************** */

    float4 color = {intensity, intensity, intensity, intensity};
    write_imagef(image, (int2){i,j}, color);
}


__kernel void max_edges(
    __read_only image2d_t image,
    __global float * dst,
    float uthreshold,
    float lthreshold
    ) {
        /* Performs the following steps of Canny algorithm:
            * Compute Sobel gradient
            * Supress non-max and below thresh pixels
        */
        int i = get_global_id(0);
        int j = get_global_id(1);

        float Gx = x_gradient(image,i,j);
        float Gy = y_gradient(image,i,j);

        float Gm = norm2(Gx,Gy);
        float Go_f = (atanpi(Gy/Gx)+0.5)*4.0; // [-PI/2 ; PI/2] normalized to [0 ; 4]
        //
        int Go = (int)round(Go_f) %4; // 0 / 1 / 2 / 3 = -90° / -45° / 0° / 45°

        // Non-max supression
        float Gm1, Gm2;
        int2 n1 = gradient_neighbour1(i,j,Go);
        int2 n2 = gradient_neighbour2(i,j,Go);
        Gm1 = norm2(x_gradient(image,n1.x,n1.y),y_gradient(image,n1.x,n1.y));
        Gm2 = norm2(x_gradient(image,n2.x,n2.y),y_gradient(image,n2.x,n2.y));

        if((Gm1 > Gm) | (Gm2 > Gm)| (Gm < lthreshold)){
            Gm = 0; // definitely no
        }else if((Gm >= lthreshold) & (Gm < uthreshold)){
            Gm = 50; // maybe
        }else{
            Gm = 255; // definitely yes
        }
        dst[LINEAR(i,j)] = Gm;

        // TODO
        /*
        Brainstorming:
        Maybe a global float buffer is not needed. This kernel could directly write on the uchar3 buffer directly 3
        possible values: not-an-edge, candidate-edge, edge.
        With that information only, the hysterysis can be done.
        A final kernel could swipe candidate-edges after the
        hysteresis.
        Possible downside: working with triple-integers instead
        of a single float.

        */

}



__kernel void hysteresis(
        __global int * intbool,
        __global float * src
    ) {
        int i = get_global_id(0);
        int j = get_global_id(1);
        if(src[LINEAR(i,j)] == 50){
            bool expand = false;
            // NOTE: should I only check the gradient direction?
            // There are different versions
            expand |= (src[LINEAR(i-1,j)]==255);
            expand |= (src[LINEAR(i-1,j-1)]==255);
            expand |= (src[LINEAR(i-1,j+1)]==255);
            expand |= (src[LINEAR(i+1,j)]==255);
            expand |= (src[LINEAR(i+1,j+1)]==255);
            expand |= (src[LINEAR(i+1,j-1)]==255);
            expand |= (src[LINEAR(i,j-1)]==255);
            expand |= (src[LINEAR(i,j+1)]==255);
            if(expand){
                src[LINEAR(i,j)] = 255;
                if (*intbool == 0) atomic_cmpxchg (intbool, 0, 1);
            }
        }
}



__kernel void intensityFloat_to_rgbChar(
        __global const float * src,
        __global uchar * dst
    ) {
        const int i = get_global_id(0);
        const int j = get_global_id(1);
        uchar intensity = src[LINEAR(i,j)];
        // uchar intensity = src[LINEAR(i,j)]==255 ? 255 : 0;
        uchar3 rgb = {intensity,intensity,intensity};
        vstore3(rgb,LINEAR(i,j),dst);
}






//
