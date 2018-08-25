#ifndef __CAMERA__H
#define __CAMERA__H

#include <QtGui>
#include <QLabel>
#include <QObject>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;

extern QAtomicInt frameReady; 

class Camera : public QThread {
	
	Q_OBJECT

	private:
		int width, height;
		VideoCapture stream;


    public:
		void run();
    signals:
    	void new_frame(Mat*);
};

#endif