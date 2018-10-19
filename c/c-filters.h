#ifndef CFILTER_H
#define CFILTER_H

#include "stdlib.h"
#include "stdbool.h"

typedef struct t_vec {
    int x, y;
} vec;

typedef struct t_vecf {
    float x, y;
} vecf;

void canny(char * src, int width, int height, float uthreshold, float lthreshold);
void hough(char * src, int width, int height, int a_ammount, int p_ammount, char* counter);
void inpainting(char * ptr, int width, int height, bool * mask, int * debug);
void opticalflow(char * ptr, char * ptr2, int width, int height);

void inpaint_generate_arbitrary_mask(bool * mask, int width, int height);
void inpaint_init(int width, int height, char * img, bool * mask, int * debug);
bool inpaint_step(int width, int height, char * img, bool * mask, int * debug);

void kanade(int width, int height, char * imgA, char * imgB, vec * flow, int levels);

#endif
