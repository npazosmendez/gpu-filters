#ifndef __CAMERA__H
#define __CAMERA__H

#include <QtGui>
#include <QLabel>
#include <QObject>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;

class Camera : public QThread {
	
	Q_OBJECT

	private:
		int width, height;
		VideoCapture stream;
		int index = 0;
		Mat _buffer[4];
		bool _frame_ready = true;

    public:
		Camera();
		void run();

    public slots:
    	void request_frame();

    signals:
    	void emit_frame(Mat*);
};

#endif