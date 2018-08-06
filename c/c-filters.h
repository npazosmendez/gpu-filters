#ifndef CFILTER_H
#define CFILTER_H

#include "stdbool.h"

void canny(char * src, int width, int height, float uthreshold, float lthreshold);
void hough(char * ptr, int width, int height);
void inpainting(char * ptr, int width, int height, bool * mask);

void generate_arbitrary_mask(bool * dst, int width, int height);

void init(int width, int height, char * ptr, bool * mask_ptr);

bool step(int width, int height, char * ptr, bool * mask_ptr);

#endif
