#include <QtCore/QCoreApplication>
#include <QUdpSocket>
#include <QDebug>

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

int main(int argc, char* argv[])
{
	QCoreApplication app(argc, argv);

	QUdpSocket udpSocket;
	QHostAddress routerAddress("10.86.146.118");
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


	QByteArray generalPacket = "302902010104067075626c6963a01c020400000001020100020100300e300c0608";
	//302c02010104067075626c6963a01f02040e33fb7d0201000201003011300f060b

//2b06010201010500 - имя
//2b06010401829521010200 - модель
	QByteArray queryPacket = "2b06010201010500"; // OID 1.3.6.1.2.1.1.5.0 - получение имени


	QByteArray ending = "0500";

	QByteArray fullPacket = generalPacket + queryPacket + ending;

	QByteArray binaryPacket = QByteArray::fromHex(fullPacket);



	//QByteArray packet1 = prepareSnmpGet(QByteArray::fromHex("2b06010201010500"));
	QByteArray packet1 = prepareSnmpGet(QByteArray::fromHex("2b06010401829521010200"));



	udpSocket.writeDatagram(packet1, routerAddress, snmpPort);

	qDebug() << "TX >> " << packet1;

	// Ожидание и чтение ответа
	if (udpSocket.waitForReadyRead(3000))
	{
		while (udpSocket.hasPendingDatagrams())
		{
			QByteArray responseData;

			responseData.resize(udpSocket.pendingDatagramSize()); // подгоняем размер массива под размер пришеднего ответа

			udpSocket.readDatagram(responseData.data(), responseData.size());

			qDebug() << "RX <<" << responseData.toHex();

			int byteIndex = responseData.indexOf(QByteArray::fromHex(queryPacket)); // OID (Object Identifier) запрос имени устрйоства
			qDebug() << "Index in bytes:" << byteIndex;

			if (byteIndex != -1) {
				// 2. Обрезаем всё, что ДО нашего OID
				responseData.remove(0, byteIndex);
				qDebug() << "After slice to OID:" << responseData.toHex();

				int lengthPos = 8 + 1;

				//9 - й байт(индекс 8) — это маркер типа 0x04 (OctetString).10 - й байт(индекс 9) — это длина строки.

				if (lengthPos < responseData.size())
				{
					// Получаем байт длины строки после 
					quint8 stringLength = static_cast<quint8>(responseData.at(lengthPos));

					qDebug() << "Length data name device = " << stringLength;

					// Вырезаем саму строку, которая начинается сразу после байта длины
					QByteArray nameBytes = responseData.mid(lengthPos + 1, stringLength);

					// Преобразуем последовательность байт в строку
					QString deviceName = QString::fromLocal8Bit(nameBytes);
					qDebug() << "Name device:" << deviceName;
				}
			}
			else
			{
				qDebug() << "Error: OID not found";
			}
		}
	}

	return app.exec();
}