#ifndef CFILTER_H
#define CFILTER_H

#include "stdbool.h"

void canny(char * src, int width, int height, float uthreshold, float lthreshold);
void hough(char * src, int width, int height, int a_ammount, int p_ammount, char* counter);
void inpainting(char * ptr, int width, int height, bool * mask);
void opticalflow(char * ptr, char * ptr2, int width, int height);

void inpaint_generate_arbitrary_mask(bool * dst, int width, int height);
void inpaint_init(int width, int height, char * ptr, bool * mask_ptr);
bool inpaint_step(int width, int height, char * ptr, bool * mask_ptr);

#endif
