#include "ControlsWindow.hpp"
#include "FilterControls.hpp"
#include <QtGui>
#include <QLabel>
#include <QObject>


ControlsWindow::ControlsWindow(QWidget *parent) : QDialog(parent), _checkBox("OpenCL"){

	_controls = NULL;
	_comboBox.addItem("No Filter");
	_comboBox.addItem("Canny");
	_comboBox.addItem("Hough");
	_main_layout.addWidget(&_checkBox);
	_main_layout.addWidget(&_comboBox);
	setLayout(&_main_layout);
	QObject::connect(&_comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(setFilter(QString)));
	//QObject::connect(&checkBox, SIGNAL(stateChanged(int)), &videoWindow, SLOT(toggleCL(int)));

}

void ControlsWindow::setFilter(QString filterName){
	ImageFilter* filter;
	if (filterName == "Canny") filter = new CannyFilter;
	if (filterName == "Hough") filter = new HoughFilter;
	if (filterName == "No Filter") filter = new NoFilter;
	_main_layout.removeWidget(_controls);
	_controls = filter->controls();
	_main_layout.addWidget(_controls);
	emit filterChanged(filter);
}
void ControlsWindow::show(){
	setFilter("No Filter");
	QDialog::show();
}
