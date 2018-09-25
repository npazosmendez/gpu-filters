#ifndef __VIDEOSTREAMER_H_
#define __VIDEOSTREAMER_H_

#include <QtGui>
#include <QLabel>
#include <QObject>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <QAtomicInt>

using namespace cv;

extern QAtomicInt frame_processed;

class VideoStreamer : public QThread {
	
	Q_OBJECT

	private:
		inline long milliseconds_since_epoch();

    public:
		virtual void run() {};
		virtual void stop() {};

    signals:
    	virtual void emit_frame(Mat*);
};

class VideoFileStreamer : public VideoStreamer {

	Q_OBJECT

	private:
		int width, height;
		VideoCapture stream;
		int index = 0;
		Mat _buffer[4];
		bool _frame_ready = true;
		bool _stop;
		float _fps;
		string _filename;

		inline long milliseconds_since_epoch();

    public:
		VideoFileStreamer(string filename);
		void run();
		void stop();

    signals:
    	void emit_frame(Mat*);
};


class Camera : public VideoStreamer {

	Q_OBJECT

	private:
		int width, height;
		VideoCapture stream;
		int index = 0;
		Mat _buffer[4];
		bool _frame_ready = true;
		bool _stop;

    public:
		Camera();
		void run();
		void stop();

    signals:
    	void emit_frame(Mat*);
};

#endif