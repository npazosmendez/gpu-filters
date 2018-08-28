#ifndef FILTERCONTROLS_H
#define FILTERCONTROLS_H

#include "FilterControls.hpp"
#include "FancySlider.hpp"
#include <QVBoxLayout>
#include <QComboBox>

class FilterControls : public QWidget {
	Q_OBJECT
public:
	FilterControls(QWidget *parent = 0);
private:
	QVBoxLayout _main_layout;
	FancySlider _higher_slider;
	FancySlider _lower_slider;
};

#endif