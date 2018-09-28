#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <QtGui>
#include <QLabel>
#include <QObject>
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include "hitcounter.hpp"
#include "ImageFilter.hpp"
#include <QMainWindow>

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
		HitCounter hitcounter;

		QImage MatToQImage(const Mat& mat);
		inline void overlay_frame_rate(QPixmap* pixmap);

    public:
    	VideoWindow();
    	~VideoWindow();

	public slots:
		void show_frame(Mat *frame);
		void xxx();
};

#endif // __WINDOW_H__
