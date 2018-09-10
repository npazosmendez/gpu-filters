#include "FilterControls.hpp"
#include "FancySlider.hpp"
#include "ImageFilter.hpp"
#include <QVBoxLayout>
#include <QComboBox>


NoControls::NoControls(ImageFilter* filter, QWidget *parent) : FilterControls(parent) {}

HoughControls::HoughControls(ImageFilter* filter, QWidget *parent) 
	: FilterControls(parent), _higher_slider("Hough param", 0 , 100), _filter(filter) {
	_main_layout.addWidget(&_higher_slider);
	setLayout(&_main_layout);
}

CannyControls::CannyControls(CannyFilter* filter, QWidget *parent) 
	: FilterControls(parent), _filter(filter), _higher_slider("Higher Threshold", 0 , 100), _lower_slider("Lower Threshold", 0 , 50){

	_main_layout.addWidget(&_higher_slider);
	_main_layout.addWidget(&_lower_slider);
	QObject::connect(&_higher_slider, SIGNAL(valueChanged(int)), _filter, SLOT(setHigherThreshold(int)));
	QObject::connect(&_lower_slider, SIGNAL(valueChanged(int)), _filter, SLOT(setLowerThreshold(int)));

	_filter->setHigherThreshold(_higher_slider.value());
	_filter->setLowerThreshold(_lower_slider.value());
	setLayout(&_main_layout);

	_higher_slider.setValue(70);
	_lower_slider.setValue(30);
}
