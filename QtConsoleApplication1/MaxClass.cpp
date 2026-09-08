#include "MaxClass.h"

MaxClass::MaxClass(QObject * parent)
	: QObject(parent), manager(new QNetworkAccessManager)
{
	AttachConsole(ATTACH_PARENT_PROCESS);
	getTokenFromFile();
	chatId = getChatIdFromFile();

	connect(this, &MaxClass::sendIdNotificationForDelete, this, &MaxClass::deleteNotification);

	//QTimer::singleShot(2000, [this]() { getLastMessageAsync(); });
}



void MaxClass::sendMessage(const QString message)
{
	if (message.isEmpty())
	{
		qWarning() << "Attempt to send empty message";
		return;
	}

	if (instanceNumber.isEmpty() || tokenFromInstance.isEmpty())
	{
		qWarning() << "Token or Instance is empty. Restart APP with correct parameters for send messege";
		return;
	}

	QString urlStringTemp = QString(R"(https://3100.api.green-api.com/waInstance%1/sendMessage/%2)")
		.arg(instanceNumber)
		.arg(tokenFromInstance);

	QUrl url(urlStringTemp);

	QJsonObject json;
	json["chatId"] = chatId;
	json["message"] = message; // Используем переданное сообщение

	// Преобразование JSON-объекта в строку
	QJsonDocument jsonDoc(json);
	QByteArray jsonData = jsonDoc.toJson();

	// Создание запроса
	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	// Отправка запроса
	QNetworkReply* reply = manager->post(request, jsonData);

	// Обработчик ответа (если необходимо). Пригодится.
	QObject::connect(reply, &QNetworkReply::finished, [reply]() {

		if (reply->error() == QNetworkReply::NoError)
		{
			QString response = reply->readAll();
			qDebug() << response;
		}
		else
			qDebug() << "Error:: " << reply->error();

		reply->deleteLater();
		});
}



void MaxClass::getTokenFromFile()
{
	//Берем за основу любой метод из API GreenAPI в котором фигурирует номер инстанса и токен

	QFile file(QCoreApplication::applicationDirPath() + "\\token.txt");

	if (!file.open(QIODevice::ReadOnly))
	{
		qDebug() << "Don't find browse file. Add a directory with a token (token.txt).";
		return;
	}

	QTextStream out(&file);

	QString myLine = out.readLine(); // метод readLine() считывает одну строку из потока

	if (myLine == "")
	{
		qDebug() << "Don't find browse file. Add a directory with a token (token.txt).";

		file.close();
		return;
	}

	file.close();

	// Через регулярку получаем сначала номер инстанса

	QRegularExpression strPattern(QString(R"(waInstance([0-9]*))"));

	QRegularExpressionMatch matchReg = strPattern.match(myLine);

	if (matchReg.hasMatch())
	{
		instanceNumber = matchReg.captured().replace("waInstance", ""); // избавляемся от приставки заменяя её на пустоту
		qDebug() << "instanceNumber = " + instanceNumber;
	}
	else
		qDebug() << "No matches in RegEx for waInstance";

	// Полученный номер инстанса добавляем к паттерну и ищем совпадения для токена которым завершается любой метод из API

	strPattern.setPattern(QString(R"(waInstance%1/[\w]+/([\w]+))").arg(instanceNumber)); // группируем токен в отдельную группу в круглых скобках чтобы в дальнейшем извлечь через индекс

	matchReg = strPattern.match(myLine);

	if (matchReg.hasMatch())
	{
		tokenFromInstance = matchReg.captured(1); // извлекаем индексированную первую скобку в паттерне. (0) или () извлекает всё совпадение. (1) - то что было опоясано в паттерне круглыми скобками
		qDebug() << "tokenFromInstance = " + tokenFromInstance;
	}
	else
		qDebug() << "No matches in RegEx for tokenFromInstance";

	return;
}



QString MaxClass::getChatIdFromFile()
{
	QFile file(QCoreApplication::applicationDirPath() + "\\chatId.txt");

	if (!file.open(QIODevice::ReadOnly))
	{
		qDebug() << "Don't find browse file. Add a directory with a token (chatId.txt).";
		return 0;
	}

	QTextStream out(&file);

	QString myLine = out.readLine(); // метод readLine() считывает одну строку из потока

	if (myLine == "")
	{
		qDebug() << "Don't find browse file. Add a directory with a token (chatId.txt).";
		file.close();
		return 0;
	}

	file.close();

	qDebug() << "";

	return myLine;
}



void MaxClass::getLastMessageAsync()
{
	if (isBusy) return;

	isBusy = true;

	QString urlStringTemp = QString(R"(https://3100.api.green-api.com/waInstance%1/receiveNotification/%2)")
		.arg(instanceNumber)
		.arg(tokenFromInstance);

	QUrl url(urlStringTemp);

	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	QNetworkReply* reply = manager->get(request);

	QObject::connect(reply, &QNetworkReply::finished, [this, reply]() {

		if (reply->error() == QNetworkReply::NoError)
		{
			QByteArray responseData = reply->readAll();
			QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);

			if (!responseDoc.isNull())
			{
				qDebug() << "\nreceiptId: " << responseDoc["receiptId"].toInt();

				QJsonObject objBody = responseDoc["body"].toObject();
				QJsonObject objMessage = objBody["messageData"].toObject();
				QJsonObject objText = objMessage["textMessageData"].toObject();

				qDebug() << "text: " << objText["textMessage"].toString();

				QJsonObject objId = objBody["senderData"].toObject();

				qDebug() << "chatId: " << objId["chatId"].toString();

				emit sendIdNotificationForDelete(QString::number(responseDoc["receiptId"].toInt()));

				if (objBody["typeWebhook"].toString() != "outgoingAPIMessageReceived")
				{
					qDebug() << "Send messege...";
					emit lastMessageReceived(qMakePair<QString, QString>(objId["chatId"].toString(), objText["textMessage"].toString()));
				}
			}
			else
				std::cout << "\r" << QDate::currentDate().toString().toStdString() << "   " << QTime::currentTime().toString().toStdString();
		}
		else
			qDebug() << "Error:" << reply->errorString();

		reply->deleteLater();
		isBusy = false;

		// Самовызов через 3 секунды
		QTimer::singleShot(3000, this, &MaxClass::getLastMessageAsync);
		});
}



void MaxClass::deleteNotification(QString idNotification)
{
	QString urlStringTemp = QString(R"(https://3100.api.green-api.com/waInstance3100514553/deleteNotification/%2)")
		.arg(instanceNumber)
		.arg(tokenFromInstance);

	urlStringTemp += idNotification;

	QUrl url(urlStringTemp);

	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	QNetworkReply* reply = manager->deleteResource(request);

	QObject::connect(reply, &QNetworkReply::finished, [this, reply]() {

		if (reply->error() == QNetworkReply::NoError)
		{
			QByteArray responseData = reply->readAll();
			QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);

			if (!responseDoc.isNull())
			{
				if (!responseDoc["result"].toBool())
					qDebug() << responseDoc["reason"].toString();
				else
					qDebug() << "Notification was delete";
			}
			else
				qDebug() << "Not array or null";
		}
		else
			qDebug() << "Error:" << reply->errorString();

		reply->deleteLater();
		}
	);
}