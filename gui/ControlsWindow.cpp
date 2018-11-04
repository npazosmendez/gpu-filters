#include "ControlsWindow.hpp"
#include "FilterControls.hpp"
#include <QtGui>
#include <QLabel>
#include <QObject>
#include <unistd.h>
#include "debug.h"


ControlsWindow::ControlsWindow(QWidget *parent) : 
	QDialog(parent), _browser("Browse video"), _camera_button("Camera", this){

	debug_print("Initializing controls window\n");
	
	debug_print("Setting layout\n");
	_comboBox.addItem("No Filter");
	_comboBox.addItem("Canny");
	_comboBox.addItem("Hough");
	_comboBox.addItem("Kanade");

	_main_layout.addWidget(&_comboBox);
	_main_layout.addWidget(&_browser);
	_main_layout.addWidget(&_camera_button);
	QObject::connect(&_comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(setFilter(QString)));
	QObject::connect(&_browser, SIGNAL(fileSelected(QString)), this, SLOT(setFile(QString)));
    QObject::connect(&_camera_button, SIGNAL(clicked()), this, SLOT(setCamera()));
	setLayout(&_main_layout);

	debug_print("Initializing processing pipeline\n");
	_video_window = new VideoWindow;
	_camera = new Camera;
	_camera_is_on = true;
    _filters[0] = new NoFilter;
    _filters[1] = new CannyFilter;
    _filters[2] = new HoughFilter;
    _filters[3] = new KanadeFilter;
	_current_filter = _filters[0];
	_controls = _current_filter->controls();
	assemble();

	QObject::connect(this, SIGNAL(yyy()),_video_window, SLOT(xxx()), Qt::QueuedConnection);
	QObject::connect(this, SIGNAL(yyy()),_current_filter, SLOT(xxx()), Qt::QueuedConnection);


}

void ControlsWindow::setCamera(){
	if (_camera_is_on) return;
	debug_print("Setting camera as input stream\n" );
	disassemble();
	_camera->deleteLater();
	_camera = new Camera;
	_camera_is_on = true;
	assemble();

}
void ControlsWindow::setFile(QString file_name){
	debug_print("Video file set to %s\n", file_name );
	disassemble();

	_camera->deleteLater();
	_camera = new VideoFileStreamer(file_name.toStdString());
	_camera_is_on = false;

	assemble();

}
void ControlsWindow::setFilter(QString filterName_){
	const char * filterName = filterName_.toStdString().c_str();;
	debug_print("Filter set to %s\n",filterName);
	disassemble();

    _main_layout.removeWidget(_controls);
	_controls->deleteLater();
	QObject::disconnect(this, SIGNAL(yyy()),_current_filter, SLOT(xxx()));

	if (strcmp(filterName, "No Filter") == 0 ){
		 _current_filter = _filters[0];
		 debug_print("Just set No Filter\n");
	}else{
		debug_print("Not No\n");
	}
	if (strcmp(filterName, "Canny") == 0 ){
		 _current_filter = _filters[1];
		 debug_print("Just set Canny\n");
	}else{
		debug_print("Not Canny\n");
	}
	if (strcmp(filterName, "Hough") == 0 ){
		 _current_filter = _filters[2];
		 debug_print("Just set Hough\n");
	}else{
		debug_print("Not Hough\n");
	}
	if (strcmp(filterName, "Kanade") == 0 ){
		 _current_filter = _filters[3];
		 debug_print("Just set Kanade\n");
	}else{
		debug_print("Not Kanade\n");
	}
	debug_print("Done choosing filter\n");

	_controls = _current_filter->controls();
	_main_layout.addWidget(_controls);

	this->adjustSize();
	QObject::connect(this, SIGNAL(yyy()),_current_filter, SLOT(xxx()), Qt::QueuedConnection);

	assemble();
}
void ControlsWindow::show(){
	debug_print("Showing controls window\n");
	QDialog::show();
	_video_window->show();
	setFilter("No Filter");
}


void ControlsWindow::disassemble(){
	debug_print("Dissassembling processing pipe\n");
	_camera->stop();
	_camera->wait();
	debug_print("Streamer said it is done\n");
	empty_pipe();

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
	debug_print("Assembling processing pipe\n");
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

void ControlsWindow::empty_pipe(){
	emit yyy();
}