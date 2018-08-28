#include <iostream>
#include <string>

using namespace std;

#include <QtGui>
#include <QLabel>
#include <QObject>

#include "window.hpp"

int main(int argc, char** argv) {
    
	QApplication app(argc, argv);


	QDialog controls;
    controls.setWindowTitle(QString("Filtering Controls"));
    QVBoxLayout *mainLayout = new QVBoxLayout;
	controls.setLayout(mainLayout);
	QComboBox * comboBox = new QComboBox;
	comboBox->addItem("Canny");
	comboBox->addItem("Hough");
	QCheckBox * checkBox =  new QCheckBox("OpenCL");
    mainLayout->addWidget(checkBox);
    mainLayout->addWidget(comboBox);
	
	MainWindow videoWindow(comboBox->currentText(), checkBox->checkState());

	QObject::connect(comboBox, SIGNAL(currentIndexChanged(QString)), &videoWindow, SLOT(setFilter(QString)));
	QObject::connect(checkBox, SIGNAL(stateChanged(int)), &videoWindow, SLOT(toggleCL(int)));


	controls.show();
	videoWindow.show();

    return app.exec();
}


