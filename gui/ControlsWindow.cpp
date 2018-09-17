#include "ControlsWindow.hpp"
#include "FilterControls.hpp"
#include <QtGui>
#include <QLabel>
#include <QObject>
#include <unistd.h>


ControlsWindow::ControlsWindow(QWidget *parent) : 
	QDialog(parent), _browser("Browse video"){

	qDebug() << "Initializing controls window";
	
	qDebug() << "Setting layout";
	_comboBox.addItem("No Filter");
	_comboBox.addItem("Canny");
	_comboBox.addItem("Hough");
	_main_layout.addWidget(&_comboBox);
	_main_layout.addWidget(&_browser);
	QObject::connect(&_comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(setFilter(QString)));
	QObject::connect(&_browser, SIGNAL(fileSelected(QString)), this, SLOT(setFile(QString)));

	setLayout(&_main_layout);

	qDebug() << "Initializing processing pipeline";
	_video_window = new VideoWindow;
	_camera = new VideoStreamer("../children_640.mp4");
    _filters[0] = new NoFilter;
    _filters[1] = new CannyFilter;
    _filters[2] = new HoughFilter;
	_current_filter = _filters[0];
	_controls = _current_filter->controls();
	assemble();
}

void ControlsWindow::setFile(QString file_name){
	qDebug() << "Video file set to " << file_name ;
	disassemble();

	_camera->deleteLater();
	_camera = new VideoStreamer(file_name.toStdString());

	assemble();

}
void ControlsWindow::setFilter(QString filterName){
	qDebug() << "Filter set to " << filterName ;
	disassemble();

    _main_layout.removeWidget(_controls);
	_controls->deleteLater();

	if (filterName == "No Filter") _current_filter = _filters[0];
	if (filterName == "Canny") _current_filter = _filters[1];
	if (filterName == "Hough") _current_filter = _filters[2];

	_controls = _current_filter->controls();
	_main_layout.addWidget(_controls);

	assemble();
}
void ControlsWindow::show(){
	qDebug() << "Showing controls window";
	QDialog::show();
	_video_window->show();
	setFilter("No Filter");
}


void ControlsWindow::disassemble(){
	qDebug() << "Dissassembling processing pipe";
	_camera->stop();
	_camera->wait();

    if (_current_filter != NULL){
		QObject::disconnect(
			_camera, SIGNAL(emit_frame(Mat*)),
			_current_filter, SLOT(give_frame(Mat*))
		);
		QObject::disconnect(
			_current_filter, SIGNAL(filtered_frame(Mat*)),
			_video_window, SLOT(show_frame(Mat*))
		);
    }

}

void ControlsWindow::assemble(){
	qDebug() << "Assembling processing pipe";
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

	_current_filter->start();
	_camera->start();

}
