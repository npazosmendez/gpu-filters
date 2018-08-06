#include "c-filters.h"
#include "math.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "cutils.h"

#define M_PI 3.14159265358979323846

#define EDGE 255
#define CANDIDATE 100
#define NOT_EDGE 0

// Mmmm I don't really like macros
#define LINEAR(y,x) (y)*width+(x)

// Global Canny variables
bool C_canny_initialized = false;
float * img_inten;
float * img_smooth;
float * Gx;
float * Gy;

/* ******************* */
/* Auxiliary Functions */
/* ******************* */

void initCannyC(int width, int height){
    // Initialize buffers for Canny algorithm
    img_inten = (float *)malloc(width*height*sizeof(float));
    img_smooth = (float*)malloc(width*height*sizeof(float));
    Gx = (float *)malloc(width*height*sizeof(float));
    Gy = (float *)malloc(width*height*sizeof(float));

    C_canny_initialized = true;
}

int roundDirection(float x, float y){
    // Gets direction of vector (x,y) rounded to 0/45/90/135
    float arctan = atan2(y,x);
    float degrees = arctan * 180.0 / M_PI;
    int res = (int)round(degrees / 360 * 8) * 360 / 8;
    res = (res + 360) % 180;
    /* NOTE: (!!!) as the +x axis goes to the right and +y goes down (because of
    how an image is stored), angles must be interpreted in clockwise direction.
    For instance, 45° would be what we normally refer as -45°. This could be
    changed by inverting the values in the sobel filters, but I prefer to make
    indexes match the axes. */
    return res;

}

bool isLocalmax(float* data, int width, int height, int y, int x, int direction){
    // Returns 'true' iff data at (x,y) reaches a local max in 'direction' direction
    /* Pre-condition:
    y+-1, x+-1 are in 'data' range
    'direction' is one of these: 0,45,90,135
    */
    float pixel = data[LINEAR(y,x)];
    bool res;
    switch (direction) {
        case 0:
        res = (pixel >= data[LINEAR(y,x+1)] && pixel >= data[LINEAR(y,x-1)]);
        break;
        case 45: // remember this can be seen as -45°
        res = (pixel >= data[LINEAR(y+1,x+1)] && pixel >= data[LINEAR(y-1,x-1)]);
        break;
        case 90:
        res = (pixel >= data[LINEAR(y+1,x)] && pixel >= data[LINEAR(y-1,x)]);
        break;
        case 135: // and this as 225 /-135
        res = (pixel >= data[LINEAR(y+1,x-1)] && pixel >= data[LINEAR(y-1,x+1)]);
        break;
        default:
        fprintf(stderr, "Unknown direction at %d %d. Got %d\n",y,x,direction);
        exit(1);
    }
    return res;

}

bool isConnected(unsigned char* data, int width, int height, int y, int x, int direction){
    // Returns true iff 'data' at (x,y) has an EDGE neighbour in 'direction' direction
    /* Pre-condition:
    y+-1, x+-1 are in 'data' range
    'direction' is one of these: -90, -45, 0, 45
    */
    bool res;
    switch (direction) {
        case 90:
        res = (data[LINEAR(y,x+1)] == EDGE) | (data[LINEAR(y,x-1)] == EDGE);
        break;
        case 135:
        res = (data[LINEAR(y+1,x+1)] == EDGE) | (data[LINEAR(y-1,x-1)] == EDGE);
        break;
        case 0:
        res = (data[LINEAR(y+1,x)] == EDGE) | (data[LINEAR(y-1,x)] == EDGE);
        break;
        case 45:
        res = (data[LINEAR(y+1,x-1)] == EDGE) | (data[LINEAR(y-1,x+1)] == EDGE);
        break;
        default:
        fprintf(stderr, "Unknown direction at %d %d. Got %d\n",y,x,direction);
        exit(1);
    }
    return res;

}

/* ************* */
/* Main Function */
/* ************* */

