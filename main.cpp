#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
using namespace cv;
using namespace std;

#include "canny/canny.h"

int main() {
    VideoCapture stream(0);   // video device number 0

    if (!stream.isOpened()) {
        cout << "Failed to open camera.";
        exit(1);
    }

    int width = stream.get(CV_CAP_PROP_FRAME_WIDTH);
    int height = stream.get(CV_CAP_PROP_FRAME_HEIGHT);

    Mat cameraFrame;
    while (true) {
        stream.read(cameraFrame); // get camera frame

        char *ptr = (char*)cameraFrame.ptr();

        canny(ptr, width, height);

        imshow("cam1", cameraFrame); // show frame

        if (waitKey(5) >= 0) break; // if key pressed, quit
    }
    return 0;
}
