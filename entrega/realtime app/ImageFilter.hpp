#ifndef __FILTERS_H__
#define __FILTERS_H__

#include <QtGui>
#include <QLabel>
#include <QObject>
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include "FilterControls.hpp"
extern "C" {
    #include <c/c-filters.h>
}
using namespace cv;
using namespace std;

class ImageFilter : public QThread {
    Q_OBJECT
	private:
		bool cl = false;
		virtual void process_frame_CL(Mat *){};
		virtual void process_frame_C(Mat *){};
		Mat _frame;
	public:
		virtual FilterControls * controls() {return NULL;};
		void start();
		void stop();

	public slots:
		void xxx();
		void give_frame(Mat* frame);
		void toggle_CL(int);

	signals:
		void filtered_frame(Mat* frame);
};


class KanadeFilter : public ImageFilter {
    Q_OBJECT
	public:
		FilterControls * controls();

	private:
		Mat _last_frame;
		vec _flow[2000*2000];
		void process_frame_CL(Mat *);
		void process_frame_C(Mat *);
		void overlay_flow(Mat* frame);
};

class CannyFilter : public ImageFilter {
    Q_OBJECT
	public:
		FilterControls * controls();

	public slots:
		void setHigherThreshold(int value);
		void setLowerThreshold(int value);

	private:
		void process_frame_CL(Mat *);
		void process_frame_C(Mat *);
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
		void setCannyHigherThreshold(int value);
		void setCannyLowerThreshold(int value);

	private:
		void process_frame_CL(Mat *);
		void process_frame_C(Mat *);
		int a_ammount, p_ammount;
		char* counter;
		int _canny_high, _canny_low;
};


class NoFilter : public ImageFilter {
    Q_OBJECT
	public:
		void process_frame_CL(Mat *);
		void process_frame_C(Mat *);
		FilterControls * controls();
};

#endif // __FILTERS_H__