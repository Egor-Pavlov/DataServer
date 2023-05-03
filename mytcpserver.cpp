#include "mytcpserver.h"
#include <QDebug>
#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include <QtSql>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>


MyTcpServer::MyTcpServer()
{

    if(this->listen(QHostAddress::Any, 2323))
    {
        qDebug() << "server is started";
    }
    else
    {
        qDebug() << "server is not started";
    }
}

void MyTcpServer::incomingConnection(qintptr socketDescriptor)
{
    Socket = new QTcpSocket;
    Socket->setSocketDescriptor(socketDescriptor);
    connect(Socket, &QTcpSocket::readyRead, this, &MyTcpServer::slotReadyRead);
    connect(Socket, &QTcpSocket::disconnected, this, &MyTcpServer::slotClientDisconnected);
    sockets.push_back(Socket);
    qDebug() << "connected " << Socket->socketDescriptor();
}

QJsonObject MyTcpServer::getLatestCoords(int roomId, QDateTime time, int interval = 60)//интервал - отрезок от начала промежутка врмени
                                                                                          //за который получаем координаты до конца
{
    QSqlDatabase db = QSqlDatabase::database(); // получаем активное подключение к базе данных

    // запрос для получения координат устройств в заданном помещении, записанных не более чем за 1 минуту до указанного времени
    QString queryStr = "SELECT DISTINCT ON(devices.mac_address) "
            "devices.mac_address, "
            "devices.name, "
            "coordinates.x, "
            "coordinates.y, "
            "coordinates.room_id, "
            "coordinates.timestamp "
        "FROM "
            "coordinates "
        "JOIN devices ON coordinates.device_id = devices.id "
        "WHERE "
            "coordinates.room_id = :roomId AND "
            "coordinates.timestamp > :timeFrom AND "
            "coordinates.timestamp < :timeTo "
        "ORDER BY "
            "devices.mac_address, coordinates.timestamp DESC;";

    QSqlQuery query(db);
    query.prepare(queryStr);
    query.bindValue(":roomId", roomId);
    query.bindValue(":timeFrom", time.addSecs(-interval)); // отнимаем количество секунд от заданного времени
    query.bindValue(":timeTo", time);
    query.exec();

    //пакуем каждый объект в элемент массива json
    QJsonArray coordsArray;
    while (query.next())
    {
        QJsonObject coordObject;
        coordObject.insert("mac", query.value(0).toString());
        coordObject.insert("name", query.value(1).toString());
        coordObject.insert("x", query.value(2).toDouble());
        coordObject.insert("y", query.value(3).toDouble());
//        coordObject.insert("roomId", query.value(4).toInt());
        coordObject.insert("dateTime", query.value(5).toDateTime().toString(Qt::ISODate));
        coordsArray.append(coordObject);
    }

    //заполняем файл json массивом
    QJsonObject resultObject;
    resultObject.insert("coords", coordsArray);

    return resultObject;
}

void MyTcpServer::slotReadyRead()
{
    Socket = (QTcpSocket*)sender();
    qintptr socketDescriptor = Socket->socketDescriptor();
    QDataStream input(Socket);
    input.setVersion(QDataStream::Qt_6_4);

    if(input.status() == QDataStream::Ok)
    {
        while(true)
        {
            //достаточно ли данных для чтения размера блока
            if (Socket->bytesAvailable() < (int)sizeof(quint32))
            {
                break;
            }
            //если да, то читаем блок
            quint32 size;
            QDataStream in(Socket);
            in >> size;
            qDebug() << size;
            //если блок пришел целиком
            if (Socket->bytesAvailable() < size)
            {
                break;
            }

            QString str;
            input >> str;
            qDebug() << str;
            if(str != "")
            {

                QDateTime DateTime;
                try
                {
                    //пытаемся конвертировать
                    DateTime = QDateTime::fromString(str, "dd.MM.yyyy H:mm:ss");
                    qDebug() << DateTime;
                }
                catch(...)
                {
                    //если не конвертировалось то выходим
                    break;
                }
                //qDebug()<<Generator.createJsonData();

                // Создаем подключение к базе данных PostgreSQL
                QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
                db.setHostName("localhost");
                db.setPort(1111);
                db.setDatabaseName("Coords");
                db.setUserName("generator");
                db.setPassword("1234");

                // Открываем подключение к базе данных
                if (!db.open())
                {
                    qDebug() << "Failed to connect to the database" << db.lastError().text();
                    //отправка сообщения об ошибке подключения
                }
                else
                {
                    qDebug() << "Succesfully connected!";
                    //запрос
                    QJsonObject resultObject = getLatestCoords(1, DateTime, 60);

                    QJsonDocument jsonDoc(resultObject);
                    //возвращаем в виде строки чтобы потом отправить
                    QString result = jsonDoc.toJson(QJsonDocument::Indented);

                    QJsonArray jsonCoords = resultObject["coords"].toArray();
                    //проверяем чтобы не отправлять пустой json
                    if(!jsonCoords.isEmpty())
                    {
                        qDebug() << result;
                        //отправка
                        SendToClient(result, socketDescriptor);

                    }

                    db.close();
                }
            }
            break;
        }
    }
    else
    {
        qDebug() << "DataStream err";
    }
}

void MyTcpServer::SendToClient(QString str, qintptr socketDescriptor)
{
    QByteArray jsonData = str.toUtf8();
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out << (quint32)jsonData.size();
    data.append(jsonData);

    for(int i = 0; i < sockets.size(); i++)
    {
        if(sockets[i]->socketDescriptor() == socketDescriptor)
        {
            sockets[i]->write(data);
        }
    }

}

void MyTcpServer::slotClientDisconnected()
{
    qDebug()<<"client diconnected";
    Socket->close();
}
