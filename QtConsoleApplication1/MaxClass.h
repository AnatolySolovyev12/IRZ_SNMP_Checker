#pragma once

#include <QCoreApplication>
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QUrlQuery>
#include <windows.h>
#include <QFile.h>
#include <iostream>
#include <QRegularExpression>


class MaxClass : public QObject
{
	Q_OBJECT

public:
	MaxClass(QObject* parent = nullptr);
	void getTokenFromFile();
	QString getChatIdFromFile();
	void getLastMessageAsync();
	void deleteNotification(QString idNotification);

signals:
	void lastMessageReceived(QPair<QString, QString>);
	void sendIdNotificationForDelete(QString id);

public slots:
	void sendMessage(const QString message);

private:
	QNetworkAccessManager* manager = nullptr;
	QString chatId = "";
	QString urlString = "";
	QString instanceNumber = "";
	QString tokenFromInstance = "";
	bool isBusy = false;
};