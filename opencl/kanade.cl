
#define forn(i,n) for(int i=0; i<(n); i++)
#define LINEAR(x, y) (y)*width+(x)

__constant int2 ZERO = (int2) (0, 0);

__constant int LK_ITERATIONS = 8;
__constant int CORNER_WINDOW_RADIUS = 1; // recommended
__constant int LK_WINDOW_RADIUS = 2;
__constant double THRESHOLD_CORNER = 0.03;

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


// AUXILIARS
// +++++++++
/*
static int is_corner(double tensor[2][2]) {
    double determinant = tensor[0][0] * tensor[1][1] - tensor[0][1] * tensor[1][0];
    double trace = tensor[0][0] + tensor[1][1];
    double magic_coefficient = 0.05;

    double cornerism = determinant - (magic_coefficient * trace * trace);

    return cornerism > THRESHOLD_CORNER;
}
 */

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

void print_row(__global float * matrix, int2 size, int2 pos) {
    int width = size.x;
    int height = size.y;
    printf("%0.2f %0.2f %0.2f %0.2f %0.2f\n", matrix[LINEAR(pos.x-2, pos.y)], matrix[LINEAR(pos.x-1, pos.y)], matrix[LINEAR(pos.x, pos.y)], matrix[LINEAR(pos.x+1, pos.y)], matrix[LINEAR(pos.x+2, pos.y)]);
}

void print_mat(__global float * matrix, int2 size, int2 pos) {
    print_row(matrix, size, (int2) (pos.x, pos.y-2));
    print_row(matrix, size, (int2) (pos.x, pos.y-1));
    print_row(matrix, size, (int2) (pos.x, pos.y));
    print_row(matrix, size, (int2) (pos.x, pos.y+1));
    print_row(matrix, size, (int2) (pos.x, pos.y+2));
}

// KERNELS
// +++++++

__kernel void calculate_tensor_and_mineigen(
        __global float * gradient_x,
        __global float * gradient_y,
        __global float * out_tensor, // Why I can't define I it as '__global float * out_tensor[2][2]?'
        __global float * out_min_eigen,
        __global float * intensity_old) {

    int2 size = (int2)(get_global_size(0), get_global_size(1));
    int2 pos = (int2)(get_global_id(0), get_global_id(1));
    int index = (pos.y * size.x) + pos.x;

    // Tensor
    float IxIx = 0;
    float IxIy = 0;
    float IyIy = 0;

    int window_radius = CORNER_WINDOW_RADIUS;
    int window_diameter = window_radius * 2 + 1;

    forn(wy, window_diameter) forn(wx, window_diameter) {
        int2 in_pos = clamp(pos + ((int2) (wx, wy) - window_radius), ZERO, size - 1);
        int in_index = (in_pos.y * size.x) + in_pos.x;

        IxIx += gradient_x[in_index] * gradient_x[in_index];
        IxIy += gradient_x[in_index] * gradient_y[in_index];
        IyIy += gradient_y[in_index] * gradient_y[in_index];
    }

    float tensor[2][2];
    tensor[0][0] = IxIx;
    tensor[0][1] = IxIy;
    tensor[1][0] = IxIy;
    tensor[1][1] = IyIy;

    // Mineigen
    double determinant = tensor[0][0] * tensor[1][1] - tensor[0][1] * tensor[1][0];
    double trace = tensor[0][0] + tensor[1][1];

    float eigen_1 = (trace / 2.0 + sqrt((float)(pow((float)trace, (float)2.0)/4.0 - determinant)));
    float eigen_2 = (trace / 2.0 - sqrt((float)(pow((float)trace, (float)2.0)/4.0 - determinant)));

    float min_eigen = min(eigen_1, eigen_2);

    if (min_eigen > 1000000 && pos.x < 3 && pos.y < 3) {
        printf("Min Eigen = %f\n", min_eigen);
        printf("A = %f %f %f %f\n", tensor[0][0], tensor[0][1], tensor[1][0], tensor[1][1]);
        printf("Gradient x \n");
        print_mat(gradient_x, size, pos);
        printf("Gradient y \n");
        print_mat(gradient_y, size, pos);
        printf("Image \n");
        print_mat(intensity_old, size, pos);
        printf("\n");
    }
    /*
    if (pos.x == 50 && pos.y == 50) {
        printf("A = %f %f %f %f\n", tensor[0][0], tensor[0][1], tensor[1][0], tensor[1][1]);
        printf("Min Eigen = %f\n", min_eigen);
        printf("\n");
    }
    */

    // Output
    out_tensor[4 * index + 0] = ((float *) tensor)[0];
    out_tensor[4 * index + 1] = ((float *) tensor)[1];
    out_tensor[4 * index + 2] = ((float *) tensor)[2];
    out_tensor[4 * index + 3] = ((float *) tensor)[3];
    out_min_eigen[index] = min_eigen;
}

#define ANSI_RED     "\x1b[31m"
#define ANSI_RESET   "\x1b[0m"
#define ASSERT(condition) do { if (!(condition)) printf("%s Failed assertion '%s'%s\n", ANSI_RED, #condition, ANSI_RESET); } while (0)

