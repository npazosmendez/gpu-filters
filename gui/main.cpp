#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <string>
#include <iomanip>
#include <ui/interactivefilter.hpp>
#include <chrono>

extern "C" {
    #include <c/c-filters.h>
}
#include <opencl/opencl-filters.hpp>

using namespace cv;
using namespace std;


// Main variables

#include <QtGui>
#include <QLabel>
#include <QObject>

#include "filters.hpp"
#include "window.hpp"



int main(int argc, char** argv) {
    
	QApplication app(argc, argv);

	MainWindow videoWindow;
	videoWindow.show();

	QDialog controls;

	QComboBox * comboBox = new QComboBox;
	comboBox->addItem("Canny");
	comboBox->addItem("Hough");
	QObject::connect(comboBox, SIGNAL(currentIndexChanged(QString)), &videoWindow, SLOT(setFilter(QString)));

	QCheckBox * checkBox =  new QCheckBox("OpenCL");
	QObject::connect(checkBox, SIGNAL(stateChanged(int)), &videoWindow, SLOT(toggleCL(int)));

    controls.setWindowTitle(QString("Basic Layouts"));

    QVBoxLayout *mainLayout = new QVBoxLayout;
	controls.setLayout(mainLayout);
    mainLayout->addWidget(checkBox);
    mainLayout->addWidget(comboBox);

	controls.show();

    return app.exec();
}


