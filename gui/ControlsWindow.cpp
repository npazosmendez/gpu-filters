#include "ControlsWindow.hpp"
#include "FilterControls.hpp"
#include <QtGui>
#include <QLabel>
#include <QObject>
#include <unistd.h>


ControlsWindow::ControlsWindow(QWidget *parent) : QDialog(parent){

	_video_window = new VideoWindow;
	_camera = new Camera;
	_controls = NULL;
	_current_filter = NULL;
    _filters[0] = new NoFilter;
    _filters[1] = new CannyFilter;
    _filters[2] = new HoughFilter;
	
	_comboBox.addItem("No Filter");
	_comboBox.addItem("Canny");
	_comboBox.addItem("Hough");
	_main_layout.addWidget(&_comboBox);
	QObject::connect(&_comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(setFilter(QString)));


    _controls = _filters[0]->controls();
	_main_layout.addWidget(_controls);


	setLayout(&_main_layout);
}

void ControlsWindow::setFilter(QString filterName){
	_camera->exit();
	_camera->wait();
	if (filterName == "No Filter") _current_filter = _filters[0];
	if (filterName == "Canny") _current_filter = _filters[1];
	if (filterName == "Hough") _current_filter = _filters[2];

    if (_controls != NULL){
    	_main_layout.removeWidget(_controls);
    	delete _controls;
    }
	_controls = _current_filter->controls();
	_main_layout.addWidget(_controls);

	QObject::connect(
		_camera, SIGNAL(emit_frame(Mat*)),
		_current_filter, SLOT(give_frame(Mat*)),
		Qt::QueuedConnection
	);
	QObject::connect(
		_current_filter, SIGNAL(filtered_frame(Mat*)),
		_video_window, SLOT(show_frame(Mat*)),
		Qt::DirectConnection
	);
}
void ControlsWindow::show(){
	QDialog::show();
	_video_window->show();
	setFilter("No Filter");
	_current_filter->start();
	_camera->start();
}
