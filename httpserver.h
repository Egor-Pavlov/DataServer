#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

#include <QSqlDatabase>


typedef QString (*FunctionPointer)();

class HttpServer : public QObject
{
    Q_OBJECT
public:
    explicit HttpServer(QObject *parent = nullptr);
    ~HttpServer();
    using RequestHandler = std::function<QString(const QString&)>;

public slots:
    void startServer();
    void stopServer();

private slots:
    void handleConnection();
    void handleDisconnection();
    void handleMessage();

//    void incomingConnection(qintptr socketDescriptor);
//    void slotReadyRead();
//    void slotClientDisconnected();

signals:
    void requestReceived(const QByteArray& request);

private:
    QTcpServer* tcpServer;
    QList<QTcpSocket*> clientConnections;
    QString extractApiRequest(const QByteArray& requestData);
    QString getPositionHandle(const QString &params);
    bool hasSqlInjection(const QString& input);
    QString getLatestCoords(const QString &token, const int &areaId, QDateTime time, int interval);
    QString getTokenHandle(const QString &params);
    QByteArray createHttpResponse(const QString& content);
    QString generateUserToken();
    int getGroupIdByToken(const QString& token);
    bool checkGroupAccessToArea(int groupId, int areaId);
    bool isTokenValid(const QString& token);
    QString getPlanHandle(const QString &params);
    QString getPlan(const QString &token, const int &areaId);
    void UpdateLastActive(const QString &token);
    QString getBuildingsHandle(const QString &params);
    QString getBuildingsList(const QString &token);
    QString getAreaList(const QString &token, const int& buildingId);
    QString getAreaListHandle(const QString &params);

    int activityInterval = 1; //сколько часов действует токен с момента последней активности

    //вектор строк + указателей на функцию, нужен для вызова обработчика каждого вида запросов (get/token, get/plan и тд)
    //позволяет легко добавить новый обработчик нового запроса
    QMap<QString, RequestHandler> Endpoints;

    QSqlDatabase db;
};

#endif // HTTPSERVER_H
