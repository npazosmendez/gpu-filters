#include "FilterControls.hpp"
#include "FancySlider.hpp"
#include <QVBoxLayout>
#include <QComboBox>

FilterControls::FilterControls(QWidget *parent) 
	: QWidget(parent), _higher_slider("Higher Threshold", 0 , 100), _lower_slider("Lower Threshold", 0 , 50){
	_main_layout.addWidget(&_higher_slider);
	_main_layout.addWidget(&_lower_slider);
	setLayout(&_main_layout);
}
