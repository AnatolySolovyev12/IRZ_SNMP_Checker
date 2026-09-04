#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QDebug>
#include <iostream>
#include <QFile>
#include <QCoreApplication>

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

private:
	QList<QString>hostsArr;
};

