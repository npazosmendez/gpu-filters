#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <QtGui>
#include <QLabel>
#include <QObject>
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include "camera.hpp"
#include "hitcounter.hpp"
#include "ImageFilter.hpp"

using namespace cv;
using namespace std;

namespace Ui {
    class VideoWindow;
}
class VideoWindow : public QMainWindow  {
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
		ImageFilter _filters[3];

    public:
    	VideoWindow();
    	~VideoWindow();

	public slots:
		void show_frame(Mat *cameraFrame);
		void setFilter(QString);
		void setFilter(ImageFilter * filter);
};

#endif // __WINDOW_H__