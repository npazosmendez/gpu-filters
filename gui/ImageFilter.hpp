#ifndef __FILTERS_H__
#define __FILTERS_H__

#include <QtGui>
#include <QLabel>
#include <QObject>
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include "FilterControls.hpp"

using namespace cv;
using namespace std;

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
		virtual FilterControls * controls() {return NULL;};

	public slots:
		virtual void process_frame(Mat *){};

	signals:
		void frame_ready(Mat* cameraFrame);
};

class CannyFilter : public ImageFilter {
    Q_OBJECT
	public:
		CannyFilter();
		~CannyFilter();
		FilterControls * controls();
	public slots:
		void process_frame(Mat *cameraFrame);
		void setHigherThreshold(int value);
		void setLowerThreshold(int value);
	private:
		CannyControls* _controls;
		int _higherThreshold = 100;
		int _lowerThreshold = 50;
};

class HoughFilter : public ImageFilter {
    Q_OBJECT
	public:
		FilterControls * controls();
		HoughFilter();
		~HoughFilter();

	public slots:
		void process_frame(Mat *cameraFrame);
	private:
		HoughControls* _controls;
		int a_ammount, p_ammount;
		char* counter;
};
class NoFilter : public ImageFilter {
    Q_OBJECT
	public:
		FilterControls * controls();

	public slots:
		void process_frame(Mat *cameraFrame);
	private:
		FilterControls *_controls = new NoControls(NULL);
};

#endif // __FILTERS_H__