#include "GeneralClass.h"

GeneralClass::GeneralClass(QObject *parent)
	: QObject(parent)
{
	if (readHostsFile())
	{
		for(auto& val : hostsArr)
			exchangeFunc(val);
	}
}



GeneralClass::~GeneralClass()
{}



QByteArray GeneralClass::prepareSnmpGet(const QByteArray& oidBytes) 
{
	QByteArray packet;

	// 1. Фиксированная часть PDU внутри GetRequest (ID транзакции, статус ошибки, индекс ошибки)
	QByteArray pduPayload;
	pduPayload.append("\x02\x04\x00\x00\x00\x01", 6); // Request ID: 1
	pduPayload.append("\x02\x01\x00", 3);             // Error Status: noError
	pduPayload.append("\x02\x01\x00", 3);             // Error Index: 0

	// 2. Сборка ОДИНОЧНОГО Varbind (OID + Null-value в конце)
	QByteArray varbindNode;
	varbindNode.append("\x06");                            // Маркер OID
	varbindNode.append(static_cast<char>(oidBytes.size())); // Длина OID
	varbindNode.append(oidBytes);                          // Сам OID в байтах
	varbindNode.append("\x05\x00", 2);                     // Конец элемента (Value = Null)

	// 3. Упаковываем элемент в контейнер Varbind (маркер 0x30)
	QByteArray varbindContainer;
	varbindContainer.append("\x30");
	varbindContainer.append(static_cast<char>(varbindNode.size()));
	varbindContainer.append(varbindNode);

	// 4. Упаковываем в общий Varbind List (маркер 0x30)
	QByteArray varbindList;
	varbindList.append("\x30");
	varbindList.append(static_cast<char>(varbindContainer.size()));
	varbindList.append(varbindContainer);

	// 5. Объединяем payload и varbindList в общий GetRequest блок (маркер 0xa0)
	QByteArray getRequest;
	getRequest.append("\xa0");
	getRequest.append(static_cast<char>(pduPayload.size() + varbindList.size()));
	getRequest.append(pduPayload);
	getRequest.append(varbindList);

	// 6. Собираем финальный пакет (Версия + Community + GetRequest)
	packet.append("\x30"); // Корневой контейнер

	// Считаем полную длину внутренностей: 3 байта (версия) + 8 байт (community public) + размер getRequest
	char totalLength = static_cast<char>(3 + 8 + getRequest.size());
	packet.append(totalLength);

	packet.append("\x02\x01\x01", 3);   // Version: 2c
	packet.append("\x04\x06public", 8); // Community: public
	packet.append(getRequest);          // Весь блок запроса

	return packet;
}



QByteArray GeneralClass::encodeOidComponent(quint64 value)
{
	if (value == 0)
		return QByteArray(1, '\x00');

	// Сначала получаем 7-битные группы (младшие сначала)
	QList<quint8> groups;
	while (value > 0)
	{
		groups.prepend(static_cast<quint8>(value & 0x7F));
		value >>= 7;
	}

	// Теперь ставим флаг продолжения (0x80) на все байты, кроме последнего
	QByteArray res;
	for (int i = 0; i < groups.size(); ++i)
	{
		quint8 b = groups[i];
		if (i != groups.size() - 1)
			b |= 0x80;
		res.append(static_cast<char>(b));
	}
	return res;
}



