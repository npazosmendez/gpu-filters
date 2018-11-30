#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdlib.h>
#include <iostream>
#include <ProgressBar.hpp>
#include <string>
#include <stdio.h>
extern "C" {
    #include "../c/c-filters.h"
    #include "../c/cutils.h"
}
#include "../opencl/opencl-filters.hpp"

using namespace cv;
using namespace std;

#define LINEAR(y,x) (y)*width+(x)
#define forn(i,n) for(int i=0; i<(n); i++)

void edit_iteration();
void inpaint_iteration();
void paint_mask(Mat img);
void overlay(Mat image, string text);
void on_mouse(int event, int x, int y, int flags, void* userdata);

int width;
int height;

Mat img_original; // original image
Mat img_inpainted; // latest inpainted image (mutated by algorithm)
Mat img_masked; // image with black mask showed in fill mode
Mat img_step; // image shown when stepping through the algorithm
bool* mask_ptr;
int* debug;

bool inpaint_mode;
bool fill_mode;
int cursor_radius;

void (*picked_init)(int, int, char*, bool*, int*);
int (*picked_step)(int, int, char*, bool*, int*);


bool quit = false;

void print_help(){
    cout << "Aplicación para correr el algoritmo de inpainting. Con el mouse dibuje la máscara sobre la imagen. Luego presione SPACEBAR." << endl;
    cout << endl;
    cout << "Úsese con las imágenes en inpainter_app/resources/ de la siguiente manera:" << endl;
    cout << "   inpainter <image_path> [flags]\n";
    cout << "Flags\n";
    cout << "   -cl: Usar la versión en OpenCL del inpainting en vez de la de C\n";
    cout << "   -d <number>: Elige el número de device de OpenCL a usar (default: 0)\n";
    cout << "\n";
}

int main( int argc, char** argv ) {

    // Read Images
    if (argc == 1) {
        cout << "Error. Faltan argumentos." << endl;
        print_help();
        return -1;
    }

    img_original = imread(argv[1], CV_LOAD_IMAGE_COLOR);
    if(!img_original.data) {
        cout <<  "Could not open or find the image" << endl;
        return -1;
    }

    picked_init = inpaint_init;
    picked_step = inpaint_step;

    for (int i = 2; i < argc; i++) {
        string param = argv[i];
        if (param == "-cl") {
            /* implementation selection */
            cout << "Using OpenCL implementation..." << endl;
            picked_init = CL_inpaint_init;
            picked_step = CL_inpaint_step;
        } else if (param == "-d") {
            /* device selection */
            if (i+1 == argc) {
                cerr << "Device name missing after '-d'." << endl;
                exit(1);
            }
            int device_number;
            try {
                device_number = stoi(argv[i+1]);
            } catch (int e) {
                cerr << "Couldn't convert '-d' parameter to number" << endl;
                return -1;
            }
            selectDevice(device_number);
        } else if (param == "--help") {
            print_help();
            return 0;
        }
    }
    cout << "\nINPAINTING APP" << endl;
    cout << "Dibuje la máscara con el mouse y presione SPACEBAR para rellenarla." << endl;
    cout << "Para alternar entre la imagen original y la filtrada presione C." << endl;
    height = img_original.rows;
    width = img_original.cols;

    // Setup
    img_inpainted = img_original.clone();
    img_masked = img_original.clone();

    overlay(img_original, "Raw");
    overlay(img_masked, "Filled");

    fill_mode = true;
    cursor_radius = min(width, height) / 20;

    mask_ptr = (bool*) malloc(width * height * sizeof(bool));
    memset(mask_ptr, 0, height * width * sizeof(bool));

    debug = (int*) malloc(width * height * sizeof(debug_data));
    memset(debug, 0, width * height * sizeof(debug_data));

    // Loop
    namedWindow("picture", WINDOW_AUTOSIZE);
    setMouseCallback("picture", on_mouse, NULL);

    while (!quit) {
        if (inpaint_mode) {
            inpaint_iteration();
        } else {
            edit_iteration();
        }
    }

    free(mask_ptr);
    free(debug);

    return 0;
}

