#ifndef CONTROLSWINDOW_H
#define CONTROLSWINDOW_H

#include <QtGui>
#include <QLabel>
#include <QObject>
#include "FilterControls.hpp"
#include "ImageFilter.hpp"

class ControlsWindow : public QDialog {
	Q_OBJECT
public:
	ControlsWindow(QWidget *parent = 0);
	void show();

private:
	FilterControls* _controls;

	QVBoxLayout _main_layout;
	QComboBox _comboBox;
	ImageFilter * _filters[3];

public slots:
	void setFilter(QString);
signals:
	void filterChanged(ImageFilter*);
};

#endif