void GeneralClass::exchangeFunc(QString host)
{
	qDebug() << "SNMP query to " << host << '\n';

	QUdpSocket udpSocket;
	QHostAddress routerAddress(host);
	quint16 snmpPort = 161;

	QList<QString>routerMask = { "TCP", routerAddress.toString() };

	for (auto& val : arrSNMP)
	{
		QString fullStringHexOID;
		QString decOID = val;
		int counterForOID = 0;

		if (!decOID.isEmpty())
		{
			int first;
			int second;
			QString temp;

			for (QString val : decOID)
			{
				if (val == ".")
				{
					if (counterForOID == 0)
						first = temp.toInt();

					if (counterForOID == 1)
					{
						second = temp.toInt();
						fullStringHexOID += QString("%1").arg(QString::number((40 * first + second), 16), 2, QChar('0'));
					}

					if (counterForOID > 1)
					{
						if (temp.toInt() > 127)
							fullStringHexOID += encodeOidComponent(temp.toInt()).toHex();
						else
							fullStringHexOID += QString("%1").arg(QString::number(temp.toInt(), 16), 2, QChar('0'));
					}

					++counterForOID;
					temp.clear();
					continue;
				}

				temp += val;
			}

			fullStringHexOID += QString("%1").arg(QString::number(temp.toInt(), 16), 2, QChar('0'));
			temp.clear();
		}

		QByteArray sendOID = QByteArray::fromHex(fullStringHexOID.toUtf8());

		qDebug() << "sendOID" << sendOID.toHex();

		QByteArray packet = prepareSnmpGet(sendOID);

		udpSocket.writeDatagram(packet, routerAddress, snmpPort);

		qDebug() << "TX >> " << packet.toHex();

		// Ожидание и чтение ответа
		if (udpSocket.waitForReadyRead(3000))
		{
			while (udpSocket.hasPendingDatagrams())
			{
				QByteArray responseData;

				responseData.resize(udpSocket.pendingDatagramSize()); // подгоняем размер массива под размер пришеднего ответа

				udpSocket.readDatagram(responseData.data(), responseData.size());

				qDebug() << "RX <<" << responseData.toHex();

				int byteIndex = responseData.indexOf(sendOID); // OID (Object Identifier) запрос имени устрйоства
				qDebug() << "Index in bytes:" << byteIndex;

				if (byteIndex != -1)
				{
					// 2. Обрезаем всё, что ДО нашего OID
					responseData.remove(0, byteIndex);
					qDebug() << "After slice to OID:" << responseData.toHex();

					int lengthPos = sendOID.length() + 1;

					//9 - й байт(индекс 8) — это маркер типа 0x04 (OctetString).10 - й байт(индекс 9) — это длина строки.

					if (lengthPos < responseData.size())
					{
						// Получаем байт длины строки после 
						quint8 stringLength = static_cast<quint8>(responseData.at(lengthPos));

						quint8 typeData = static_cast<quint8>(responseData.at(sendOID.length()));

						qDebug() << "Type data = " << typeData;

						qDebug() << "Length data answer = " << stringLength;

						// Вырезаем саму строку, которая начинается сразу после байта длины
						QByteArray nameBytes = responseData.mid(lengthPos + 1, stringLength);

						// Преобразуем последовательность байт в строку

						QString finalAnswer;

						if (typeData == 4)
						{
							finalAnswer = QString::fromLocal8Bit(nameBytes);
						}
						else if (typeData == 2)
						{
							finalAnswer = QString::number(QString(nameBytes.toHex()).toInt());
						}
						else if (typeData == 129)
						{
							finalAnswer = "noSuchInstance";
						}
						else if (typeData == 128)
						{
							finalAnswer = "noSuchObject";
						}
						else if (typeData == 64)
						{
							quint8 b0 = static_cast<quint8>(nameBytes[0]);
							quint8 b1 = static_cast<quint8>(nameBytes[1]);
							quint8 b2 = static_cast<quint8>(nameBytes[2]);
							quint8 b3 = static_cast<quint8>(nameBytes[3]);

							QString ip = QString("%1.%2.%3.%4")
								.arg(b0).arg(b1).arg(b2).arg(b3);

							finalAnswer = ip;
						}

						qDebug() << "Answer:" << finalAnswer << "\n\n\n";
						routerMask << val + "   " + snmpName[arrSNMP.indexOf(val)] + "   " << finalAnswer;
					}
				}
				else
					qDebug() << "Error: OID not found";
			}
		}
	}

	if (routerMask.length() <= 2) 
	{
		qDebug() << "HUETA"; //////////////////////////////

		qDebug() << "\n\n\n";
		return;
	}

	int counter = 0;
	for (auto& val : routerMask)
	{
		if (counter == 2)
		{
			std::cout << '\n';
			counter = 0;
		}
		std::cout << val.toStdString() << "   ";
		counter++;
	}

	if ((routerMask[13] == "UNKNOWN" && routerMask[15] == "UNKNOWN") || (routerMask[17] == "0" && routerMask[19] == "0") || (routerMask[21] == "UNKNOWN" && routerMask[23] == "UNKNOWN") || (routerMask[25] == "0.0.0.0" && routerMask[27] == "0.0.0.0"))
		qDebug() << "\n\nHUETA"; //////////////////////////////

	qDebug() << "\n\n\n";
}



bool GeneralClass::readHostsFile()
{
	QFile file(QCoreApplication::applicationDirPath() + "\\hosts.txt");

	if (!file.open(QIODevice::ReadOnly))
	{
		qDebug() << "Don't find hosts file. Create file and try again";
		return false;
	}

	QTextStream out(&file);

	bool portBool = false;
	QString ip;
	QString port;
	QString* myLine = new QString();

	while (out.readLineInto(myLine, 0))
	{
		hostsArr.push_back(*myLine);
		ip.clear();
		port.clear();
		portBool = false;
	}

	delete myLine;
	myLine = nullptr;

	file.close();

	qDebug() << hostsArr;
	qDebug() << "Count of Hosts = " << hostsArr.length() << "\n\n\n";

	if (hostsArr.length() > 0)
		return true;
	else
		return false;
}