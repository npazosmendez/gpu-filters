#ifndef CONTROLSWINDOW_H
#define CONTROLSWINDOW_H

#include <QtGui>
#include <QLabel>
#include <QObject>
#include "FilterControls.hpp"
#include "ImageFilter.hpp"
#include "VideoWindow.hpp"
#include "camera.hpp"

class ControlsWindow : public QDialog {
	Q_OBJECT
public:
	ControlsWindow(QWidget *parent = 0);
	void show();

private:
	VideoWindow * _video_window;
	Camera * _camera;
	ImageFilter * _current_filter;
	ImageFilter * _filters[3];
	FilterControls* _controls;

	QVBoxLayout _main_layout;
	QComboBox _comboBox;

public slots:
	void setFilter(QString);

};

#endif