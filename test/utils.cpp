#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <cmath>
#include <iostream>
using namespace cv;
using namespace std;

#include "utils.hpp"

float MSE(Mat& image1, Mat& image2){
	Mat diff = image1 - image2;
	int width = image1.size().width;
	int height = image1.size().height;
	float mse = 0;
	for(int i = 0; i < height; i++){
		for(int j = 0; j < width; j++){
			Vec3b pix1 = image1.at<Vec3b>(i,j);
			Vec3b pix2 = image2.at<Vec3b>(i,j);
			float r, g, b;
			r = pow((float)pix1[0] - (float)pix2[0], 2.0);
			g = pow((float)pix1[1] - (float)pix2[1], 2.0);
			b = pow((float)pix1[2] - (float)pix2[2], 2.0);
			mse += r+g+b;
		}
	}
	float size = image1.size().width * image1.size().height * 3;
	return mse / size;
}

void print_mse(float mse){
	if (mse > 0.1){
        cout << ANSI_RED;
    }else{
        cout << ANSI_GREEN;
    }
    cout << "MSE: " << mse << ANSI_RESET << endl;
}

void compare(Mat& image1, Mat& image2){
	if (image1.size().height != image2.size().height
		or image1.size().width != image2.size().width){
		cerr << ANSI_RED << "Sizes do not match" << ANSI_RESET << endl;
		abort();
	}
    float mse = MSE(image1, image2);
    print_mse(mse);
}