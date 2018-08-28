#include <iostream>
#include <string>

using namespace std;

#include <QtGui>
#include <QLabel>
#include <QObject>

#include "VideoWindow.hpp"
#include "FancySlider.hpp"
#include "FilterControls.hpp"

int main(int argc, char** argv) {
    
	QApplication app(argc, argv);


	FilterControls controls;
	QDialog controlsWindow;
	QVBoxLayout mainLayout;
	QComboBox comboBox;
	comboBox.addItem("Canny");
	comboBox.addItem("Hough");
	QCheckBox checkBox("OpenCL");
	mainLayout.addWidget(&checkBox);
	mainLayout.addWidget(&comboBox);
	mainLayout.addWidget(&controls);
	controlsWindow.setLayout(&mainLayout);


	
	VideoWindow videoWindow(comboBox.currentText(), checkBox.checkState());

	QObject::connect(&comboBox, SIGNAL(currentIndexChanged(QString)), &videoWindow, SLOT(setFilter(QString)));
	QObject::connect(&checkBox, SIGNAL(stateChanged(int)), &videoWindow, SLOT(toggleCL(int)));


	controlsWindow.show();
	videoWindow.show();

    return app.exec();
}