void edit_iteration() {
    if (fill_mode) {
        imshow("picture", img_masked);
    } else {
        imshow("picture", img_original);
    }

    switch ((char) cv::waitKey(5)) {
        case 'c':
            fill_mode = !fill_mode;
            break;
        case ' ':
            if (fill_mode) inpaint_mode = true;
            break;
        case 'q':
            quit = true;
    }
}

bool first_inpaint_iteration = true;
bool last_inpaint_iteration = false;
int is_more;
int mask_size();


void inpaint_iteration() {
    if (first_inpaint_iteration) {
        picked_init(width, height, (char*) img_inpainted.ptr(), mask_ptr, debug);
        first_inpaint_iteration = false;
        cout << "\nRellenando máscara..." << endl;
    }

    img_step = img_inpainted.clone();
    paint_mask(img_step);
    imshow("picture", img_step);

    ProgressBar bar(mask_size());
    while (true) {
        is_more = picked_step(width, height, (char*) img_inpainted.ptr(), mask_ptr, debug);
        bar.update(is_more);
        if (!is_more) {
            last_inpaint_iteration = true;
            bar.finish();
            break;
        }
    }

    if (last_inpaint_iteration) {
        memset(mask_ptr, 0, height * width * sizeof(bool));
        img_masked = img_inpainted.clone();
        overlay(img_masked, "Filled");
        first_inpaint_iteration = true;
        last_inpaint_iteration = false;
        inpaint_mode = false;
    }
}

int mask_size(){
    int mask_size = 0;
    for(int i = 0; i < width*height; i++) if(mask_ptr[i]) mask_size++;
    return mask_size;
}


bool dragging = false;
void draw_square(Mat img, int i, int j, Vec3b color, int radius = 1);
void draw_circle(Mat img, int i, int j, Vec3b color, int radius);
void draw_square(bool * mask, int i, int j, bool val, int radius = 1);
void draw_circle(bool * mask, int i, int j, bool val, int radius);

const Vec3b BLUE = Vec3b(255, 0, 0);
const Vec3b GREEN = Vec3b(0, 255, 0);
const Vec3b RED = Vec3b(0, 0, 255);
const Vec3b BLACK = Vec3b(0, 0, 0);

void on_mouse(int event, int x, int y, int flags, void* userdata){
    if (fill_mode) {
        if (event == EVENT_LBUTTONDOWN) {
            dragging = true;
        } else if (event == EVENT_LBUTTONUP) {
            dragging = false;
        }

        if (dragging == true) {
            draw_circle(img_masked, y, x,  BLACK, cursor_radius);
            draw_circle(mask_ptr, y, x,  true, cursor_radius);
        }
    }
}

void paint_mask(Mat img) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (mask_ptr[LINEAR(i, j)]) {
                img.at<Vec3b>(Point(j, i)) = BLACK;
            }
        }
    }
}


void draw_circle(Mat img, int i, int j, Vec3b color, int radius) {
    for (int di = -radius; di <= radius; di++) {
        for (int dj = -radius; dj <= radius; dj++) {
            int local_i = clamp(i + di, 0, height-1);
            int local_j = clamp(j + dj, 0, width-1);
            if ((pow(local_i-i, 2) + pow(local_j-j, 2)) <= pow(radius,2)){
                img.at<Vec3b>(Point(local_j, local_i)) = color;
            }
        }
    }
}
void draw_circle(bool * mask, int i, int j, bool val, int radius) {
    for (int di = -radius; di <= radius; di++) {
        for (int dj = -radius; dj <= radius; dj++) {
            int local_i = clamp(i + di, 0, height-1);
            int local_j = clamp(j + dj, 0, width-1);
            if ((pow(local_i-i, 2) + pow(local_j-j, 2)) <= pow(radius,2)){
                mask[LINEAR(local_i, local_j)] = val;
            }
        }
    }
}

void overlay(Mat image, string text) {
    cv::putText(image, 
                text,
                Point(5,30), // Coordinates
                FONT_HERSHEY_TRIPLEX, // Font
                1.0, // Scale. 2.0 = 2x bigger
                Scalar(255,255,255), // Color
                1, // Thickness
                CV_AA); // Anti-alias
}
