#include <iostream>
extern "C" {
  #include "c/c-filters.h"
}
#include "opencl/opencl-filters.hpp"
#include <set>
#include <map>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;


int width;
int height;

Mat source_image;

Mat image1;
Mat image2;

map<string, string> parse_args(int argc, const char** argv){
    map<string, string> result;
    for (int i = 0; i < argc; ++i){
        string key = argv[i];
        if (key.size() > 1 and key[0] == '-' and key[1] == '-'){
            if (i+1 < argc){
                result[key] = argv[i+1];
            }else{
                result[key] = "";
            }
        }
    }
    return result;
}

int main(int argc, const char** argv) {

    map<string, string> arguments = parse_args(argc, argv);

    if (arguments.count("--help")){
        cout << "Timing application for the filters. For custom timing try:" << endl;
        cout << "\t--filter <filter_name> (canny/hough/kanade/inpainting)" << endl;
        cout << "\t--image <image_path>" << endl;
        cout << "\t--output <output_image_path>" << endl;
        cout << "\t--help" << endl;
        return 0;
    }

    if (not arguments.count("--filter")){
        cerr << "No filter selected" << endl;
        abort();
    }
    if (not arguments.count("--output")){
        cerr << "No output path specified" << endl;
        abort();
    }

    string filter = arguments["--filter"];
    string output = arguments["--output"];

    if (filter == "canny"){
        image1 = imread(arguments["--image"], CV_LOAD_IMAGE_COLOR);
        CL_canny((char*)image1.ptr(), image1.size().width, image1.size().height, 100, 70);
        imwrite(output, image1);
    }else if(filter == "hough"){
        image1 = imread(arguments["--image"], CV_LOAD_IMAGE_COLOR);
        CL_hough((char*)image1.ptr(), image1.size().width, image1.size().height, 300, 300, NULL);
        imwrite(output, image1);
    }


    return 0;
}