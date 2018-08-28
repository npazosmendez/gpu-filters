#ifndef __FILTERS_H__
#define __FILTERS_H__

#include <QtGui>
#include <QLabel>
#include <QObject>
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

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
	public:
		HoughFilter() : a_ammount(150), p_ammount(150), counter((char*)malloc(150*150*3)) {};
		~HoughFilter() { delete counter; };

	public slots:
		void process_frame(Mat *cameraFrame);
	private:
		int a_ammount, p_ammount;
		char* counter;
};

#endif // __FILTERS_H__