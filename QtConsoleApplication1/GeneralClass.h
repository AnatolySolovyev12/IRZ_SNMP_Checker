#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QDebug>
#include <iostream>
#include <QFile>
#include <QtCore/QCoreApplication>
#include <QElapsedTimer>
#include "MaxClass.h"
#include <Qapplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QInputDialog>


/*
void resetTimerForListHosts()
{
	qDebug() << "\nTimer for List Hosts was reset.";
	hostsList->restartCycleFunc();
}

*/



class GeneralClass  : public QObject
{
	Q_OBJECT

public:
	GeneralClass(QObject *parent);
	~GeneralClass();

	QByteArray prepareSnmpGet(const QByteArray& oidBytes);
	QByteArray encodeOidComponent(quint64 value);
	void exchangeFunc(QString host);
	bool readHostsFile();
	void mainFuncForCheck();

	void cmdOpen();
	void cmdClose();
	void iconActivated(QSystemTrayIcon::ActivationReason reason);

	bool getOIDfromFile();

private:
	QSystemTrayIcon* trayIcon = nullptr;
	QMenu* menu = nullptr;
	QAction* restoreActionOpenCLI = nullptr;
	QAction* restoreActionHideCLI = nullptr;
	QAction* quitAction = nullptr;

	MaxClass* messegeMaxClass = nullptr;

	QElapsedTimer * queryTimeChecker = nullptr;

	QTimer* recursionTimer = nullptr;

	QList<QString>hostsArr;

	QStringList problemDevice;

	QList<QString> snmpName = { "RouterModel", "SN", "SoftVer", "ModelGSM_1", "ModelGSM_2", "IMSI_1", "IMSI_2", "Reg_1", "Reg_2", "Operator_1", "Operator_2", "IP_1", "IP_2" };

	QList<QString> arrSNMP;
};

