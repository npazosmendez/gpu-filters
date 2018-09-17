#ifndef FILTERCONTROLS_H
#define FILTERCONTROLS_H

#include "FancySlider.hpp"
#include <QVBoxLayout>
#include <QComboBox>
class ImageFilter;
class CannyFilter;
class HoughFilter;


class FilterControls : public QWidget {
	Q_OBJECT
public:
	FilterControls(ImageFilter*) {};
	FilterControls(QWidget *parent = 0) : QWidget(parent) {};
private:
	QVBoxLayout _main_layout;
};

class CannyControls : public FilterControls {
	Q_OBJECT
public:
	CannyControls(CannyFilter*, QWidget *parent = 0);
private:
	CannyFilter* _filter;
	QVBoxLayout _main_layout;
	QCheckBox _checkBox;

	FancySlider _higher_slider;
	FancySlider _lower_slider;
};

class HoughControls : public FilterControls {
	Q_OBJECT
public:
	HoughControls(ImageFilter*, QWidget *parent = 0);
private:
	ImageFilter* _filter;
	QVBoxLayout _main_layout;
	QCheckBox _checkBox;

	FancySlider _higher_slider;
};

class NoControls : public FilterControls {
	Q_OBJECT
public:
	NoControls(ImageFilter*, QWidget *parent = 0);
};

#endif