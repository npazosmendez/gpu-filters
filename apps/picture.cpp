#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <string>
extern "C" {
    #include "c/c-filters.h"
}
#include "opencl/opencl-filters.hpp"

using namespace cv;
using namespace std;

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

int main( int argc, char** argv ) {
    // Parse input
    if (argc != 2) {
        cout << "Requires image path as parameter" << endl;
        return -1;
    }

    Mat original;
    original = imread(argv[1], CV_LOAD_IMAGE_COLOR);
    if(!original.data) {
        cout <<  "Could not open or find the image" << std::endl ;
        return -1;
    }

    int width = original.cols;
    int height = original.rows;

    // Apply filters
    int quantity = 2;
    vector<Mat> images(quantity);
    for (int i = 0; i < quantity; i++) {
        images[i] = original.clone();
    }

    overlay(images[0], "Raw");

    bool* mask = (bool*) malloc(width * height * sizeof(bool));
    generate_arbitrary_mask(mask, width, height);
    inpainting((char*) images[1].ptr(), width, height, mask);
    overlay(images[1], "Inpainting");

    // Loop
    namedWindow("Display window", WINDOW_AUTOSIZE);
    int index = 0;

    while (true) {
        imshow("Display window", images[index]);

        char input = (char) cv::waitKey(5);
        if (input >= '0' && input < '0' + quantity) {
            index = input - '0';
        } else if (input == 'q') {
            return 0;
        }
    }


    return 0;
}