__kernel void calculate_flow(
        int width,
        int height,
        __global float * gradient_x,
        __global float * gradient_y,
        __global float * intensity_old,
        __global float * intensity_new,
        __global float2 * flow,
        int previous_width,
        int previous_height,
        __global float2 * previous_flow,
        __global float * tensor, // TODO: Remove this, I actually don't need it
        __global float * min_eigen,
        float max_min_eigen) {

    int2 size = (int2)(get_global_size(0), get_global_size(1));
    int2 pos = (int2)(get_global_id(0), get_global_id(1));
    int index = (pos.y * size.x) + pos.x;

    // Corner

    bool is_corner = min_eigen[index] / max_min_eigen > THRESHOLD_CORNER;

    if (is_corner) {
        // A

        float IxIx = 0;
        float IxIy = 0;
        float IyIy = 0;

        int window_diameter = LK_WINDOW_RADIUS * 2 + 1;
        forn(wy, window_diameter) forn(wx, window_diameter) {
            int2 in_pos = clamp(pos + ((int2) (wx, wy) - LK_WINDOW_RADIUS), ZERO, size - 1);

            IxIx += gradient_x[LINEAR(in_pos.x, in_pos.y)] * gradient_x[LINEAR(in_pos.x, in_pos.y)];
            IxIy += gradient_x[LINEAR(in_pos.x, in_pos.y)] * gradient_y[LINEAR(in_pos.x, in_pos.y)];
            IyIy += gradient_y[LINEAR(in_pos.x, in_pos.y)] * gradient_y[LINEAR(in_pos.x, in_pos.y)];
        }

        float A[2][2];
        A[0][0] = IxIx;
        A[0][1] = IxIy;
        A[1][0] = IxIy;
        A[1][1] = IyIy;

        // b

        float2 previous_guess = previous_flow ? previous_flow[(pos.y/2) * previous_width + (pos.x/2)] * 2: (float2) ( 0, 0 );
        float2 iter_guess = (float2) ( 0, 0 );

        for (int i = 0; i < LK_ITERATIONS; i++) {
            float IxIt = 0;
            float IyIt = 0;

            int window_diameter = LK_WINDOW_RADIUS * 2 + 1;
            forn(wy, window_diameter) forn(wx, window_diameter) {
                int2 in_pos = clamp(pos + ((int2) (wx, wy) - LK_WINDOW_RADIUS), ZERO, size - 1);
                int2 in_guessed_pos = in_pos + convert_int2(previous_guess + iter_guess);

                int source_index = LINEAR(in_pos.x, in_pos.y);
                int target_index = LINEAR(in_guessed_pos.x, in_guessed_pos.y);

                float gradient_t = 0;
                if (target_index >= 0 && target_index < width * height) {
                    gradient_t = intensity_new[target_index] - intensity_old[source_index];
                }

                IxIt += gradient_x[LINEAR(in_pos.x, in_pos.y)] * gradient_t;
                IyIt += gradient_y[LINEAR(in_pos.x, in_pos.y)] * gradient_t;
            }

            double b[2] = {-IxIt, -IyIt};
            double d[2];

            gauss_eliminate((double*)A, b, d, 2);
            iter_guess.x += d[0];
            iter_guess.y += d[1];
        }

        flow[LINEAR(pos.x, pos.y)] = previous_guess + iter_guess;

    } else {

        flow[LINEAR(pos.x, pos.y)] = (float2) (0, 0);

    }
}

void convolution(
        __global float * src,
        __global float * dst,
        __constant float * kern) {

    int2 size = (int2)(get_global_size(0), get_global_size(1));
    int2 pos = (int2)(get_global_id(0), get_global_id(1));

    if (((pos.y - KSIZE/2 ) < 0) || ((pos.y + KSIZE/2) > (size.y-1)) ||
        ((pos.x - KSIZE/2 ) < 0) || ((pos.x + KSIZE/2) > (size.x-1))){
        dst[pos.y * size.x + pos.x] = 0;
        return;
    }

    float temp = 0;
    for (int yk = 0; yk < KSIZE; yk++) {
        for (int xk = 0; xk < KSIZE; xk++) {
            float intensity = src[(pos.y + yk - KSIZE/2) * size.x + (pos.x + xk - KSIZE/2)];
            float factor = kern[yk * KSIZE + xk];
            temp += intensity * factor;
        }
    }

    dst[pos.y * size.x + pos.x] = temp;
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

    dst[pos.y * size.x + pos.x] = src[2*pos.y * src_width + 2*pos.x];
    dst[pos.y * size.x + pos.x] = src[2*pos.y * src_width + 2*pos.x];
}

__kernel void calculate_intensity(
        __global char * src,
        __global float * dst) {

    int i = (int)(get_global_id(0));

    char3 pixel = vload3(i, src);

    dst[i] = (((unsigned char)pixel.x) + ((unsigned char)pixel.y) + ((unsigned char)pixel.z)) / 3.0f; // TODO: omgg char can be signeddd
}