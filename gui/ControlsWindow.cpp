#include "ControlsWindow.hpp"
#include "FilterControls.hpp"
#include <QtGui>
#include <QLabel>
#include <QObject>
#include <unistd.h>


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

    _filters[0] = new NoFilter;
    _filters[1] = new CannyFilter;
    _filters[2] = new HoughFilter;

    _controls = _filters[0]->controls();
	_main_layout.addWidget(_controls);
}

void ControlsWindow::setFilter(QString filterName){
	ImageFilter* filter;
	if (filterName == "No Filter") filter = _filters[0];
	if (filterName == "Canny") filter = _filters[1];
	if (filterName == "Hough") filter = _filters[2];
    _main_layout.removeWidget(_controls);
    delete _controls;
	_controls = filter->controls();
	_main_layout.addWidget(_controls);
	emit filterChanged(filter);
}
void ControlsWindow::show(){
	setFilter("No Filter");
	QDialog::show();
}
