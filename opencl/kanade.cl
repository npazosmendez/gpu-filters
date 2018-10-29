
#define forn(i,n) for(int i=0; i<(n); i++)
#define LINEAR(x, y) (y)*width+(x)

typedef struct t_vecf {
    float x, y;
} vecf;

__constant int LK_ITERATIONS = 8;
__constant int LK_WINDOW_RADIUS = 2;
__constant double THRESHOLD_CORNER = 1e7;


// AUXILIARS
// +++++++++
static int is_corner(double tensor[2][2]) {
    double determinant = tensor[0][0] * tensor[1][1] - tensor[0][1] * tensor[1][0];
    double trace = tensor[0][0] + tensor[1][1];
    double magic_coefficient = 0.05;

    double cornerism = determinant - (magic_coefficient * trace * trace);

    return cornerism > THRESHOLD_CORNER;
}

// ROSETTA SNIPPET
// https://rosettacode.org/wiki/Gaussian_elimination

// Careful, modifies a and b!!

#define mat_elem(a, y, x, n) (a + ((y) * (n) + (x)))

static void swap_row(double *a, double *b, int r1, int r2, int n)
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

static void gauss_eliminate(double *a, double *b, double *x, int n)
{
#define A(y, x) (*mat_elem(a, y, x, n))
    int j, col, row, max_row,dia; // i
    double max, tmp;

    for (dia = 0; dia < n; dia++) {
        max_row = dia, max = A(dia, dia);

        for (row = dia + 1; row < n; row++)
            if ((tmp = fabs((float)A(row, dia))) > max) // Could be double if we extend with 'cl_khr_fp64'
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


// KERNELS
// +++++++

__kernel void calculate_flow(
        int width,
        int height,
        __global float * gradient_x,
        __global float * gradient_y,
        __global float * intensity_old,
        __global float * intensity_new,
        __global vecf * flow,
        int previous_width,
        int previous_height,
        __global vecf * previous_flow) {

    //int2 size = (int2)(get_global_size(0), get_global_size(1));
    int2 pos = (int2)(get_global_id(0), get_global_id(1));

    int x = pos.x;
    int y = pos.y;

    vecf previous_guess = previous_flow ? previous_flow[(y/2) * previous_width + (x/2)] : (vecf) {0};
    previous_guess.x *= 2;
    previous_guess.y *= 2;

    vecf iter_guess = {};

    for (int i = 0; i < LK_ITERATIONS; i++) {

        float IxIx = 0;
        float IxIy = 0;
        float IyIy = 0;
        float IxIt = 0;
        float IyIt = 0;

        int window_diameter = LK_WINDOW_RADIUS * 2 + 1;
        forn(wy, window_diameter) forn(wx, window_diameter) {
            int in_x = clamp(x + (wx - LK_WINDOW_RADIUS), 0, width - 1);
            int in_y = clamp(y + (wy - LK_WINDOW_RADIUS), 0, height - 1);

            int source_index = LINEAR(in_x, in_y);
            int target_index = LINEAR((int)(in_x + previous_guess.x + iter_guess.x), (int)(in_y + previous_guess.y + iter_guess.y));
            float gradient_t = 0;
            if (target_index >= 0 && target_index < width * height) {
                gradient_t = intensity_new[target_index] - intensity_old[source_index];
            } // TODO: Uhh... Do something?

            // TODO: This can be factored out of the loop
            IxIx += gradient_x[LINEAR(in_x, in_y)] * gradient_x[LINEAR(in_x, in_y)];
            IxIy += gradient_x[LINEAR(in_x, in_y)] * gradient_y[LINEAR(in_x, in_y)];
            IyIy += gradient_y[LINEAR(in_x, in_y)] * gradient_y[LINEAR(in_x, in_y)];
            IxIt += gradient_x[LINEAR(in_x, in_y)] * gradient_t;
            IyIt += gradient_y[LINEAR(in_x, in_y)] * gradient_t;
        }

        double A[2][2];
        A[0][0] = IxIx;
        A[0][1] = IxIy;
        A[1][0] = IxIy;
        A[1][1] = IyIy;
        double b[2] = {-IxIt, -IyIt};
        double d[2];

        if (is_corner(A)) {
            gauss_eliminate((double*)A, b, d, 2);
            iter_guess = (vecf) { iter_guess.x + (float)d[0], iter_guess.y + (float)d[1] };
        } else {
            iter_guess = (vecf) {0, 0};
        }

    }

    flow[LINEAR(x, y)] = (vecf) { previous_guess.x + iter_guess.x, previous_guess.y + iter_guess.y };

}

__constant int KSIZE = 3;

__constant float KERNEL_SOBEL_Y[] = {
        -1,-2,-1,
        0,0,0,
        1,2,1
};
__constant float KERNEL_SOBEL_X[] = {
        -1,0,1,
        -2,0,2,
        -1,0,1
};

__constant float KERNEL_GAUSSIAN_BLUR[] = {
        1/16.0, 1/8.0 , 1/16.0,
        1/8.0 , 1/4.0 , 1/8.0 ,
        1/16.0, 1/8.0 , 1/16.0
};

void convolution(
        __global float * src,
        __global float * dst,
        __constant float * kern) {

    int2 size = (int2)(get_global_size(0), get_global_size(1));
    int2 pos = (int2)(get_global_id(0), get_global_id(1));

    int x = pos.x;
    int y = pos.y;
    int width = size.x;
    int height = size.y;
    int ksize = KSIZE;

    if (((y - ksize/2 ) < 0) || ((y + ksize/2) > (height-1)) ||
        ((x - ksize/2 ) < 0) || ((x + ksize/2) > (width-1))){
        dst[LINEAR(x, y)] = 0;
        return; // TODO: handle boundaries
    }

    /* kernel loop */
    float temp = 0;
    for (int yk = 0; yk < ksize; yk++) {
        for (int xk = 0; xk < ksize; xk++) {
            temp += src[LINEAR(x+xk-ksize/2, y+yk-ksize/2)]*(kern[yk*ksize+xk]);
        }
    }

    dst[LINEAR(x, y)] = temp;
}

__kernel void convolution_x(
        __global float * src,
        __global float * dst) {
    convolution(src, dst, KERNEL_SOBEL_X);
}

__kernel void convolution_y(
        __global float * src,
        __global float * dst) {
    convolution(src, dst, KERNEL_SOBEL_Y);
}

__kernel void convolution_blur(
        __global float * src,
        __global float * dst) {
    convolution(src, dst, KERNEL_GAUSSIAN_BLUR);
}

__kernel void subsample(
        int src_width,
        int src_height,
        __global float * src,
        __global float * dst) {

    int2 size = (int2)(get_global_size(0), get_global_size(1));
    int2 pos = (int2)(get_global_id(0), get_global_id(1));

    int x = pos.x;
    int y = pos.y;
    int width = size.x;
    int height = size.y;

    dst[y * width + x] = src[2*y * src_width + 2*x];
    dst[y * width + x] = src[2*y * src_width + 2*x];
}

__kernel void calculate_intensity(
        __global char * src,
        __global float * dst) {

    int i = (int)(get_global_id(0));

    unsigned char (*tri_src)[3] = (unsigned char (*)[3]) src;
    dst[i] = (tri_src[i][0] + tri_src[i][1] + tri_src[i][2]) / 3;

}