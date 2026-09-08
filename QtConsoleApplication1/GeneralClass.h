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

	QList<QString> arrSNMP =
	{
		// Info
		"1.3.6.1.4.1.35489.1.2.0", // infoModel       Ч модель роутера (например RL21l)
		"1.3.6.1.4.1.35489.1.3.0", // infoSN          Ч серийный номер
		"1.3.6.1.4.1.35489.1.4.0", // infoFV          Ч верси€ прошивки

		// model
		"1.3.6.1.4.1.35489.4.2.1.1.1",
		"1.3.6.1.4.1.35489.4.2.1.1.2",

		// IMSI
		"1.3.6.1.4.1.35489.4.2.1.24.1",
		"1.3.6.1.4.1.35489.4.2.1.24.2",

		// registration
		"1.3.6.1.4.1.35489.4.2.1.5.1",
		"1.3.6.1.4.1.35489.4.2.1.5.2",

		// operator
		"1.3.6.1.4.1.35489.4.2.1.4.1",
		"1.3.6.1.4.1.35489.4.2.1.4.2",

		// IP address
		"1.3.6.1.4.1.35489.4.2.1.11.1",
		"1.3.6.1.4.1.35489.4.2.1.11.2",

		/*// SIM number / slot index
        "1.3.6.1.4.1.35489.4.2.1.3.1",
        "1.3.6.1.4.1.35489.4.2.1.3.2",*/

		/*// revision
		"1.3.6.1.4.1.35489.4.2.1.2.1",
		"1.3.6.1.4.1.35489.4.2.1.2.2",*/

		/*// technology
		"1.3.6.1.4.1.35489.4.2.1.6.1",
		"1.3.6.1.4.1.35489.4.2.1.6.2",

		// signal strength
		"1.3.6.1.4.1.35489.4.2.1.7.1",
		"1.3.6.1.4.1.35489.4.2.1.7.2",

		// CSQ
		"1.3.6.1.4.1.35489.4.2.1.8.1",
		"1.3.6.1.4.1.35489.4.2.1.8.2",

		// upTime
		"1.3.6.1.4.1.35489.4.2.1.9.1",
		"1.3.6.1.4.1.35489.4.2.1.9.2",

		// connectTime (unix time)
		"1.3.6.1.4.1.35489.4.2.1.10.1",
		"1.3.6.1.4.1.35489.4.2.1.10.2",

		// PLMN
		"1.3.6.1.4.1.35489.4.2.1.12.1",
		"1.3.6.1.4.1.35489.4.2.1.12.2",

		// Cell ID
		"1.3.6.1.4.1.35489.4.2.1.13.1",
		"1.3.6.1.4.1.35489.4.2.1.13.2",

		// LAC
		"1.3.6.1.4.1.35489.4.2.1.14.1",
		"1.3.6.1.4.1.35489.4.2.1.14.2",

		// ICCID
		"1.3.6.1.4.1.35489.4.2.1.15.1",
		"1.3.6.1.4.1.35489.4.2.1.15.2",

		// IMEI
		"1.3.6.1.4.1.35489.4.2.1.16.1",
		"1.3.6.1.4.1.35489.4.2.1.16.2",

		// BAND
		"1.3.6.1.4.1.35489.4.2.1.17.1",
		"1.3.6.1.4.1.35489.4.2.1.17.2",

		// BANDS LTE
		"1.3.6.1.4.1.35489.4.2.1.18.1",
		"1.3.6.1.4.1.35489.4.2.1.18.2",

		// BANDS GSM/WCDMA
		"1.3.6.1.4.1.35489.4.2.1.19.1",
		"1.3.6.1.4.1.35489.4.2.1.19.2",

		// RSRP
		"1.3.6.1.4.1.35489.4.2.1.20.1",
		"1.3.6.1.4.1.35489.4.2.1.20.2",

		// RSRQ
		"1.3.6.1.4.1.35489.4.2.1.21.1",
		"1.3.6.1.4.1.35489.4.2.1.21.2",

		// RSSI
		"1.3.6.1.4.1.35489.4.2.1.22.1",
		"1.3.6.1.4.1.35489.4.2.1.22.2",

		// SINR
		"1.3.6.1.4.1.35489.4.2.1.23.1",
		"1.3.6.1.4.1.35489.4.2.1.23.2",
		*/
	};
};

