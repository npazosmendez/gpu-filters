#ifndef __FILTERS_H__
#define __FILTERS_H__

#include <QtGui>
#include <QLabel>
#include <QObject>
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
using namespace cv;
using namespace std;
extern "C" {
    #include <c/c-filters.h>
}
#include <opencl/opencl-filters.hpp>
#include <atomic>

#include "camera.hpp"
#include "hitcounter.hpp"

namespace Ui {
    class MainWindow;
}

class ImageFilter : public QThread {
    Q_OBJECT
	private:
		bool cl = true;
	protected:
		void run(){
			while(1) sleep(1);
		}
	public:
		bool CL() {return cl;};
		bool C() {return !cl;};
		void setCL() {cl = true;};
		void setC() {cl = false;};

	public slots:
		virtual void process_frame(Mat *){};

	signals:
		void frame_ready(Mat* cameraFrame);
};

class CannyFilter : public ImageFilter {
    Q_OBJECT
	public slots:
		void process_frame(Mat *cameraFrame);
};

class HoughFilter : public ImageFilter {
    Q_OBJECT
	public slots:
		void process_frame(Mat *cameraFrame);
};

#endif // __FILTERS_H__