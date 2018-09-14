#include <QtGui>
#include <QLabel>
#include <QObject>
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include "ImageFilter.hpp"
#include "camera.hpp"
#include "VideoWindow.hpp"
using namespace cv;
using namespace std;

QImage VideoWindow::MatToQImage(const Mat& mat){
    // 8-bits unsigned, NO. OF CHANNELS=1
    if(mat.type()==CV_8UC1){
        // Set the color table (used to translate colour indexes to qRgb values)
        QVector<QRgb> colorTable;
        for (int i=0; i<256; i++)
            colorTable.push_back(qRgb(i,i,i));
        // Copy input Mat
        const uchar *qImageBuffer = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage img(qImageBuffer, mat.cols, mat.rows, mat.step, QImage::Format_Indexed8);
        img.setColorTable(colorTable);
        return img;
    }else if(mat.type()==CV_8UC3){
    // 8-bits unsigned, NO. OF CHANNELS=3
        // Copy input Mat
        const uchar *qImageBuffer = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage img(qImageBuffer, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return img.rgbSwapped();
    }else{
        qDebug() << "ERROR: Mat could not be converted to QImage.";
        return QImage();
    }
}

inline void VideoWindow::overlay_frame_rate(QPixmap* pixmap){
    QPainter painter( pixmap );
    painter.setPen(QColor(255,255,0));
    painter.setFont( QFont("Arial", 20) );
    double fps = hitcounter.hitsPerSecond();
    painter.drawText( QPoint(0 + 5, pixmap->height() - 5), QString::number(fps) );
}
    
void VideoWindow::show_frame(Mat *cameraFrame){
    hitcounter.hit();
    //_currentFilter.process_frame(cameraFrame);
    image = MatToQImage(*cameraFrame);
    pixmap.convertFromImage(image);
    overlay_frame_rate(&pixmap);
    _label->setPixmap(pixmap);
    _label->show();
    frameReady = 0;
}

void VideoWindow::setFilter(ImageFilter * filter){
    assert(filter != NULL);
    if (_currentFilter != NULL){
        QObject::disconnect(&cam, SIGNAL(new_frame(Mat*)), _currentFilter, SLOT(process_frame(Mat*)) );
        QObject::disconnect(_currentFilter, SIGNAL(frame_ready(Mat*)), this, SLOT(show_frame(Mat*)) );
        _currentFilter->quit();
    }
    _currentFilter = filter;
    _currentFilter->start();

    // It's important to keep the order of these two lines below
    QObject::connect(_currentFilter, SIGNAL(frame_ready(Mat*)), this, SLOT(show_frame(Mat*)), Qt::QueuedConnection );
    QObject::connect(&cam, SIGNAL(new_frame(Mat*)), _currentFilter, SLOT(process_frame(Mat*)), Qt::QueuedConnection );   
}

VideoWindow::VideoWindow() : _label(new QLabel), _currentFilter(NULL) {
    setCentralWidget(_label);
    setFilter(&_noFilter);
    cam.start();
};

VideoWindow::~VideoWindow() {
    delete _label;
}
