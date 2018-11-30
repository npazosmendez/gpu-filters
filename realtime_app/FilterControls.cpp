#include "FilterControls.hpp"
#include "FancySlider.hpp"
#include "ImageFilter.hpp"
#include <QVBoxLayout>
#include <QComboBox>


NoControls::NoControls(ImageFilter*, QWidget *parent) : FilterControls(parent) {}

HoughControls::HoughControls(ImageFilter* filter, QWidget *parent) 
	: FilterControls(parent), _filter(filter), _checkBox("OpenCL"),
	_higher_canny_slider("Canny's H.T.", 0 , 100),
	_lower_canny_slider("Canny's L.T.", 0 , 100)
	 {

	_main_layout.addWidget(&_checkBox);

	QObject::connect(&_checkBox, SIGNAL(stateChanged(int)), filter, SLOT(toggle_CL(int)));

	// _main_layout.addWidget(&_higher_canny_slider);
	// _main_layout.addWidget(&_lower_canny_slider);
	// QObject::connect(&_higher_canny_slider, SIGNAL(valueChanged(int)), _filter, SLOT(setCannyHigherThreshold(int)));
	// QObject::connect(&_lower_canny_slider, SIGNAL(valueChanged(int)), _filter, SLOT(setCannyLowerThreshold(int)));

	_checkBox.setChecked(true);
	setLayout(&_main_layout);
}

KanadeControls::KanadeControls(ImageFilter* filter, QWidget *parent) 
	: FilterControls(parent), _filter(filter), _checkBox("OpenCL")
	 {

	_main_layout.addWidget(&_checkBox);

	QObject::connect(&_checkBox, SIGNAL(stateChanged(int)), filter, SLOT(toggle_CL(int)));

	_checkBox.setChecked(true);
	setLayout(&_main_layout);
}

CannyControls::CannyControls(CannyFilter* filter, QWidget *parent) 
	: FilterControls(parent), _filter(filter), _checkBox("OpenCL"),
	_higher_slider("Higher Threshold", 0 , 300), _lower_slider("Lower Threshold", 0 , 100){

	_main_layout.addWidget(&_checkBox);
	_main_layout.addWidget(&_higher_slider);
	_main_layout.addWidget(&_lower_slider);
	QObject::connect(&_higher_slider, SIGNAL(valueChanged(int)), _filter, SLOT(setHigherThreshold(int)));
	QObject::connect(&_lower_slider, SIGNAL(valueChanged(int)), _filter, SLOT(setLowerThreshold(int)));

	_filter->setHigherThreshold(_higher_slider.value());
	_filter->setLowerThreshold(_lower_slider.value());

	_higher_slider.setValue(170);
	_lower_slider.setValue(70);

	QObject::connect(&_checkBox, SIGNAL(stateChanged(int)), filter, SLOT(toggle_CL(int)));

	_checkBox.setChecked(true);
	setLayout(&_main_layout);
}
