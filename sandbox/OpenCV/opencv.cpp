#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
using namespace cv;
using namespace std;

int main() {
    VideoCapture stream1(0);   //0 is the id of video device.0 if you have only one camera.

    if (!stream1.isOpened()) { //check if video device has been initialised
        cout << "cannot open camera";
        exit(1);
    }

    Mat cameraFrame;
    while (true) {
        stream1.read(cameraFrame);

        // distitnas formas de referenciar un pixel:
        /*
        puntero a fila (sin argumentos es fila 0):
            cameraFrame.ptr()[0]
        at<Vec3b>:
            cameraFrame.at<Vec3b>(0,0)[0]
        puntero a arreglo de 3 char:
            char (*ptr)[3] = (char (*)[3])cameraFrame.ptr();
            !! cuidado que estén contiguos
        */
        /*
        if(cameraFrame.isContinuous()){
            cout << (int)cameraFrame.ptr()[0] << " " << (int)cameraFrame.at<Vec3b>(0,0)[0] << " "<< (int)ptr[0][0]<< endl;
            cout << (int)cameraFrame.ptr()[1] << " " << (int)cameraFrame.at<Vec3b>(0,0)[1] << " "<< (int)ptr[0][1]<< endl;
            cout << (int)cameraFrame.ptr()[2] << " " << (int)cameraFrame.at<Vec3b>(0,0)[2] << " "<< (int)ptr[0][2]<< endl;
            cout << (int)cameraFrame.ptr()[3] << " " << (int)cameraFrame.at<Vec3b>(0,1)[0] << " "<< (int)ptr[1][0]<< endl;
            cout << (int)cameraFrame.ptr()[4] << " " << (int)cameraFrame.at<Vec3b>(0,1)[1] << " "<< (int)ptr[1][1]<< endl;
            cout << (int)cameraFrame.ptr()[5] << " " << (int)cameraFrame.at<Vec3b>(0,1)[2] << " "<< (int)ptr[1][2]<< endl;

        }else{
            exit(1);
        }
        */

        char (*ptr)[3] = (char (*)[3])cameraFrame.ptr();
        for (size_t i = 0; i < cameraFrame.total(); i++) {
            // ptr[i][0] += 100;
            // ptr[i][1] = 0;
            // ptr[i][2] = 0;
        }

        imshow("cam1", cameraFrame);

        if (waitKey(5) >= 0)
        break;
    }
    return 0;
}


// g++ opencv.cpp -o opencv `pkg-config --cflags --libs opencv`
