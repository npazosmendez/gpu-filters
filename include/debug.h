#ifndef DEBUG
#define DEBUG 0
#endif

// This macro below works only with GCC
#define debug_print(fmt, ...) \
        do { if (DEBUG) printf("%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, ##__VA_ARGS__); } while (0)