#ifndef MYTCPSERVER_H
#define MYTCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVector>

typedef void (*FunctionPointer)();
class MyTcpServer : public QTcpServer
{
    Q_OBJECT
public:
    MyTcpServer();
    QTcpSocket * Socket;

private:
    //вектор строк + указателей на функцию, нужен для вызова обработчика каждого вида запросов (get/token, get/plan и тд)
    //позволяет легко добавить новый обработчик нового запроса
    QVector<std::pair<QString, FunctionPointer>> Endpoints;
    //подключенные клиенты
    QVector <QTcpSocket*> sockets;
    QByteArray Data;
    quint16 nextBlockSize = 0;//размер блока данных
    QJsonObject getLatestCoords(int roomId, QDateTime time, int interval);
    void SendToClient(QString str, qintptr socketDescriptor);
    bool СheckGetPrefix(QString& str);

public slots:
    void incomingConnection(qintptr socketDescriptor);
    void slotReadyRead();
    void slotClientDisconnected();

};

#endif // MYTCPSERVER_H
