#include <QtGui>
#include <QLabel>
#include <QObject>
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include "ImageFilter.hpp"
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
    
void VideoWindow::show_frame(Mat *frame){
    hitcounter.hit();
    image = MatToQImage(*frame);
    pixmap.convertFromImage(image);
    overlay_frame_rate(&pixmap);
    _label->resize(image.size());
    _label->setPixmap(pixmap);
    _label->show();
    this->show();

}


VideoWindow::VideoWindow() : _label(new QLabel) {
    // _label->setMinimumSize(1, 1);
    setCentralWidget(_label);
};

VideoWindow::~VideoWindow() {
    delete _label;
}
