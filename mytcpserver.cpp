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
            } else
            {
                qDebug() << "Succesfully connected!";
            }




            SendToClient("", socketDescriptor);
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
