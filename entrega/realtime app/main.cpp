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
#include "debug.h"

int main(int argc, char** argv) {
	debug_print("Creating QApplication\n");
	QApplication app(argc, argv);

	debug_print("Creating controls window\n");
	ControlsWindow controlsWindow(&app);

	controlsWindow.show();

	debug_print("Event loop executing\n");

    return app.exec();
}


