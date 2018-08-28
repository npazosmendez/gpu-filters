#include "FancySlider.hpp"

#include <QStyleOptionSlider>
#include <QToolTip>
#include <QGridLayout>
#include <QtGui>
#include <QSpinBox>

FancySlider::FancySlider(QString title, int min, int max, QWidget *parent)
    : QWidget(parent), _slider(Qt::Horizontal), _label(title) {

    // Set bounds
    _slider.setMinimum(min);
    _slider.setMaximum(max);
    _value.setMinimum(min);
    _value.setMaximum(max);

    // Connect the typing box with the slider
    QObject::connect(&_value, SIGNAL(valueChanged(int)), &_slider, SLOT(setValue(int)));
    QObject::connect(&_slider, SIGNAL(valueChanged(int)), &_value, SLOT(setValue(int)));

    // Replicate the "valueChanged" signal
    QObject::connect(&_value, SIGNAL(valueChanged(int)), this, SIGNAL(valueChanged(int)));
    QObject::connect(&_slider, SIGNAL(valueChanged(int)), this, SIGNAL(valueChanged(int)));

    // Arrange layout grid
    _layout.addWidget(&_label,0,0);
    _layout.addWidget(&_slider,1,0);
    _layout.addWidget(&_value,1,1);
    setLayout(&_layout);
}