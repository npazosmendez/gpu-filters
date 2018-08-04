#include "c-filters.h"
#include "math.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define M_PI 3.14159265358979323846
#define LINEAR(y,x) (y)*width+(x)

bool C_hough_initialized = false;
unsigned char (*edgesRGB)[3] = NULL;
float* alfas, *cos_alfas, *sin_alfas, *pp;

void initHoughC(int width, int height, int a_ammount, int p_ammount){
    // Initialize buffers for Hough line detection algorithm

    edgesRGB = malloc(width*height*3);
    float p_max = sqrt(width*width + height*height);
    float p_min = -p_max;
    float p_step = (p_max-p_min)/p_ammount;
    float a_max = M_PI;
    float a_min = 0;
    float a_step = (a_max-a_min)/a_ammount;

    int alto = p_ammount;
    int ancho = a_ammount;

    alfas = malloc(ancho * sizeof(int));
    /* NOTE: second optimization that made a really big difference,
    having cos a sin precomputed
    */
    cos_alfas = malloc(ancho * sizeof(int));
    sin_alfas =  malloc(ancho * sizeof(int));
    for (int i = 0; i < ancho; ++i){
        alfas[i] = a_min+a_step*i;
        cos_alfas[i] = cos(alfas[i]);
        sin_alfas[i] = sin(alfas[i]);
    }    
    pp = malloc(alto* sizeof(int));
    for (int i = 0; i < alto; ++i){
        pp[i] = p_min+p_step*i;
    }
    
    C_hough_initialized = true;
}

void hough(char * src, int width, int height, int a_ammount, int p_ammount, char* counter){
    /*
    Line detection using Hough Transformation. Detected lines are drawn over
    the source image.
    - src: pointer to matrix of char[3], representing an RGB image.
    - width: width of the image (in pixels)
    - height: height of the image (in pixels)
    */
    if(!C_hough_initialized) initHoughC(width, height, a_ammount, p_ammount);

    unsigned char (*imgRGB)[3] = (unsigned char (*)[3])src;
    unsigned char (*counterRGB)[3] = (unsigned char (*)[3])counter;
    src = 0;
    counter = 0;

    memcpy (edgesRGB, imgRGB, width*height*3 );

    canny((char*)edgesRGB, width, height, 70, 30);
    canny((char*)imgRGB, width, height, 70, 30);
    
    int contador[a_ammount][p_ammount];
    for (int y = 0; y < p_ammount; ++y){
        for (int x = 0; x < a_ammount; ++x){
           contador[y][x] = 0;
        }
    }
    int max = 1;
    // the canny algorithm is not yet considering bounds,
    // so we ignore them. should be fixed in the future 
    for (int y = 10; y < height-10; ++y){
        for (int x = 10; x < width-10; ++x){
            if (edgesRGB[LINEAR(y,x)][0] > 100){
                for (int ai = 0; ai < a_ammount; ++ai){
                    float p = x*cos_alfas[ai]+y*sin_alfas[ai];
                    /*
                    int p_index = 0;
                    while(p_index < alto-1 && pp[p_index] <= p) p_index++;
                    contador[p_index][ai]++;
                    if (contador[p_index][ai] > max){
                        max = contador[p_index][ai];
                    }
                    */
                    /*
                    NOTE: first optimizacion that made a big difference,
                    finding p_index with a binary search.
                    */
                    int l_index = 0;
                    int h_index = p_ammount;
                    while(l_index+1 < h_index){
                        int p_index = (l_index+h_index)/2;
                        if (pp[p_index] <= p){
                            l_index = p_index;
                        }else{
                            h_index = p_index;
                        }
                    }
                    contador[l_index][ai]++;
                    if (contador[l_index][ai] > max){
                        max = contador[l_index][ai];
                    }
                }
            }
        }
    }

    
    for (int y = 0; y < p_ammount; ++y){
        for (int x = 0; x < a_ammount; ++x){
            int val = contador[y][x];
            counterRGB[(y)*a_ammount+(x)][0] = 0;
            counterRGB[(y)*a_ammount+(x)][1] = val*255/max;
            counterRGB[(y)*a_ammount+(x)][2] = 0;
        }
    }
    
    int threshold = max/2;
    for (int y = 5; y < p_ammount-5; ++y){
        for (int x = 5; x < a_ammount-5; ++x){
            int val = contador[y][x];
            if (val > threshold){
                if(contador[y-1][x] > val) continue;
                if(contador[y+1][x] > val) continue;
                if(contador[y][x-1] > val) continue;
                if(contador[y][x+1] > val) continue;
                if(contador[y+1][x+1] > val) continue;
                if(contador[y-1][x+1] > val) continue;
                if(contador[y+1][x-1] > val) continue;
                if(contador[y-1][x-1] > val) continue;

                counterRGB[(y)*a_ammount+(x)][0] = 0;
                counterRGB[(y)*a_ammount+(x)][1] = 0;
                counterRGB[(y)*a_ammount+(x)][2] = 255;
                int ai = x;
                int pi = y;
                float alpha = alfas[ai];
                float p = pp[pi];
                float m2 = sin(alpha)/(cos(alpha)+0.00001);
                float m1 = -1.0 / (m2 + 0.00001);
                float b = p / (sin(alpha) + 0.00001);
                for (int xx = 0; xx < width; ++xx)
                {
                    int yy = b+xx*m1;
                    if (yy < height && yy >= 0)
                    {
                    imgRGB[LINEAR(yy,xx)][0] = 255;
                    }
                }
            }
        }
    }

    counterRGB[0*a_ammount][0] = 255;
    counterRGB[10*a_ammount][0] = 255;
    counterRGB[20*a_ammount][0] = 255;
    counterRGB[30*a_ammount][0] = 255;
    counterRGB[40*a_ammount][0] = 255;
    counterRGB[50*a_ammount][0] = 255;
    counterRGB[60*a_ammount][0] = 255;
    counterRGB[70*a_ammount][0] = 255;
    counterRGB[80*a_ammount][0] = 255;
    /*
    
    */

}

