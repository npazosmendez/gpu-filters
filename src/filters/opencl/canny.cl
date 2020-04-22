
#define LINEAR(x,y) (x)+(y)*get_global_size(0)

#define PI 3.14159265358979323846

#define EDGE 255
#define CANDIDATE 100
#define NOT_EDGE 0

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


inline float norm2(float x, float y){
    // Returns norm2 of vector (x,y)
    return native_sqrt(x*x + y*y);
}

inline float x_gradient(__read_only image2d_t image, int x, int y){
    // Returns the horizontal gradient of 'image' at (x,y) (Sobel filter)
    // NOTE: too many reads. Consider using shared memory in caller function.
    float Gx = 0;
    Gx -= 2*read_imagef(image, sampler, (int2){x-1,y}).x;
    Gx -= 1*read_imagef(image, sampler, (int2){x-1,y+1}).x;
    Gx -= 1*read_imagef(image, sampler, (int2){x-1,y-1}).x;

    Gx += 2*read_imagef(image, sampler, (int2){x+1,y}).x;
    Gx += 1*read_imagef(image, sampler, (int2){x+1,y+1}).x;
    Gx += 1*read_imagef(image, sampler, (int2){x+1,y-1}).x;

    return Gx;
}

inline float y_gradient(__read_only image2d_t image, int x, int y){
    // Returns the vertical gradient of 'image' at (x,y) (Sobel filter)
    // NOTE: too many reads. Consider using shared memory in caller function.
    float Gy = 0;
    Gy -= 2*read_imagef(image, sampler, (int2){x,y-1}).x;
    Gy -= 1*read_imagef(image, sampler, (int2){x-1,y-1}).x;
    Gy -= 1*read_imagef(image, sampler, (int2){x+1,y-1}).x;

    Gy += 2*read_imagef(image, sampler, (int2){x,y+1}).x;
    Gy += 1*read_imagef(image, sampler, (int2){x-1,y+1}).x;
    Gy += 1*read_imagef(image, sampler, (int2){x+1,y+1}).x;

    return Gy;
}

inline int2 gradient_neighbour1(int x, int y, int angle){
    // Returns (x,y)'s neighbour1's coordinate in 'angle' direction
    // NOTE 2: neighbours are divided in two groups. This function aims to group 1
    // 2  2  1
    // 2 (X) 1
    // 2  1  1
    // NOTE 3: Yes, this is horrible. Will probably fix in the future. For now,
    // at least the caller function stays cleaner.
    int res_x, res_y;
    switch(angle){
        case 0:
        res_x = x+1;
        res_y = y;
        break;
        case 45:
        res_x = x+1;
        res_y = y+1;
        break;
        case 90:
        res_x = x;
        res_y = y+1;
        break;
        case 135:
        res_x = x+1;
        res_y = y-1;
        break;
        default:
        printf("ERROR!");
        break;
    }
    return (int2){res_x,res_y};
}

inline int2 gradient_neighbour2(int x, int y, int angle){
    // Returns (x,y)'s neighbour2's coordinate in 'angle' direction
    int res_x, res_y;
    switch(angle){
        case 0:
        res_x = x-1;
        res_y = y;
        break;
        case 45:
        res_x = x-1;
        res_y = y-1;
        break;
        case 90:
        res_x = x;
        res_y = y-1;
        break;
        case 135:
        res_x = x-1;
        res_y = y+1;
        break;
        default:
        printf("ERROR!");
        break;
    }
    return (int2){res_x,res_y};
}


inline int roundDirection(float x, float y){
    // Gets direction of vector (x,y) rounded to 0/45/90/135
    float arctan = atan2(y,x);
    float degr = arctan * 180.0 / PI;
    int res = (int)round(degr / 360 * 8) * 360 / 8;
    res = (res + 360) % 180;

    return res;

}

/***********************************************************/
/******************** Kernel functions  ********************/
/***********************************************************/

__kernel void intensity_gauss_filter(
        __global const uchar * src,
        __write_only image2d_t image,
        int width,
        int height
    ) {
    /* Takes the RGB image from buffer 'src' and stores its intensity
    channel in the texture 'image' as floats, smoothened with
    a gaussian filter. */

    const int i = get_global_id(0);
    const int j = get_global_id(1);

    if((i-2) < 0 || (i+2) > width) return;
    if((j-2) < 0 || (j+2) > height) return;

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
        int x = get_global_id(0);
        int y = get_global_id(1);

        int2 image_dim = get_image_dim(image);

        // Need neighbours gradients -> +-2
        if((x-2) < 0 || (x+2) > image_dim.x) return;
        if((y-2) < 0 || (y+2) > image_dim.y) return;

        float Gx = x_gradient(image,x,y);
        float Gy = y_gradient(image,x,y);

        float Gm = norm2(Gx,Gy);
        int Go = roundDirection(Gx,Gy);

        // Non-max supression
        float Gm1, Gm2;
        int2 n1 = gradient_neighbour1(x,y,Go);
        int2 n2 = gradient_neighbour2(x,y,Go);
        Gm1 = norm2(x_gradient(image,n1.x,n1.y),y_gradient(image,n1.x,n1.y));
        Gm2 = norm2(x_gradient(image,n2.x,n2.y),y_gradient(image,n2.x,n2.y));

        float temp = NOT_EDGE;
        if(Gm >= Gm1 && Gm >= Gm2){
            /* Is local max */
            if(Gm >= uthreshold){
                // An edge for sure
                temp = EDGE;
            }else if(Gm > lthreshold){
                // May be an edge
				temp = CANDIDATE;
            }
        }

        dst[LINEAR(x,y)] = temp;
}



__kernel void hysteresis(
        __global int * intbool,
        __global float * src,
        int width,
        int height
    ) {
        int i = get_global_id(0);
        int j = get_global_id(1);

        if((i-1) < 0 || (i+1) > width) return;
        if((j-1) < 0 || (j+1) > height) return;

        if(src[LINEAR(i,j)] == CANDIDATE){
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
                src[LINEAR(i,j)] = EDGE;
                if (*intbool == 0) atomic_cmpxchg (intbool, 0, 1);
            }
        }
}



__kernel void floatEdges_to_RGBChar(
        __global const float * src,
        __global uchar * dst
    ) {
        const int i = get_global_id(0);
        const int j = get_global_id(1);
        uchar intensity = src[LINEAR(i,j)]== EDGE ? 255 : 0;
        uchar3 rgb = {intensity,intensity,intensity};
        vstore3(rgb,LINEAR(i,j),dst);
}






//
