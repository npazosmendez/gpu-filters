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
	ControlsWindow(QApplication * app, QWidget *parent = 0);
	void show();

private:

	QApplication * _app;

	VideoWindow * _video_window;
	VideoStreamer * _camera;
	ImageFilter * _current_filter;
	ImageFilter * _filters[4];
	FilterControls* _controls;

	QPushButton _camera_button;
	FileBrowser _browser;
	QComboBox _comboBox;
	QVBoxLayout _main_layout;

	bool _camera_is_on;

	void disassemble();
	void assemble();
	void empty_pipe();

protected:
	void keyPressEvent(QKeyEvent * e);

public slots:
	void setFilter(QString);
	void setFile(QString);
	void setCamera();

signals:
	void yyy();

};

#endif