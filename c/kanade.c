//
// Created by Daro on 24/09/2018.
//

#include "c-filters.h"
#include "stdio.h"
#include "stdbool.h"
#include "string.h"
#include "math.h"
#include "time.h"
#include "float.h"
#include "cutils.h"

#define LINEAR(x, y) (y)*width+(x)
#define forn(i,n) for(int i=0; i<(n); i++)

static int LK_WINDOW_RADIUS = 2;

static float KERNEL_SOBEL_Y[3][3] = {
        -1,-2,-1,
        0,0,0,
        1,2,1
};
static float KERNEL_SOBEL_X[3][3] = {
        -1,0,1,
        -2,0,2,
        -1,0,1
};

static bool initialized = false;

static float * gradient_x;
static float * gradient_y;
static float * gradient_t;

static float * intensity_old;
static float * intensity_new;


// ROSETTA SNIPPET
// https://rosettacode.org/wiki/Gaussian_elimination

// Careful, modifies a and b!!

#define mat_elem(a, y, x, n) (a + ((y) * (n) + (x)))

void swap_row(double *a, double *b, int r1, int r2, int n)
{
    double tmp, *p1, *p2;
    int i;

    if (r1 == r2) return;
    for (i = 0; i < n; i++) {
        p1 = mat_elem(a, r1, i, n);
        p2 = mat_elem(a, r2, i, n);
        tmp = *p1, *p1 = *p2, *p2 = tmp;
    }
    tmp = b[r1], b[r1] = b[r2], b[r2] = tmp;
}

void gauss_eliminate(double *a, double *b, double *x, int n)
{
#define A(y, x) (*mat_elem(a, y, x, n))
    int i, j, col, row, max_row,dia;
    double max, tmp;

    for (dia = 0; dia < n; dia++) {
        max_row = dia, max = A(dia, dia);

        for (row = dia + 1; row < n; row++)
            if ((tmp = fabs(A(row, dia))) > max)
                max_row = row, max = tmp;

        swap_row(a, b, dia, max_row, n);

        for (row = dia + 1; row < n; row++) {
            tmp = A(row, dia) / A(dia, dia);
            for (col = dia+1; col < n; col++)
                A(row, col) -= tmp * A(dia, col);
            A(row, dia) = 0;
            b[row] -= tmp * b[dia];
        }
    }
    for (row = n - 1; row >= 0; row--) {
        tmp = b[row];
        for (j = n - 1; j > row; j--)
            tmp -= x[j] * A(row, j);
        x[row] = tmp / A(row, row);
    }
#undef A
}


int interest_x = 400;
int interest_y = 190;


void init(int width, int height) {
    gradient_x = (float *) malloc(width * height * sizeof(float));
    gradient_y = (float *) malloc(width * height * sizeof(float));
    gradient_t = (float *) malloc(width * height * sizeof(float));

    intensity_old = (float *) malloc(width * height * sizeof(float));
    intensity_new = (float *) malloc(width * height * sizeof(float));

    initialized = true;
}


void kanade(int width, int height, char * img_old, char * img_new, vec * flow) {

    if (!initialized) init(width, height);

    // Intensity channels
    for (int i = 0; i < height * width; i++) {
        unsigned char (*img_oldRGB)[3] = (unsigned char (*)[3])img_old;
        intensity_old[i] = (img_oldRGB[i][0] + img_oldRGB[i][1] + img_oldRGB[i][2]) / 3;
    }

    for (int i = 0; i < height * width; i++) {
        unsigned char (*img_newRGB)[3] = (unsigned char (*)[3])img_new;
        intensity_new[i] = (img_newRGB[i][0] + img_newRGB[i][1] + img_newRGB[i][2]) / 3;
    }

    // Gradients
    convoluion2D(intensity_old, width, height, KERNEL_SOBEL_Y, 3, gradient_y);
    convoluion2D(intensity_old, width, height, KERNEL_SOBEL_X, 3, gradient_x);
    forn(i, width * height) gradient_t[i] = intensity_new[i] - intensity_old[i];

    // Build Matrices
    /*
     * A = [ sum IxIx  sum IxIy           b = [ - sum IxIt
     *       sum IyIx  sum IyIy ]               - sum IyIt ]
     */
    forn(y, height) forn(x, width) {

        float IxIx = 0;
        float IxIy = 0;
        float IyIy = 0;
        float IxIt = 0;
        float IyIt = 0;

        int window_diameter = LK_WINDOW_RADIUS * 2 + 1;
        forn(wy, window_diameter) forn(wx, window_diameter) {
            int in_x = clamp(x + (wx - LK_WINDOW_RADIUS), 0, width - 1);
            int in_y = clamp(y + (wy - LK_WINDOW_RADIUS), 0, height - 1);

            IxIx += gradient_x[LINEAR(in_x, in_y)] * gradient_x[LINEAR(in_x, in_y)];
            IxIy += gradient_x[LINEAR(in_x, in_y)] * gradient_y[LINEAR(in_x, in_y)];
            IyIy += gradient_y[LINEAR(in_x, in_y)] * gradient_y[LINEAR(in_x, in_y)];
            IxIt += gradient_x[LINEAR(in_x, in_y)] * gradient_t[LINEAR(in_x, in_y)];
            IyIt += gradient_y[LINEAR(in_x, in_y)] * gradient_t[LINEAR(in_x, in_y)];
        }

        double A[2][2] = {IxIx, IxIy, IxIy, IyIy};
        double b[2] = {-IxIt, -IyIt};
        double u[2]; // TODO: If this was x it would still compile, why?

        gauss_eliminate((double*)A, b, u, 2);

        flow[LINEAR(x, y)] = (vec) {u[0], u[1]};

        /*
        if (y == interest_y && x == interest_x) {
            forn(dy, 3) {
                forn(dx, 3) {
                    printf("%.2f ", intensity_old[LINEAR(interest_x + (dx - 1), interest_y + (dy - 1))]);
                }
                printf("\n");
            }

            flow[LINEAR(interest_x - 10, interest_y - 10)] = (vec) {u[0] * 10, u[1] * 10};
            printf("Disp = <%.2f, %.2f>\n", u[0], u[1]);
            printf("\n");
        }
        */
    }

    // Debug


    //printf("Gradient = <%.2f, %.2f>\n", gradient_x[LINEAR(interest_x, interest_y)], gradient_y[LINEAR(interest_x, interest_y)]);

    /*

        flow[LINEAR(interest_x - 10, interest_y - 10)] = (vec) {10, 10};

        printf("\n");

        for(int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (y % 50 == 0 && x % 50 == 0) {
                    flow[LINEAR(x, y)] = (vec) {10, 10};
                }
            }
        }
        */
}