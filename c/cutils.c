#include "cutils.h"
#include "math.h"

// Checks if value is inside the range [minimum, maximum)
int within(int value, int minimum, int maximum) {
    return value >= minimum && value < maximum;
}

// Returns max
int max(int a, int b) {
    return a > b ? a : b;
}

// Returns min
int min(int a, int b) {
    return a < b ? a : b;
}

// Returns value restricted to range [minimum, maximum]
int clamp(int value, int minimum, int maximum) {
    return value < minimum ? minimum : value > maximum ? maximum : value;
}

// Calculates norm of vector
float norm(float x, float y) {
    return sqrt(x * x + y * y);
}

// Calculates squared distance of two vectors in three dimensions
float squared_distance3(char p[3], char q[3]) {
    return (p[0] - q[0]) * (p[0] - q[0]) +  \
           (p[1] - q[1]) * (p[1] - q[1]) +  \
           (p[2] - q[2]) * (p[2] - q[2]);
}
