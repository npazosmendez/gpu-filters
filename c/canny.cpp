#include "c-filters.hpp"
#include "convolution.hpp"
#include <iostream>
#include <cmath>

/*
NOTE: might be a good idea to let aux arrays be global, as they are being used
constantly. Malloc es very caro.
*/

void canny(char * src, int width, int height, float uthreshold, float lthreshold){
    /*
    Edge detection using Canny's method. Source image is overwritten with
    a binary one, representing detected edges.
    - src: pointer to matrix of char[3], representing an RGB image.
    - width: width of the image (in pixels)
    - height: height of the image (in pixels)
    - uthreshold: upper-threshold for Canny's method
    - lthreshold: lower-threshhold for Canny's method
    */
    float (*img_inten2)[width] = (float (*)[width])malloc(width*height*sizeof(float));
    float * img_inten = (float*)malloc(width*height*sizeof(float));
    float * Gx = (float *)malloc(width*height*sizeof(float));
    float * Gy = (float *)malloc(width*height*sizeof(float));

    // Intensity channel only (color average)
    unsigned char (*imgRGB)[width][3] = (unsigned char (*)[width][3])src;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            unsigned int temp = imgRGB[i][j][0]+imgRGB[i][j][2]+imgRGB[i][j][2];
            img_inten2[i][j] = temp/3;
        }
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
    convoluion2D<float,float>((float*)img_inten2, width, height, Gauss_kernel, 5, img_inten);

    /* 2. Get gradients: Sobel filter */
    float Gy_kernel[9] = {
        1,2,1,
        0,0,0,
        -1,-2,-1
    };
    float Gx_kernel[9] = {
        1,0,-1,
        2,0,-2,
        1,0,-1
    };
    convoluion2D<float, float>(img_inten, width, height, Gy_kernel, 3, Gy);
    convoluion2D<float, float>(img_inten, width, height, Gx_kernel, 3, Gx);
    // Obtain gradient module and (quantized) orientation
    // Arrays are overwritten to save memory
    for (int i = 0; i < height*width; i++) {
        float mod = sqrt(Gy[i]*Gy[i]+Gx[i]*Gx[i]);
        Gx[i] = mod;
        float arctan = atan(Gy[i]/Gx[i]);
        if(arctan <= 3*M_PI/8 && arctan > M_PI/8){
            /* round to 45° */
            arctan = 45;
        }else if(arctan <= M_PI/8 && arctan > -M_PI/8){
            /* round to 0° */
            arctan = 0;
        }else if(arctan <= -M_PI/8 && arctan > -3*M_PI/8){
            /* round to -45 */
            arctan = -45;
        }else{
            /* round to 90 */
            arctan = 90;
        }
        Gy[i] = arctan;
    }
    float (*Gm)[width] = (float (*)[width])Gx; // Gradient module
    float (*Go)[width] = (float (*)[width])Gy; // Gradient orientation
    Gx = NULL; Gy = NULL;

    /* 3. Discard below-thresh and double edges (Non-maximum supression) */
    for (int i = 1; i < height-1; i++) {
        for (int j = 1; j < width-1; j++) {
            imgRGB[i][j][0] = 0;
            imgRGB[i][j][1] = 0;
            imgRGB[i][j][2] = 0;
            // Only looking for a local max in edge direction
            float temp;
            if(Go[i][j] == -45)
            temp = (Gm[i][j] > Gm[i+1][j-1] && Gm[i][j] > Gm[i-1][j+1] ? Gm[i][j] : 0);
            else if(Go[i][j] == 90)
            temp = (Gm[i][j] > Gm[i+1][j] && Gm[i][j] > Gm[i-1][j] ? Gm[i][j] : 0);
            else if(Go[i][j] == 45)
            temp = (Gm[i][j] > Gm[i+1][j+1] && Gm[i][j] > Gm[i-1][j-1] ? Gm[i][j] : 0);
            else if(Go[i][j] == 0)
            temp = (Gm[i][j] > Gm[i][j+1] && Gm[i][j] > Gm[i][j-1] ? Gm[i][j] : 0);
            else{
                // Quantization failed?
                fprintf(stderr, "Failed to get angle at %d %d. Got %f\n",i,j,Go[i][j]);
                exit(1);
            }
            if (temp >= uthreshold) {
                // An edge
                imgRGB[i][j][0] = 255;
                imgRGB[i][j][1] = 255;
                imgRGB[i][j][2] = 255;
            }else if(temp < lthreshold){
                // Not an edge
                Gm[i][j] = 0;
            }
        }
    }

    /* 4. Expand edges: Hysterysis*/
    /* At this point, only pixels in range [lthres,htresh] remain, and will be
    edges only if connected to pixels that already are. */
    bool changes = true;
    while (changes) {
        changes = false;
        for (int i = 1; i < height-1; i++) {
            for (int j = 1; j < width-1; j++) {
                if (imgRGB[i][j][0] == 255 || Gm[i][j] == 0) continue;

                unsigned char temp = 0;
                if(Go[i][j] == -45){
                    if((imgRGB[i+1][j+1][0] == 255 && Go[i+1][j+1] == -45) ||
                    (imgRGB[i-1][j-1][0] == 255 && Go[i-1][j-1] == -45)){
                        temp = 255;
                    }
                }else if(Go[i][j] == 0){
                    if((imgRGB[i+1][j][0] == 255 && Go[i+1][j] == 0) ||
                    (imgRGB[i-1][j][0] == 255 && Go[i-1][j] == 0)){
                        temp = 255;
                    }
                }else if(Go[i][j] == 45){
                    if((imgRGB[i+1][j-1][0] == 255 && Go[i+1][j-1] == 45) ||
                    (imgRGB[i-1][j+1][0] == 255 && Go[i-1][j+1] == 45)){
                        temp = 255;
                    }
                }else if(Go[i][j] == 90){
                    if((imgRGB[i][j-1][0] == 255 && Go[i][j-1] == 90) ||
                    (imgRGB[i][j+1][0] == 255 && Go[i][j+1] == 90)){
                        temp = 255;
                    }
                }else{
                    fprintf(stderr, "Failed to get angle at %d %d. Got %f\n",i,j,Go[i][j]);
                    exit(1);
                }
                if (temp == 255) changes = true;
                imgRGB[i][j][0] = temp;
                imgRGB[i][j][1] = temp;
                imgRGB[i][j][2] = temp;
            }
        }
    }

    free(img_inten);
    free(img_inten2);
    free(Gm);
    free(Go);

}
