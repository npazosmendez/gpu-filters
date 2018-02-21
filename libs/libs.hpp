#ifndef LIBS_H
#define LIBS_H
#include <cassert>

void CL_convoluion2D(float* src, int width, int height, float * kernel, int ksize, float* dst);

template <typename T1, typename T2>
void convoluion2D(T1* src, int width, int height, float * kernel, int ksize, T2 * dst){
    /*
    Computes 2d convolution between the image stored in 'src' and 'kernel'.
    Result is stored in 'dst', that should be a buffer of as many elements
    as pixels are in the image. Types T1,T2 must be numbers.
    -src: pointer to matrix of elements, representing a single channel image.
    -width: width of the matrix (in bytes/pixels)
    -height: height of the matrix (in bytes/pixels)
    -kernel: pointer to odd square matrix of floats, representing the kernel
    -ksize: width and height of the kernel (must be an odd number)
    -dst: pointer to matrix of floats, where to store result
    */
    assert(ksize % 2 == 1);

    T1 (*src_img)[width] = (T1 (*)[width])src; // no sabía que esto compilaba jojo
    T2 (*dst_img)[width] = (T2 (*)[width])dst;
    float (*kern_img)[ksize] = (float (*)[ksize])kernel;

    /* image loop*/
    for (int i = 0; i < height; i++) {
        if ((i - ksize/2 ) < 0 || (i + ksize/2) > (height-1)) continue; // TODO: handle boundaries
        for (int j = 0; j < width; j++) {
            if ((j - ksize/2 ) < 0 || (j + ksize/2) > (width-1)) continue; // TODO: handle boundaries
            /* kernel loop */
            T2 temp = 0;
            for (int ik = 0; ik < ksize; ik++) {
                for (int jk = 0; jk < ksize; jk++) {
                    temp += src_img[i+ik-ksize/2][j+jk-ksize/2]*kern_img[ik][jk];
                }
            }
            dst_img[i][j] = temp;
        }
    }
}

#endif
