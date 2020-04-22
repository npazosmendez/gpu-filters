#include "c-filters.h"
#include "math.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "utils/debug.h"

#define M_PI 3.14159265358979323846
#define LINEAR(x,y) (y)*width+(x)

unsigned char (*edgesRGB)[3] = NULL;
float* alfas, *cos_alfas, *sin_alfas, *pp;

#define MAX_DST_GRANULARITY 1000
#define MAX_DGR_GRANULARITY 1000

void initHoughC(int width, int height, int a_ammount, int p_ammount){
    debug_print("Initializing C Hough\n");

    static bool initialized = false;
    if (initialized == false){
        // Initialize buffers for Hough line detection algorithm
        edgesRGB = malloc(MAX_BUFFER_SIZE*sizeof(char)*3);
        cos_alfas = malloc(MAX_DGR_GRANULARITY * sizeof(float));
        sin_alfas =  malloc(MAX_DGR_GRANULARITY * sizeof(float));
        pp = malloc(MAX_DST_GRANULARITY* sizeof(int));
        initialized = true;
    }

    static int _width = -1, _height = -1, _a_ammount = -1, _p_ammount = -1;

    if (width != _width || height != _height ||
        _a_ammount != a_ammount || _p_ammount != p_ammount){
        _width = width;
        _height = height;
        _a_ammount = a_ammount;
        _p_ammount = p_ammount;

        float p_max = sqrt(width*width + height*height);
        float p_min = -p_max;
        float p_step = (p_max-p_min)/p_ammount;
        float a_max = M_PI;
        float a_min = 0;
        float a_step = (a_max-a_min)/a_ammount;

        /* NOTE: second optimization that made a really big difference,
        having cos a sin precomputed
        */
        for (int i = 0; i < a_ammount; ++i){
            float alfa = a_min+a_step*i;
            cos_alfas[i] = cos(alfa);
            sin_alfas[i] = sin(alfa);
        }
        for (int i = 0; i < p_ammount; ++i){
            pp[i] = p_min+p_step*i;
        }

    }
}

void hough(char * src, int width, int height, int a_ammount, int p_ammount, char* counter){
    /*
    Line detection using Hough Transformation. Detected lines are drawn over
    the source image.
    - src: pointer to matrix of char[3], representing an RGB image.
    - width: width of the image (in pixels)
    - height: height of the image (in pixels)
    - a_ammount: line angle granularity, quantity considered in [0, PI]
    - p_ammount: line position granularity, quantity considered along image diagonal
    - counter: pointer to matrix of char[3], to output hough transform of edge pixels
    */
    initHoughC(width, height, a_ammount, p_ammount);
    float p_max = sqrt(width*width + height*height);
    float p_min = -p_max;
    float p_step = (p_max-p_min)/p_ammount;

    unsigned char (*imgRGB)[3] = (unsigned char (*)[3])src;
    unsigned char (*counterRGB)[3] = (unsigned char (*)[3])counter;
    src = 0;
    counter = 0;

    memcpy(edgesRGB, imgRGB, width*height*3 );

    canny((char*)edgesRGB, width, height, 150, 100);
    
    int contador[a_ammount][p_ammount];
    memset(contador, 0, a_ammount*p_ammount*sizeof(int));

    int max = 1;
    // TODO: the canny algorithm is not yet considering bounds,
    // so we ignore them. should be fixed in the future 
    for (int y = 10; y < height-10; ++y){
        for (int x = 10; x < width-10; ++x){
            if (edgesRGB[LINEAR(x,y)][0] > 100){
                for (int ai = 0; ai < a_ammount; ++ai){
                    float p = x*cos_alfas[ai]+y*sin_alfas[ai];
                    int p_index = round((p-p_min)/p_step);
                    contador[p_index][ai]++;
                    if (contador[p_index][ai] > max){
                        max = contador[p_index][ai];
                    }
                }
            }
        }
    }

    /* draw counter in buffer */
    for (int y = 0; y < p_ammount; ++y){
        for (int x = 0; x < a_ammount; ++x){
            int val = contador[y][x];
            counterRGB[(y)*a_ammount+(x)][0] = 0;
            counterRGB[(y)*a_ammount+(x)][1] = val*255/max;
            counterRGB[(y)*a_ammount+(x)][2] = 0;
        }
    }
    
    /* find peaks in counter */
    const int window = a_ammount*0.1;
    int threshold = max*0.5;
    for (int y = 1; y < p_ammount-1; ++y){
        for (int x = 1; x < a_ammount-1; ++x){
            int val = contador[y][x];
            if (val > threshold){
                for(int dy = y-window/2; dy < y+window/2; dy++){
                    for(int dx = x-window/2; dx < x+window/2; dx++){
                        if(dy > 0 && dx > 0 
                            && dy < p_ammount 
                            && dx < a_ammount 
                            && contador[dy][dx] > val){
                            // forgive me, lord
                            goto not_local_max;
                        }
                    }
                }
                /* mark it on counter */
                counterRGB[(y)*a_ammount+(x)][0] = 0;
                counterRGB[(y)*a_ammount+(x)][1] = 0;
                counterRGB[(y)*a_ammount+(x)][2] = 255;
                
                /* overlay line in image */
                int ai = x;
                int pi = y;
                float p = pp[pi];
                float m2 = sin_alfas[ai]/(cos_alfas[ai]+0.00001);
                float m1 = -1.0 / (m2 + 0.00001);
                float b = p / (sin_alfas[ai] + 0.00001);
                for (int xx = 0; xx < width; ++xx){
                    int yy = b+xx*m1;
                    if (yy < height && yy >= 0){
                        imgRGB[LINEAR(xx,yy)][0] = 0;
                        imgRGB[LINEAR(xx,yy)][1] = 0;
                        imgRGB[LINEAR(xx,yy)][2] = 255;
                    }
                }
                for (int yy = 0; yy < height; ++yy){
                    int xx = (yy-b)/m1;
                    if (xx < width && xx >= 0){
                        imgRGB[LINEAR(xx,yy)][0] = 0;
                        imgRGB[LINEAR(xx,yy)][1] = 0;
                        imgRGB[LINEAR(xx,yy)][2] = 255;
                    }
                }
            }
            not_local_max:
            continue;
        }
    }

}

