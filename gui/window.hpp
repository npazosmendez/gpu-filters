#ifndef __WINDOW_H__
#define __WINDOW_H__

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
#include "filters.hpp"

#include <QMenu>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow  {
    Q_OBJECT

	private:
		QLabel * _label;
		QPixmap pixmap;
		QImage image;
		Camera cam;
		QImage MatToQImage(const Mat& mat);
		ImageFilter* currentFilter;
		HitCounter hitcounter;
		inline void overlay_frame_rate(QPixmap* pixmap);
		QMenu* m_menu;
		void setFilter(ImageFilter * filter);

    public:
    	MainWindow() : _label(new QLabel) {
    		currentFilter = new HoughFilter;
    		setCentralWidget(_label);
			QObject::connect(&cam, SIGNAL(new_frame(Mat*)), currentFilter, SLOT(process_frame(Mat*)), Qt::QueuedConnection );
			QObject::connect(currentFilter, SIGNAL(frame_ready(Mat*)), this, SLOT(show_frame(Mat*)), Qt::QueuedConnection );
			currentFilter->start();
			cam.start();
    	};

	public slots:
		void show_frame(Mat *cameraFrame);
		void setFilter(QString);
		void toggleCL(int);
};

#endif // __WINDOW_H__