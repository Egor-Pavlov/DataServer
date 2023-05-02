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

QString MyTcpServer::getLatestCoords(int roomId)
{
    QSqlDatabase db = QSqlDatabase::database(); // получаем активное подключение к базе данных

    // запрос для получения самых свежих координат каждого устройства с фильтрацией по room_id
    QString queryStr = "SELECT DISTINCT ON (device_id) "
                       "devices.mac_address, devices.name, coordinates.x, coordinates.y, coordinates.room_id, coordinates.timestamp "
                       "FROM coordinates "
                       "JOIN devices ON coordinates.device_id = devices.id "
                       "WHERE coordinates.room_id = :roomId "
                       "ORDER BY device_id, timestamp DESC;";

    QSqlQuery query(db);
    query.prepare(queryStr);
    query.bindValue(":roomId", roomId);
    query.exec();

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

    QJsonObject resultObject;
    resultObject.insert("coords", coordsArray);

    QJsonDocument jsonDoc(resultObject);
    return jsonDoc.toJson(QJsonDocument::Indented);
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
            input >> size;
            //если блок пришел целиком
            if (Socket->bytesAvailable() < size)
            {
                break;
            }

            QString str;
            input >> str;
            qDebug() << str;
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
                QString result = getLatestCoords(1);
                qDebug() << result;


                //упаковка

                //отправка
                SendToClient(result, socketDescriptor);
                db.close();
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
//    Data.clear();
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
