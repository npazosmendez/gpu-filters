
#define forn(i,n) for(int i=0; i<(n); i++)
#define LINEAR(x, y) (y)*width+(x)

__constant int2 ZERO = (int2) (0, 0);

__constant int LK_ITERATIONS = 30; // TODO: This many iterations are needed for tests, but recomended is actually TATWTOOTM'5-10'
__constant int CORNER_WINDOW_RADIUS = 1; // recommended
__constant int LK_WINDOW_RADIUS = 1;
__constant double THRESHOLD_CORNER = 0.05;

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

#define ANSI_RED     "\x1b[31m"
#define ANSI_RESET   "\x1b[0m"
#define IVAR(var) printf("%s = %d\n", #var, (var))
#define FVAR(var) printf("%s = %f\n", #var, (var))
#define I2VAR(var) printf("%s = (%d, %d)\n", #var, (var).x, (var).y)
#define F2VAR(var) printf("%s = (%f, %f)\n", #var, (var).x, (var).y)
#define ASSERT(condition) do { if (!(condition)) printf("%s Failed assertion '%s'%s\n", ANSI_RED, #condition, ANSI_RESET); } while (0)
#define IN_BOUNDS(index, size) ASSERT((index) >= 0 && (index) < (size).x * (size).y)


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

void print_row(__global float * matrix, int2 size, int2 pos);
void print_mat(__global float * matrix, int2 size, int2 pos);

void print_row(__global float * matrix, int2 size, int2 pos) {
    int width = size.x;
    IN_BOUNDS(LINEAR(pos.x-2, pos.y), size);
    IN_BOUNDS(LINEAR(pos.x-1, pos.y), size);
    IN_BOUNDS(LINEAR(pos.x, pos.y), size);
    IN_BOUNDS(LINEAR(pos.x+1, pos.y), size);
    IN_BOUNDS(LINEAR(pos.x+2, pos.y), size);
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

        IN_BOUNDS(in_index, size);
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

    // Output
    IN_BOUNDS(4 * index + 0, size * 4);
    IN_BOUNDS(4 * index + 3, size * 4);
    out_tensor[4 * index + 0] = tensor[0][0];
    out_tensor[4 * index + 1] = tensor[0][1];
    out_tensor[4 * index + 2] = tensor[1][0];
    out_tensor[4 * index + 3] = tensor[1][1];
    IN_BOUNDS(index, size);
    out_min_eigen[index] = min_eigen;
}

float get_gradient_t(int2 in_pos, float2 previous_guess, float2 iter_guess, int2 size, __global float * intensity_new, __global float * intensity_old);

float get_gradient_t(int2 in_pos, float2 previous_guess, float2 iter_guess, int2 size, __global float * intensity_new, __global float * intensity_old) {

    float2 in_guessed_pos = convert_float2(in_pos) + previous_guess + iter_guess;
    int2 base = convert_int2(in_guessed_pos);

    float gradient_t = 0;

    if (base.x >= 0 && base.y >= 0 && base.x < size.x - 1 && base.y < size.y - 1) {
        float alpha_x = 1 - (in_guessed_pos.x - base.x);
        float alpha_y = 1 - (in_guessed_pos.y - base.y);

        float interpolation = 0;

        IN_BOUNDS((base.y) * size.x + (base.x), size);
        IN_BOUNDS((base.y) * size.x + (base.x + 1), size);
        IN_BOUNDS((base.y + 1) * size.x + (base.x), size);
        IN_BOUNDS((base.y + 1) * size.x + (base.x + 1), size);
        interpolation += (alpha_x) * (alpha_y) * intensity_new[(base.y) * size.x + (base.x)];
        interpolation += (1 - alpha_x) * (alpha_y) * intensity_new[(base.y) * size.x + (base.x + 1)];
        interpolation += (alpha_x) * (1 - alpha_y) * intensity_new[(base.y + 1) * size.x + (base.x)];
        interpolation += (1 - alpha_x) * (1 - alpha_y) * intensity_new[(base.y + 1) * size.x + (base.x + 1)];

        IN_BOUNDS(in_pos.y * size.x + in_pos.x, size);
        gradient_t = interpolation - intensity_old[in_pos.y * size.x + in_pos.x];

    } else {
        // TODO: Lost feature
    }

    return gradient_t;
}

__kernel void test_get_gradient_t(
        int2 in_pos,
        float2 previous_guess,
        float2 iter_guess,
        int2 size,
        __global float * intensity_new,
        __global float * intensity_old,
        __global float * output) {

    output[0] = get_gradient_t(in_pos, previous_guess, iter_guess, size, intensity_new, intensity_old);
}

float2 solve_system(double a, double b, double c, double d, double e, double f) {
    double determinant = a*d - b*c;
    if(determinant != 0) {
        return (float2) ( (e*d - b*f)/determinant, (a*f - e*c)/determinant );
    } else {
        return (float2) ( 0, 0 );
    }
}

