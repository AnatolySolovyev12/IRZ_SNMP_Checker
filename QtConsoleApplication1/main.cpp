#include <QtCore/QCoreApplication>
#include "GeneralClass.h"

#include <qwidget.h>
#include <QApplication>
#include <Windows.h>

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	GeneralClass * myGeneral = new GeneralClass(nullptr);
	
	return app.exec();
}


