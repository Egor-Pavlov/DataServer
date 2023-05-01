#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVector>
#include "datagenerator.h"

class MyTcpServer : public QTcpServer
{
    Q_OBJECT
public:
    MyTcpServer();
    QTcpSocket * Socket;

private:
    DataGenerator Generator;

    QVector <QTcpSocket*> sockets;
    QByteArray Data;
    quint16 nextBlockSize = 0;//размер блока данных

    void SendToClient(QString str, qintptr socketDescriptor);

public slots:
    void incomingConnection(qintptr socketDescriptor);
    void slotReadyRead();
    void slotClientDisconnected();
};

#endif // MYTCPSERVER_H
