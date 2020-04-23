#include "FileBrowser.hpp"


FileBrowser::FileBrowser(QString title, QWidget *parent)
    : QWidget(parent), _button("..."), _label(title){

    _button.setFixedSize(30,25);
    // Arrange layout grid
    _layout.addWidget(&_label,0,0);
    _layout.addWidget(&_button,0,1);

    QObject::connect(&_button, SIGNAL(clicked()), this, SLOT(openBrowser()));
    QObject::connect(&_filedialog, SIGNAL(fileSelected(QString)), this, SIGNAL(fileSelected(QString)));

    setLayout(&_layout);
}

void FileBrowser::openBrowser(){
    _filedialog.exec();
}