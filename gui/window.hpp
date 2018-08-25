#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <QtGui>
#include <QLabel>
#include <QObject>
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include "camera.hpp"
#include "hitcounter.hpp"
#include "filters.hpp"

using namespace cv;
using namespace std;

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
		ImageFilter* _currentFilter;
		HitCounter hitcounter;
		inline void overlay_frame_rate(QPixmap* pixmap);
		int _cl_status;
		void setFilter(ImageFilter * filter);

    public:
    	MainWindow(QString filter, int cl_status);
    	~MainWindow();

	public slots:
		void show_frame(Mat *cameraFrame);
		void setFilter(QString);
		void toggleCL(int);
};

#endif // __WINDOW_H__