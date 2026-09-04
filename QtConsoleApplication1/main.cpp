#include <QtCore/QCoreApplication>
#include <QUdpSocket>
#include <QDebug>
#include <iostream>


QByteArray prepareSnmpGet(const QByteArray& oidBytes) {
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



QByteArray encodeOidComponent(quint64 value)
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



int main(int argc, char* argv[])
{
	QCoreApplication app(argc, argv);

	QUdpSocket udpSocket;
	QHostAddress routerAddress("10.86.146.118");
	//QHostAddress routerAddress("10.0.245.138");

	quint16 snmpPort = 161;

	// 30 - признак самого SNMP запроса
	// 29 - размер всего что идёт после (41)
	// 02 - маркер типа Integer
	// 01 - длина поля 1 байт
	// 01 - версия протокола (v2c)
	// 04 - маркер типа OctetString
	// 06 - длина поля
	// 7075626c6963 - Comunity = public (в ASCII)

	// Запрос и его подноготная 
	// a0 - маркер контекстного типа Get-Request PDU
	// 1c - размер этого PDU (28 байт = всё, что идёт после)            
	// 02 - маркер типа INTEGER (для Request ID)
	// 04 - длина поля (4 байта)
	// 00000001 - само значение Request ID
	// 02 - маркер типа INTEGER (для Error Status)
	// 01 - длина поля (1 байт)
	// 00 - значение ошибки: noError (0)
	// 02 - маркер типа INTEGER (для Error Index)
	// 01 - длина поля (1 байт)
	// 00 - значение индекса ошибки: 0

	// Список переменных (Varbind List)
	// 30 - отделитель и список переменных
	// 0e - размер вложенного пакета (14 = всё что идёт после)

	// Конкретная переменная (Varbind) - OID, его маркер, размеры и окончание
	// 30 - маркер первой переменной OID + value
	// 0c - размер переменной (12 = количество байт идущее после)
	// 06 - маркер запроса
	// 08 - длина запроса  
	// OID
	// 0500 - конец запроса

	//QByteArray generalPacket = "302902010104067075626c6963a01c020400000001020100020100300e300c0608";
	//302c02010104067075626c6963a01f02040e33fb7d0201000201003011300f060b

	//2b06010201010500 - имя
	//2b06010401829521010200 - модель
	//QByteArray queryPacket = "2b06010201010500"; // OID 1.3.6.1.2.1.1.5.0 - получение имени
	//QByteArray ending = "0500";
	//QByteArray fullPacket = generalPacket + queryPacket + ending;
	//QByteArray binaryPacket = QByteArray::fromHex(fullPacket);

	QByteArray nameOID = "2b06010201010500"; // 1.3.6.1.2.1.1.5.0  имя
	QByteArray modelOID = "2b06010401829521010200"; // 1.3.6.1.4.1.35489.1.2.0  модель

	QList<QString>routerMask = { "TCP", routerAddress.toString() };

	QList<QString> arrSNMP = 
	{
   // Info
   "1.3.6.1.4.1.35489.1.2.0", // infoModel       — модель роутера (например RL21l)
   "1.3.6.1.4.1.35489.1.3.0", // infoSN          — серийный номер
   "1.3.6.1.4.1.35489.1.4.0", // infoFV          — версия прошивки

   // model
   "1.3.6.1.4.1.35489.4.2.1.1.1",
   "1.3.6.1.4.1.35489.4.2.1.1.2",

   // SIM number / slot index
   "1.3.6.1.4.1.35489.4.2.1.3.1",
   "1.3.6.1.4.1.35489.4.2.1.3.2",

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
						routerMask << val << finalAnswer;
					}
				}
				else
					qDebug() << "Error: OID not found";
			}
		}
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

	if((routerMask[17] == "UNKNOWN" && routerMask[19] == "UNKNOWN") || (routerMask[21] == "0" && routerMask[23] == "0") || (routerMask[25] == "UNKNOWN" && routerMask[27] == "UNKNOWN") || (routerMask[29] == "0.0.0.0" && routerMask[31] == "0.0.0.0"))
		qDebug() << "HUETA";

	return app.exec();
}


/*
TCP   10.86.146.118
1.3.6.1.4.1.35489.1.2.0   RL21l
1.3.6.1.4.1.35489.1.3.0   RDDC1000802
1.3.6.1.4.1.35489.1.4.0   20.8
1.3.6.1.4.1.35489.4.2.1.1.1   UNKNOWN
1.3.6.1.4.1.35489.4.2.1.1.2   QUECTEL EC25
1.3.6.1.4.1.35489.4.2.1.3.1   1
1.3.6.1.4.1.35489.4.2.1.3.2   2
1.3.6.1.4.1.35489.4.2.1.24.1   UNKNOWN
1.3.6.1.4.1.35489.4.2.1.24.2   UNKNOWN
1.3.6.1.4.1.35489.4.2.1.5.1   0
1.3.6.1.4.1.35489.4.2.1.5.2   0
1.3.6.1.4.1.35489.4.2.1.4.1   UNKNOWN
1.3.6.1.4.1.35489.4.2.1.4.2   UNKNOWN
1.3.6.1.4.1.35489.4.2.1.11.1   0.0.0.0
1.3.6.1.4.1.35489.4.2.1.11.2   0.0.0.0
*/