void canny(char * src, int width, int height, float uthreshold, float lthreshold){
    /*
    Edge detection using Canny's method. Source image is overwritten with
    a binary one, representing detected edges.
    - src: pointer to matrix of uchar[3], representing an RGB image.
    - width: width of the image (in pixels)
    - height: height of the image (in pixels)
    - uthreshold: upper-threshold for Canny's method
    - lthreshold: lower-threshhold for Canny's method
    */

    if(!C_canny_initialized) initCannyC(width, height);

    unsigned char (*imgRGB)[3] = (unsigned char (*)[3])src;
    src = NULL;

    // Intensity channel only (color average)
    for (int i = 0; i < height*width; i++) {
        unsigned int temp = imgRGB[i][0]+imgRGB[i][1]+imgRGB[i][2];
        img_inten[i] = temp/3;
    }

    /* 1. Smooth image: Gaussian filter */
    float Gauss_kernel[25] = {
        // sigma = 1
        1.0 /273.0,4.0 /273.0,7.0 /273.0,4.0 /273.0,1.0 /273.0,
        4.0 /273.0,16.0 /273.0,26.0 /273.0,16.0 /273.0,4.0 /273.0,
        7.0 /273.0,26.0 /273.0,41.0 /273.0,26.0 /273.0,7.0 /273.0,
        4.0 /273.0,16.0 /273.0,26.0 /273.0,16.0 /273.0,4.0 /273.0,
        1.0 /273.0,4.0 /273.0,7.0 /273.0,4.0 /273.0,1.0 /273
    };
    convoluion2D(img_inten, width, height, Gauss_kernel, 5, img_smooth);

    /* 2. Get gradients: Sobel filter */
    float Gy_kernel[9] = {
        -1,-2,-1,
        0,0,0,
        1,2,1
    };
    float Gx_kernel[9] = {
        -1,0,1,
        -2,0,2,
        -1,0,1
    };
    convoluion2D(img_smooth, width, height, Gy_kernel, 3, Gy);
    convoluion2D(img_smooth, width, height, Gx_kernel, 3, Gx);
    // Obtain gradient module and (quantized) orientation
    // Arrays are overwritten to save memory
    float * Gm = Gx; // Gradient module
    float * Go = Gy; // Gradient orientation
    for (int i = 0; i < height*width; i++) {
        float mod = sqrt(Gy[i]*Gy[i]+Gx[i]*Gx[i]);
        Go[i] = roundDirection(Gx[i],Gy[i]);
        Gm[i] = mod;
    }

    /* 3. Discard below-thresh and double edges (Non-maximum supression) */
    for (int y = 1; y < height-1; y++) {
        for (int x = 1; x < width-1; x++) {
            imgRGB[LINEAR(y,x)][0] = NOT_EDGE;
            imgRGB[LINEAR(y,x)][1] = NOT_EDGE;
            imgRGB[LINEAR(y,x)][2] = NOT_EDGE;
            float module = Gm[LINEAR(y,x)];
            int direction = Go[LINEAR(y,x)];
            if (isLocalmax(Gm, width, height, y, x, direction)) {
                if (module >= uthreshold) {
                    // An edge for sure
                    imgRGB[LINEAR(y,x)][0] = EDGE;
                    imgRGB[LINEAR(y,x)][1] = EDGE;
                    imgRGB[LINEAR(y,x)][2] = EDGE;
                }else if(module > lthreshold){
                    // May be an edge
                    imgRGB[LINEAR(y,x)][0] = CANDIDATE;
                    imgRGB[LINEAR(y,x)][1] = CANDIDATE;
                    imgRGB[LINEAR(y,x)][2] = CANDIDATE;
                }
            }
        }
    }

    /* 4. Expand edges: Hysterysis*/
    /* At this point, only pixels in range [lthres,htresh] remain, and will be
    edges only if connected to pixels that already are. */
    bool changes = true;
    while (changes) {
        changes = false;
        for (int y = 1; y < height-1; y++) {
            for (int x = 1; x < width-1; x++) {
                if (imgRGB[LINEAR(y,x)][0] == EDGE || imgRGB[LINEAR(y,x)][0] == NOT_EDGE) continue;
                int gradient_direction = (int)Go[LINEAR(y,x)];
                int edge_direction = (gradient_direction + 90) % 180;
                if (isConnected((unsigned char *)imgRGB, width, height, y, x, edge_direction)) {
                    imgRGB[LINEAR(y,x)][0] = EDGE;
                    imgRGB[LINEAR(y,x)][1] = EDGE;
                    imgRGB[LINEAR(y,x)][2] = EDGE;
                    changes = true;
                }
            }
        }
    }

    // Clean candidate edges
    for (int y = 1; y < height-1; y++) {
        for (int x = 1; x < width-1; x++) {
            if (imgRGB[LINEAR(y,x)][0] == CANDIDATE){
                imgRGB[LINEAR(y,x)][0] = NOT_EDGE;
                imgRGB[LINEAR(y,x)][1] = NOT_EDGE;
                imgRGB[LINEAR(y,x)][2] = NOT_EDGE;
            }
       }
    }
}
