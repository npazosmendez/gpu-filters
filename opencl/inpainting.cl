
#define LINEAR(x,y) (x)+(y)*get_global_size(0)

__kernel void add (__global const int* v1, __global int* v2) {
    const int id = get_global_id(0);
    v2[id] = 2 * v1[id];
}

__kernel void target_diffs(void) {
    if (get_global_id(0) == 0 && get_global_id(1) == 0) printf("In kernel!\n");
    int i = 0;
    i += 2;
}