void debug_calculate_flow(
        int width,
        int height,
        __global float * gradient_x,
        __global float * gradient_y,
        __global float * intensity_old,
        __global float * intensity_new,
        __global float2 * flow,
        int previous_width,
        int previous_height,
        __global float2 * previous_flow, // TODO: Why is it not NULL when L = 0?
        __global float * tensor, // TODO: Remove this, I actually don't need it
        __global float * min_eigen,
        float max_min_eigen,
        int level) {

    int2 size = (int2)(get_global_size(0), get_global_size(1));
    int2 pos = (int2)(get_global_id(0), get_global_id(1));
    int index = (pos.y * size.x) + pos.x;

    printf("ABNORMAL\n");

    bool has_previous_level = previous_width != -1;


    // Spatial Gradient Matrix
    float IxIx = 0;
    float IxIy = 0;
    float IyIy = 0;

    int window_diameter = LK_WINDOW_RADIUS * 2 + 1;
    forn(wy, window_diameter) forn(wx, window_diameter) {
            int2 in_pos = clamp(pos + ((int2) (wx, wy) - LK_WINDOW_RADIUS), ZERO, size - 1);

            IN_BOUNDS(LINEAR(in_pos.x, in_pos.y), size);
            IxIx += gradient_x[LINEAR(in_pos.x, in_pos.y)] * gradient_x[LINEAR(in_pos.x, in_pos.y)];
            IxIy += gradient_x[LINEAR(in_pos.x, in_pos.y)] * gradient_y[LINEAR(in_pos.x, in_pos.y)];
            IyIy += gradient_y[LINEAR(in_pos.x, in_pos.y)] * gradient_y[LINEAR(in_pos.x, in_pos.y)];
        }

    // Iterative LK
    if (has_previous_level) IN_BOUNDS((pos.y/2) * previous_width + (pos.x/2), size / 2);

    float2 previous_guess = has_previous_level ? previous_flow[(pos.y/2) * previous_width + (pos.x/2)] * 2 : (float2) ( 0, 0 );
    float2 iter_guess = (float2) ( 0, 0 );

    for (int i = 0; i < LK_ITERATIONS; i++) {
        F2VAR(iter_guess);

        float IxIt = 0;
        float IyIt = 0;

        int window_diameter = LK_WINDOW_RADIUS * 2 + 1;
        forn(wy, window_diameter) forn(wx, window_diameter) {

            int2 in_pos = clamp(pos + ((int2) (wx, wy) - LK_WINDOW_RADIUS), ZERO, size - 1);

            float gradient_t = get_gradient_t(in_pos, previous_guess, iter_guess, size, intensity_new, intensity_old);

            IN_BOUNDS(LINEAR(in_pos.x, in_pos.y), size);
            IxIt += gradient_x[LINEAR(in_pos.x, in_pos.y)] * gradient_t;
            IyIt += gradient_y[LINEAR(in_pos.x, in_pos.y)] * gradient_t;
        }

        //DEBUG
        float diff_old = 0;
        forn(wy, window_diameter) forn(wx, window_diameter) {
            int2 in_pos = clamp(pos + ((int2)(wx, wy) - LK_WINDOW_RADIUS), ZERO, size - 1);
            float gradient_t = get_gradient_t(in_pos, previous_guess, iter_guess, size, intensity_new, intensity_old);
            diff_old += (gradient_t * gradient_t);
        }
        printf("Old Error = %f\n", diff_old);
        // DEBUG

        iter_guess += solve_system(IxIx, IxIy, IxIy, IyIy,  -IxIt, -IyIt);
        // TODO: So... I'm solving the system correctly... But solving it can actually INCREASE the error...
        // Option A: Numeric issue (but I don't think so)
        // In all likelihood I'm feeding the system with the wrong numbers
        //  So... Either the gradients are baddly computed...
        //  Or I'm calculating them wrong

        // DEBUG
        float diff_new = 0;
        forn(wy, window_diameter) forn(wx, window_diameter) {
            int2 in_pos = clamp(pos + ((int2)(wx, wy) - LK_WINDOW_RADIUS), ZERO, size - 1);
            float gradient_t = get_gradient_t(in_pos, previous_guess, iter_guess, size, intensity_new, intensity_old);
            diff_new += (gradient_t * gradient_t);
        }
        printf("New Error = %f\n", diff_new);
        // DEBUG

    }

    printf("\n");
}




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
        __global float2 * previous_flow, // TODO: Why is it not NULL when L = 0?
        __global float * tensor, // TODO: Remove this, I actually don't need it
        __global float * min_eigen,
        float max_min_eigen,
        int level) {

    int2 size = (int2)(get_global_size(0), get_global_size(1));
    int2 pos = (int2)(get_global_id(0), get_global_id(1));
    int index = (pos.y * size.x) + pos.x;

    bool has_previous_level = previous_width != -1;

    // Corner
    IN_BOUNDS(index, size);
    bool is_corner = min_eigen[index] / max_min_eigen > THRESHOLD_CORNER;

    //if (is_corner || level != 0) {
    if (true) {

        // Spatial Gradient Matrix
        float IxIx = 0;
        float IxIy = 0;
        float IyIy = 0;

        if (pos.x == 2 && pos.y == 2) {
            printf("Intensity Old\n"); print_mat(intensity_old, size, pos);
            printf("Intensity New\n"); print_mat(intensity_new, size, pos);
            printf("Gradient X\n"); print_mat(gradient_x, size, pos);
            printf("Gradient Y\n"); print_mat(gradient_y, size, pos);
        }

        //if (pos.x == 2 && pos.y == 2)
        //if (pos.x == 2 && pos.y == 2)
        //if (pos.x == 2 && pos.y == 2)

        int window_diameter = LK_WINDOW_RADIUS * 2 + 1;
        forn(wy, window_diameter) forn(wx, window_diameter) {
            int2 in_pos = clamp(pos + ((int2) (wx, wy) - LK_WINDOW_RADIUS), ZERO, size - 1);

            IN_BOUNDS(LINEAR(in_pos.x, in_pos.y), size);
            IxIx += gradient_x[LINEAR(in_pos.x, in_pos.y)] * gradient_x[LINEAR(in_pos.x, in_pos.y)];
            IxIy += gradient_x[LINEAR(in_pos.x, in_pos.y)] * gradient_y[LINEAR(in_pos.x, in_pos.y)];
            IyIy += gradient_y[LINEAR(in_pos.x, in_pos.y)] * gradient_y[LINEAR(in_pos.x, in_pos.y)];

            if (pos.x == 2 && pos.y == 2) I2VAR(in_pos);
            if (pos.x == 2 && pos.y == 2) FVAR(IxIx);
            if (pos.x == 2 && pos.y == 2) FVAR(IxIy);
            if (pos.x == 2 && pos.y == 2) FVAR(IyIy);
        }


        // Iterative LK
        if (has_previous_level) IN_BOUNDS((pos.y/2) * previous_width + (pos.x/2), size / 2);

        float2 previous_guess = has_previous_level ? previous_flow[(pos.y/2) * previous_width + (pos.x/2)] * 2 : (float2) ( 0, 0 );
        float2 iter_guess = (float2) ( 0, 0 );

        for (int i = 0; i < LK_ITERATIONS; i++) {
            float IxIt = 0;
            float IyIt = 0;

            int window_diameter = LK_WINDOW_RADIUS * 2 + 1;
            forn(wy, window_diameter) forn(wx, window_diameter) {

                int2 in_pos = clamp(pos + ((int2) (wx, wy) - LK_WINDOW_RADIUS), ZERO, size - 1);

                float gradient_t = get_gradient_t(in_pos, previous_guess, iter_guess, size, intensity_new, intensity_old);

                IN_BOUNDS(LINEAR(in_pos.x, in_pos.y), size);
                IxIt += gradient_x[LINEAR(in_pos.x, in_pos.y)] * gradient_t;
                IyIt += gradient_y[LINEAR(in_pos.x, in_pos.y)] * gradient_t;
            }

            iter_guess += solve_system(IxIx, IxIy, IxIy, IyIy,  -IxIt, -IyIt);
        }

        // DEBUG
        //if (fabs(iter_guess.y) > 40) debug_calculate_flow(width, height, gradient_x, gradient_y, intensity_old, intensity_new, flow, previous_width, previous_height, previous_flow, tensor, min_eigen, max_min_eigen, level);
        // DEBUG

        IN_BOUNDS(LINEAR(pos.x, pos.y), size);
        flow[LINEAR(pos.x, pos.y)] = previous_guess + iter_guess;

    } else {

        IN_BOUNDS(LINEAR(pos.x, pos.y), size);
        flow[LINEAR(pos.x, pos.y)] = (float2) (0, 0);

    }
}

void convolution(
        __global float * src,
        __global float * dst,
        __constant float * kern);

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
            IN_BOUNDS((pos.y + yk - KSIZE/2) * size.x + (pos.x + xk - KSIZE/2), size);
            IN_BOUNDS(yk * KSIZE + xk, (int2) (KSIZE, KSIZE));
            float intensity = src[(pos.y + yk - KSIZE/2) * size.x + (pos.x + xk - KSIZE/2)];
            float factor = kern[yk * KSIZE + xk];
            temp += intensity * factor;
        }
    }

    IN_BOUNDS(pos.y * size.x + pos.x, size);
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

    IN_BOUNDS(pos.y * size.x + pos.x, size);
    IN_BOUNDS(2*pos.y * src_width + 2*pos.x, (int2)(src_width, src_height));
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