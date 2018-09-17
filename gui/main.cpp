#include <iostream>
#include <string>

using namespace std;

#include <QtGui>
#include <QLabel>
#include <QObject>

#include "VideoWindow.hpp"
#include "FancySlider.hpp"
#include "FilterControls.hpp"
#include "ControlsWindow.hpp"

int main(int argc, char** argv) {
    qDebug() << "Creating QApplication";
	QApplication app(argc, argv);

    qDebug() << "Creating controls window";
	ControlsWindow controlsWindow;

	controlsWindow.show();

    qDebug() << "Event loop executing";
    return app.exec();
}


