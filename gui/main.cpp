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
    
	QApplication app(argc, argv);

	ControlsWindow controlsWindow;

	controlsWindow.show();

    return app.exec();
}


