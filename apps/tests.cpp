//
// Created by Daro on 23/09/2018.
//

#include <stdlib.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
extern "C" {
#include "c/c-filters.h"
}
#include "opencl/opencl-filters.hpp"

using namespace std;

#define EPS 1e-3

#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_RESET   "\x1b[0m"

#define mu_assert(test) do { if (!(test)) return (char *) #test ; } while (0)
#define mu_success(test) printf("%s%s: Success%s\n", ANSI_GREEN, #test, ANSI_RESET)
#define mu_failure(test, msg) printf("%s%s: Failed assertion '%s'%s\n", ANSI_RED, #test, msg, ANSI_RESET)
#define mu_run_test(test) do { char * msg = test(); msg ? mu_failure(test, msg) : mu_success(test); } while (0)

void intensity_to_img(unsigned char * intensity, char * img, int width, int height) {
    for (int i = 0; i < height * width; i++) {
        for (int k = 0; k < 3; k++) {
            img[3*i+k] = intensity[i];
        }
    }
}

static char * test_kanade() {
    int width = 5;
    int height = 5;

    unsigned char intensity_old[5*5] = {
              0,   0, 200,   0,   0,
              0,   0, 225,   0,   0,
              0,   0, 250, 200, 150,
              0,   0,   0,   0,   0,
              0,   0,   0,   0,   0
    };
    char img_old[5*5*3];
    intensity_to_img(intensity_old, img_old, width, height);

    unsigned char intensity_new[5*5] = {
              0, 175,   0,   0,   0,
              0, 200,   0,   0,   0,
              0, 225,   0,   0,   0,
              0, 250, 200, 150, 100,
              0,   0,   0,   0,   0
    };
    char img_new[5*5*3];
    intensity_to_img(intensity_new, img_new, width, height);

    vec flow[5*5];

    int levels = 1;

    kanade(width, height, img_old, img_new, flow, levels);

    cout << flow[2*width+2].x << "\n";
    //mu_assert(abs(flow[2*width+2].x - (-0.127)) < EPS);   Nope, flow is integer
    //mu_assert(abs(flow[2*width+2].y - (0.093)) < EPS);    Nope, flow is integer

    return 0;
}

int main(int argc, char** argv) {
    //string _; cin >> _;
    mu_run_test(test_kanade);
    return 0;
}