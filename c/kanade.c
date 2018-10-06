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

static float KERNEL_SOBEL_Y[] = {
        -1,-2,-1,
        0,0,0,
        1,2,1
};
static float KERNEL_SOBEL_X[] = {
        -1,0,1,
        -2,0,2,
        -1,0,1
};

static bool initialized = false;

static int * pyramidal_widths;
static int * pyramidal_heights;

static float ** pyramidal_gradients_x;
static float ** pyramidal_gradients_y;
static float ** pyramidal_gradients_t;

static float ** pyramidal_intensities_old;
static float ** pyramidal_intensities_new;

/**
 * Algorithm Overview
 *
 * Para cada pixel...
 *   1. Obtenemos la intensidad (promedio de colores)
 *   2. Calculamos el gradiente en X, en Y (sobel) y en T (resta de imágenes)
 *   3. Armamos el sistema de LK con los gradientes
 *   4. Chequeamos si el tensor (matriz A del sistema) es 'interesante' (Usé el Detector de Esquinas de Harris, hay mejores)
 *    a. Si lo es, resolvemos el sistema que nos da el vector de desplazamiento (Me choree Gauss-Jordan, hay mejores)
 *    b. Si no lo es, dejamos el flow en cero
 *
 *  Fuentes:
 *   Base Optical Flow - Wikipedia (https://en.wikipedia.org/wiki/Lucas%E2%80%93Kanade_method)
 *   Harris Corner Detector - O'Reilly's 'Learning OpenCV' and Wikipedia (https://en.wikipedia.org/wiki/Harris_Corner_Detector)
 *   Pyramidal Implementation - Intel's 'Pyramidal Implementation of the Lucas Kanade Feature Tracker'
 *
 */


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
    int j, col, row, max_row,dia; // i
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


int interest_x = 100;
int interest_y = 100;


void init(int in_width, int in_height) {
    // Pyramid buffers
    pyramidal_widths = (int *) malloc(1 * sizeof(pyramidal_widths));
    pyramidal_heights = (int *) malloc(1 * sizeof(pyramidal_heights));
    pyramidal_gradients_x = (float **) malloc(1 * sizeof(pyramidal_gradients_x));
    pyramidal_gradients_y = (float **) malloc(1 * sizeof(pyramidal_gradients_y));
    pyramidal_gradients_t = (float **) malloc(1 * sizeof(pyramidal_gradients_t));
    pyramidal_intensities_old = (float **) malloc(1 * sizeof(pyramidal_intensities_old));
    pyramidal_intensities_new = (float **) malloc(1 * sizeof(pyramidal_intensities_new));

    // Image buffers
    forn(pi, 1) {
        pyramidal_widths[pi] = in_width;
        pyramidal_heights[pi] = in_height;
        pyramidal_gradients_x[pi] = (float *) malloc(pyramidal_widths[pi] * pyramidal_heights[pi] * sizeof(float));
        pyramidal_gradients_y[pi] = (float *) malloc(pyramidal_widths[pi] * pyramidal_heights[pi] * sizeof(float));
        pyramidal_gradients_t[pi] = (float *) malloc(pyramidal_widths[pi] * pyramidal_heights[pi] * sizeof(float));
        pyramidal_intensities_old[pi] = (float *) malloc(pyramidal_widths[pi] * pyramidal_heights[pi] * sizeof(float));
        pyramidal_intensities_new[pi] = (float *) malloc(pyramidal_widths[pi] * pyramidal_heights[pi] * sizeof(float));
    }

    initialized = true;
}

void finish() {
    // TODO
}

#define THRESHOLD_CORNER 100000
int is_corner(double tensor[2][2]) {
    double determinant = tensor[0][0] * tensor[1][1] - tensor[0][1] * tensor[1][0];
    double trace = tensor[0][0] + tensor[1][1];
    double magic_coefficient = 0.05;

    double cornerism = determinant - (magic_coefficient * trace * trace);

    return cornerism > THRESHOLD_CORNER;
}

void calculate_flow(vec * flow, int pi) {
    // Matrices
    /*
     * A = [ sum IxIx  sum IxIy           b = [ - sum IxIt
     *       sum IyIx  sum IyIy ]               - sum IyIt ]
     */

    int height = pyramidal_heights[pi];
    int width = pyramidal_widths[pi];
    float * gradient_x = pyramidal_gradients_x[pi];
    float * gradient_y = pyramidal_gradients_y[pi];
    float * gradient_t = pyramidal_gradients_t[pi];

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
        double u[2];

        if (is_corner(A)) {
            gauss_eliminate((double*)A, b, u, 2);
            flow[LINEAR(x, y)] = (vec) {u[0], u[1]};
        } else {
            flow[LINEAR(x, y)] = (vec) {0, 0};
        }
    }
}


void kanade(int in_width, int in_height, char * img_old, char * img_new, vec * flow) {

    if (!initialized) init(in_width, in_height);


    forn(pi, 1) {
        // Sub-image buffers
        int width = pyramidal_widths[pi];
        int height = pyramidal_heights[pi];
        float * gradient_x = pyramidal_gradients_x[pi];
        float * gradient_y = pyramidal_gradients_y[pi];
        float * gradient_t = pyramidal_gradients_t[pi];
        float * intensity_old = pyramidal_intensities_old[pi];
        float * intensity_new = pyramidal_intensities_new[pi];

        // Intensity
        forn(i, height * width) {
            unsigned char (*img_oldRGB)[3] = (unsigned char (*)[3])img_old;
            intensity_old[i] = (img_oldRGB[i][0] + img_oldRGB[i][1] + img_oldRGB[i][2]) / 3;
        }

        forn(i, height * width) {
            unsigned char (*img_newRGB)[3] = (unsigned char (*)[3])img_new;
            intensity_new[i] = (img_newRGB[i][0] + img_newRGB[i][1] + img_newRGB[i][2]) / 3;
        }

        // Gradients
        convoluion2D(intensity_old, width, height, KERNEL_SOBEL_Y, 3, gradient_y);
        convoluion2D(intensity_old, width, height, KERNEL_SOBEL_X, 3, gradient_x);
        forn(i, width * height) gradient_t[i] = intensity_new[i] - intensity_old[i];

        // LK algorithm (+ corner detection)
        calculate_flow(flow, pi);
    }



    /*
        printf("Gradient = <%.2f, %.2f>\n", gradient_x[LINEAR(interest_x, interest_y)], gradient_y[LINEAR(interest_x, interest_y)]);


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