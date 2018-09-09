#ifndef FANCYSLIDER_H
#define FANCYSLIDER_H

#include <QSlider>
#include <QToolTip>
#include <QLayout>
#include <QtGui>
#include <QSpinBox>
#include <QLabel>

class FancySlider : public QWidget
{
    Q_OBJECT
public:
    FancySlider(QString title, int min, int max, QWidget *parent = 0);
    int value();

private:
	QLabel _label;
	QGridLayout _layout;
	QSlider _slider;
	QSpinBox _value;

signals:
	void valueChanged(int);
};

#endif // FANCYSLIDER_H