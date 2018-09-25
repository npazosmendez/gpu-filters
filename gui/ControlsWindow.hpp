#ifndef CONTROLSWINDOW_H
#define CONTROLSWINDOW_H

#include <QtGui>
#include <QLabel>
#include <QObject>
#include "FilterControls.hpp"
#include "ImageFilter.hpp"
#include "VideoWindow.hpp"
#include "VideoStreamer.hpp"
#include "FileBrowser.hpp"

class ControlsWindow : public QDialog {
	Q_OBJECT
public:
	ControlsWindow(QWidget *parent = 0);
	void show();

private:
	VideoWindow * _video_window;
	VideoStreamer * _camera;
	ImageFilter * _current_filter;
	ImageFilter * _filters[3];
	FilterControls* _controls;

	QPushButton _camera_button;
	FileBrowser _browser;
	QComboBox _comboBox;
	QVBoxLayout _main_layout;

	bool _camera_is_on;

	void disassemble();
	void assemble();

public slots:
	void setFilter(QString);
	void setFile(QString);
	void setCamera();

};

#endif