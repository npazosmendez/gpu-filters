#ifndef FILEBROWSER_H
#define FILEBROWSER_H

#include <QSlider>
#include <QToolTip>
#include <QLayout>
#include <QtGui>
#include <QSpinBox>
#include <QLabel>
#include <QString>

class FileBrowser : public QWidget
{
    Q_OBJECT
public:
    FileBrowser(QString title, QWidget *parent = 0);

private:
	QFileDialog _filedialog;
	QPushButton _button;
	QLabel _label;

	QGridLayout _layout;

private slots:
	void openBrowser();

signals:
	void fileSelected(QString);


};

#endif // FILEBROWSER_H