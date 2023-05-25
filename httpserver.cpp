#include "httpserver.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrl>
#include <QtSql>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

HttpServer::HttpServer(QObject *parent) : QObject(parent), tcpServer(nullptr){}

HttpServer::~HttpServer()
{
    stopServer();
}
void HttpServer::startServer()
{
    if (!tcpServer)
    {
        tcpServer = new QTcpServer(this);
        connect(tcpServer, &QTcpServer::newConnection, this, &HttpServer::handleConnection);
//привзяка маршрутов API к обработчикам
        Endpoints.insert("GET /api/positions", std::bind(&HttpServer::getPositionHandle, this, std::placeholders::_1));
        Endpoints.insert("GET /api/token", std::bind(&HttpServer::getTokenHandle, this, std::placeholders::_1));
        Endpoints.insert("GET /api/plan", std::bind(&HttpServer::getPlanHandle, this, std::placeholders::_1));
        Endpoints.insert("GET /api/buildings", std::bind(&HttpServer::getBuildingsHandle, this, std::placeholders::_1));
        Endpoints.insert("GET /api/arealist", std::bind(&HttpServer::getAreaListHandle, this, std::placeholders::_1));

        // Создаем подключение к базе данных PostgreSQL
        db = QSqlDatabase::addDatabase("QPSQL");
        db.setHostName("localhost");
        db.setPort(1111);
        db.setDatabaseName("Coords");
        db.setUserName("generator");
        db.setPassword("1234");
    }

    if (!tcpServer->isListening())
    {
        if (tcpServer->listen(QHostAddress::Any, 8080))
        {
            qDebug() << "Server started on port 8080";
        }
        else
        {
            qDebug() << "Failed to start server:" << tcpServer->errorString();
        }
    }
}

bool HttpServer::isTokenValid(const QString& token)
{
    QString selectQuery = "SELECT last_active FROM Users WHERE token = :token";

    // Создание объекта запроса и привязка значения
    QSqlQuery query;
    query.prepare(selectQuery);
    query.bindValue(":token", token);

    // Выполнение запроса
    if (query.exec() && query.next())
    {
        QDateTime lastActive = query.value(0).toDateTime();
        QDateTime currentDateTime = QDateTime::currentDateTime();
        int difference = lastActive.secsTo(currentDateTime);

        if (difference/3600 > activityInterval)
        {
            return false;
        }
        else return true;
    }
    return false;
}

void HttpServer::stopServer()
{
    if (tcpServer && tcpServer->isListening())
    {
        tcpServer->close();
        qDebug() << "Server stopped";
    }
}

//проверяем наличие sql инъекций в логине или пароле
bool HttpServer::hasSqlInjection(const QString& input)
{
    // Список ключевых слов SQL
    QStringList sqlKeywords = {
        "SELECT", "INSERT", "UPDATE", "DELETE",
        "DROP", "CREATE", "ALTER", "TRUNCATE"
        // Добавьте другие ключевые слова SQL по необходимости
    };

    // Проверка наличия ключевых слов SQL во входной строке
    foreach (const QString& keyword, sqlKeywords) {
        if (input.contains(keyword, Qt::CaseInsensitive)) {
            return true; // Найдено ключевое слово SQL
        }
    }

    // Проверка наличия символов, используемых для SQL инъекций
    QStringList sqlSpecialChars = {"'", "\"", ";", "--", "/*", "*/"};
    foreach (const QString& specialChar, sqlSpecialChars) {
        if (input.contains(specialChar)) {
            return true; // Найден символ SQL инъекции
        }
    }

    // Если не найдены ключевые слова SQL или символы инъекции
    return false;
}

QString HttpServer::getAreaList(const QString &token, const int& buildingId)
{
    QSqlQuery query;
    int groupId = getGroupIdByToken(token);
    query.prepare("SELECT Areas.id, Areas.description, Areas.BuildingId "
                  "FROM Areas "
                  "JOIN GroupAccesAreas ON Areas.id = GroupAccesAreas.Area_id "
                  "WHERE GroupAccesAreas.Group_id = :groupId "
                  "AND Areas.BuildingId = :buildingId");
    query.bindValue(":groupId", groupId);
    query.bindValue(":buildingId", buildingId);
    if (!query.exec()) {
        qDebug() << "Failed to execute query:" << query.lastError().text();
        return QString();
    }

    QJsonArray jsonArray;
    while (query.next()) {
        QJsonObject jsonObject;
        jsonObject["id"] = query.value(0).toInt();
        jsonObject["description"] = query.value(1).toString();
        jsonObject["buildingId"] = query.value(2).toInt();
        jsonArray.append(jsonObject);
    }
    UpdateLastActive(token);

    QJsonObject result;
    result["code"] = 200;
    result["areas"] = jsonArray;

    QJsonDocument jsonDoc(result);
    return jsonDoc.toJson();
}

//обработка запроса списка помещений здания
QString HttpServer::getAreaListHandle(const QString &params)
{
    //GET /api/positions?token=&buildingId=1
    try
    {
        QStringList Params = params.split("&");
        if (Params.size() >= 2)
        {
            QString token = Params[0].split("=")[1];
            int buildingId = Params[1].split("=")[1].toInt();
            //проверка токена на наличие sql инъекций
            if(hasSqlInjection(token))
            {
                qDebug() << "Token has sql symbols. Operation denyed";

                return "{\n    \"code\": 401\n}\n";
            }

            if(!isTokenValid(token))
            {
                return "{\n    \"code\": 401\n}\n";
            }

            //запрос к бд
            return getAreaList(token, buildingId);
        }
        else
        {
            return "{\n    \"code\": 404\n}\n";
        }

    }
    catch (const std::exception& e)
    {
        // Обработка исключения типа std::exception
        qDebug() << "Exception caught:" << e.what();
    }
    catch (...)
    {
        // Обработка других исключений
        qDebug() << "Unknown exce`ption caught";
    }
}

QString HttpServer::getPositionHandle(const QString &params)
{
    //GET /api/positions?token=&time=&areaId=1
    try
    {
        QStringList Params = params.split("&");
        if (Params.size() >= 3)
        {
            QString token = Params[0].split("=")[1];
            QString str = Params[1].split("=")[1].replace("%20", " ");


            QDateTime time = QDateTime::fromString(str, "yyyy-MM-dd hh:mm:ss");
            int areaId = Params[2].split("=")[1].toInt();
            //проверка токена на наличие sql инъекций
            if(hasSqlInjection(token))
            {
                qDebug() << "Token has sql symbols. Operation denyed";

                QJsonArray coordsArray;
                //заполняем файл json массивом
                QJsonObject resultObject;
                resultObject.insert("code", 403);
                resultObject.insert("coords", coordsArray);
                QJsonDocument jsonDoc(resultObject);
                //возвращаем в виде строки чтобы потом отправить
                QString result = jsonDoc.toJson(QJsonDocument::Indented);
                return result;
            }

            if(!isTokenValid(token))
            {
                return "{\n    \"code\": 401\n}\n";
            }

            //запрос к бд
            return getLatestCoords(token, areaId, time, 60);
        }
        else
        {
            return "{\n    \"code\": 404\n}\n";
        }

    }
    catch (const std::exception& e)
    {
        // Обработка исключения типа std::exception
        qDebug() << "Exception caught:" << e.what();
    }
    catch (...)
    {
        // Обработка других исключений
        qDebug() << "Unknown exception caught";
    }
}

QString HttpServer::getBuildingsList(const QString &token)
{
    int groupId = getGroupIdByToken(token);

    QSqlQuery query;
    query.prepare("SELECT Buildings.id, Buildings.description, Buildings.Address, Buildings.Latitude, Buildings.Longitude "
                  "FROM Buildings "
                  "JOIN GroupAccesBuildings ON Buildings.id = GroupAccesBuildings.Building_id "
                  "WHERE GroupAccesBuildings.Group_id = :groupId");
    query.bindValue(":groupId", groupId);
    if (!query.exec())
    {
        qDebug() << "Failed to execute query:" << query.lastError().text();
        return "{\n    \"code\": 404\n}\n";
    }

    QJsonArray jsonArray;
    while (query.next())
    {
        QJsonObject jsonObject;
        jsonObject["id"] = query.value(0).toInt();
        jsonObject["description"] = query.value(1).toString();
        jsonObject["address"] = query.value(2).toString();
        jsonObject["latitude"] = query.value(3).toDouble();
        jsonObject["longitude"] = query.value(4).toDouble();
        jsonArray.append(jsonObject);
    }
    UpdateLastActive(token);
    QJsonObject result;
    result["code"] = 200;
    result["buildings"] = jsonArray;

    QJsonDocument jsonDoc(result);
    return jsonDoc.toJson();
}

//обработка запроса списка зданий
QString HttpServer::getBuildingsHandle(const QString &params)
{
    //GET /api/positions?token=hbjkl
    try
    {
        if (params != "")
        {
            QString token = params.split("=")[1];
            //проверка токена на наличие sql инъекций
            if(hasSqlInjection(token))
            {
                return "{\n    \"code\": 403\n}\n";
            }

            if(!isTokenValid(token))
            {
                return "{\n    \"code\": 401\n}\n";
            }

            //запрос к бд
            return getBuildingsList(token);
        }
        else
        {
            return "{\n    \"code\": 404\n}\n";
        }

    }
    catch (const std::exception& e)
    {
        // Обработка исключения типа std::exception
        qDebug() << "Exception caught:" << e.what();
    }
    catch (...)
    {
        // Обработка других исключений
        qDebug() << "Unknown exce`ption caught";
    }
}

QString HttpServer::getPlan(const QString &token, const int &areaId)
{
    int groupId = getGroupIdByToken(token);
    if (!checkGroupAccessToArea(groupId, areaId))
    {
        //если нет доступа возвращаем с кодом нет доступа
        return "{\n    \"code\": 403\n}\n";
    }

    // Запрос для получения самого свежего плана помещения для заданного areaId
    QString selectQuery = "SELECT Image FROM Plans "
                          "WHERE AreaId = :areaId "
                          "ORDER BY Date_end DESC "
                          "LIMIT 1";

    // Создание объекта запроса
    QSqlQuery query;
    query.prepare(selectQuery);
    query.bindValue(":areaId", areaId);

    // Выполнение запроса
    if (query.exec() && query.next())
    {
        QByteArray imageData = query.value(0).toByteArray();
        QJsonDocument jsonDoc;
        QJsonObject jsonObj;
        jsonObj["image"] = QString::fromLatin1(imageData.toBase64());
        jsonObj["code"] = 200;
        jsonDoc.setObject(jsonObj);
        UpdateLastActive(token);
        return jsonDoc.toJson();
    }
    else
    {
        qDebug() << "Failed to retrieve the plan image from the database.";
        return "{\n    \"code\": 404\n}\n";
    }
}

//обработка запроса плана
QString HttpServer::getPlanHandle(const QString &params)
{
    //GET /api/positions?token=&areaId=1
    try
    {
        QStringList Params = params.split("&");
        if (Params.size() >= 2)
        {
            QString token = Params[0].split("=")[1];
            int areaId = Params[1].split("=")[1].toInt();
            //проверка токена на наличие sql инъекций
            if(hasSqlInjection(token))
            {
                qDebug() << "Token has sql symbols. Operation denyed";

                QJsonArray coordsArray;
                //заполняем файл json массивом
                QJsonObject resultObject;
                resultObject.insert("code", 403);
                resultObject.insert("coords", coordsArray);
                QJsonDocument jsonDoc(resultObject);
                //возвращаем в виде строки чтобы потом отправить
                QString result = jsonDoc.toJson(QJsonDocument::Indented);
                return result;
            }

            if(!isTokenValid(token))
            {
                return "{\n    \"code\": 401\n}\n";
            }

            //запрос к бд
            return getPlan(token, areaId);
        }
        else
        {
            return "{\n    \"code\": 404\n}\n";
        }

    }
    catch (const std::exception& e)
    {
        // Обработка исключения типа std::exception
        qDebug() << "Exception caught:" << e.what();
    }
    catch (...)
    {
        // Обработка других исключений
        qDebug() << "Unknown exce`ption caught";
    }
}

QString HttpServer::generateUserToken()
{
    const QString characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const int tokenLength = 20;
    QString token;

    // Генерация токена
    for (int i = 0; i < tokenLength; ++i)
    {
        int randomIndex = rand() % characters.length();
        QChar randomChar = characters.at(randomIndex);
        token.append(randomChar);
    }

    // Проверка токена на наличие символов SQL
    if (hasSqlInjection(token)) {
        // Если токен содержит символы SQL, генерируем новый токен
        return generateUserToken();
    }

    return token;
}

QString HttpServer::getTokenHandle(const QString &params)
{
    //GET /api/positions?token=&time=&areaId=1
    try
    {
        QStringList Params = params.split("&");
        if(Params.size() >= 2)
        {
            QString username = Params[0].split("=")[1];
            QString password = Params[1].split("=")[1];

            //проверка токена на наличие sql инъекций
            if(hasSqlInjection(username) || hasSqlInjection(password))
            {
                qDebug() << "Detected sql symbols in params value. Operation denyed";
                return "";
            }

            //проверка совпадения в базе
            // Формирование запроса на проверку логина и пароля
            QString queryStr = "SELECT COUNT(*) FROM Users WHERE username = :username AND password = :password";
            QSqlQuery query;
            query.prepare(queryStr);
            query.bindValue(":username", username);
            query.bindValue(":password", password);

            // Выполнение запроса и получение результата
            if (!query.exec()) {
                qWarning() << "Не удалось выполнить запрос";
                return "";
            }

            // Получение результата проверки
            if (query.next()) {
                int count = query.value(0).toInt();
                if (count > 0) // Возвращает true, если есть соответствующая запись, иначе false
                {
                    //сгенерировали токен
                    QString token = generateUserToken();

                    //записываем в базу токен и дату авторизации + отправляем токен клиенту

                    QSqlQuery query;
                    query.prepare("UPDATE Users SET token = :token, last_active = :last_active WHERE username = :username AND password = :password");
                    query.bindValue(":token", token);
                    query.bindValue(":last_active", QDateTime::currentDateTime());
                    query.bindValue(":username", username);
                    query.bindValue(":password", password);

                    if (query.exec())
                    {
                        // Запрос выполнен успешно
                        qDebug() << "Token and last_active updated for user:" << username;

                        //заполняем файл json массивом
                        QJsonObject resultObject;
                        resultObject.insert("code", 200);
                        resultObject.insert("token", token);

                        QJsonDocument jsonDoc(resultObject);
                        //возвращаем в виде строки чтобы потом отправить
                        QString result = jsonDoc.toJson(QJsonDocument::Indented);
                        return result;

                        //return "{\n    \"code\": 200,\n    \"token\": " + token + "\n}\n";
                    }
                    else
                    {
                        // Обработка ошибки выполнения запроса
                        qDebug() << "Failed to update token and last_active for user:" << username;
                        qDebug() << "Error:" << query.lastError().text();
                        return "{\n    \"code\": 401\n}\n";
                    }


                }
            }
        }

        return "{\n    \"code\": 401\n}\n";
        //генерация токена
        //запись токена в базу
        //отправка токена обратно


    }
    catch (const std::exception& e)
    {
        // Обработка исключения типа std::exception
        qDebug() << "Exception caught:" << e.what();
    }
    catch (...)
    {
        // Обработка других исключений
        qDebug() << "Unknown exception caught";
    }
}

QByteArray HttpServer::createHttpResponse(const QString& content)
{
    qDebug() << content.size();
    QByteArray response;

    // Формирование заголовков
    QString headers = QString("HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: %1\r\n"
                              "\r\n").arg(content.size());

    qDebug() << headers;
    response.append(headers.toUtf8());
    response.append(content.toUtf8());

    return response;
}

void HttpServer::handleMessage()
{
    QString responseStr = "";
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (clientSocket)
    {
        QByteArray requestData = clientSocket->readAll();


        // Открываем подключение к базе данных
        if (!db.open())
        {
            qDebug() << "Failed to connect to the database" << db.lastError().text();
            return;
            //отправка сообщения об ошибке подключения
        }
        else
        {
            qDebug() << "Succesfully connected!";
        }


        // Обработка входящего запроса
        //отбрасываем заголовки http и оставляем только api запрос и параметры
        QString req = extractApiRequest(requestData);

        qDebug() << "Received request:" << req;

        //делим запрос на путь и параметры
        QStringList list = req.split('?');

        // Получаем указатель на функцию из QMap по соответствующей строке
        RequestHandler handler = Endpoints.value(list[0]);

        // Вызываем функцию с передачей необходимых параметров
        if(handler != NULL)
        {
            responseStr = handler(list[1]);
        }

        qDebug() << responseStr;
        db.close();

        // Создание ответа если вообще нечего ответить
        if(responseStr == "")
        {
            responseStr = "{\n    \"code\": 404\n}\n";
        }

        QByteArray response = createHttpResponse(responseStr);
        // Отправка ответа
        if (clientSocket)
        {
            clientSocket->write(response);
            clientSocket->waitForBytesWritten();
            clientSocket->close();
        }

        //emit requestReceived(requestData);
    }
}

void HttpServer::handleConnection()
{
    while (tcpServer->hasPendingConnections())
    {
        QTcpSocket* clientSocket = tcpServer->nextPendingConnection();
        connect(clientSocket, &QTcpSocket::disconnected, this, &HttpServer::handleDisconnection);
        connect(clientSocket, &QTcpSocket::readyRead, this, &HttpServer::handleMessage);

        clientConnections.append(clientSocket);
        qDebug() << "Client connected " << clientSocket->socketDescriptor();
    }
}

void HttpServer::handleDisconnection()
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (clientSocket)
    {
        clientConnections.removeAll(clientSocket);
        clientSocket->deleteLater();
        qDebug() << "Client disconnected";
    }
}

QString HttpServer::extractApiRequest(const QByteArray& requestData)
{
    // Находим позицию начала API-запроса
    int requestStart = requestData.indexOf("GET /api/");

    // Если не найдено начало API-запроса, возвращаем пустую строку
    if (requestStart == -1)
        return "";

    // Находим позицию окончания API-запроса
    int requestEnd = requestData.indexOf(" HTTP/1.1\r\n", requestStart);

    // Если не найдено окончание API-запроса, возвращаем пустую строку
    if (requestEnd == -1)
        return "";

    // Получаем часть строки, содержащую только API-запрос и его параметры
    QByteArray apiRequest = requestData.mid(requestStart, requestEnd - requestStart);

    // Преобразуем в QString и возвращаем результат
    return QString::fromUtf8(apiRequest);
}
//вытаскиваем id группы
int HttpServer::getGroupIdByToken(const QString& token)
{
    int groupId = -1;

    QSqlQuery tokenQuery(db);
    tokenQuery.prepare("SELECT Group_id FROM Users WHERE token = :token");
    tokenQuery.bindValue(":token", token);

    if (tokenQuery.exec() && tokenQuery.next())
    {
        groupId = tokenQuery.value(0).toInt();
    }
    else
    {
        qDebug() << "Failed to get group ID for token:" << token;
    }

    return groupId;
}
//проверка доступа группе к ареа
bool HttpServer::checkGroupAccessToArea(int groupId, int areaId)
{
    bool hasAccess = false;

    QSqlQuery accessQuery(db);
    accessQuery.prepare("SELECT COUNT(*) FROM GroupAccesAreas WHERE Group_id = :groupId AND Area_id = :areaId");
    accessQuery.bindValue(":groupId", groupId);
    accessQuery.bindValue(":areaId", areaId);

    if (accessQuery.exec() && accessQuery.next())
    {
        int count = accessQuery.value(0).toInt();
        hasAccess = (count > 0);
    }
    else
    {
        qDebug() << "Failed to check group access to area. Group ID:" << groupId << "Area ID:" << areaId;
    }

    return hasAccess;
}

void HttpServer::UpdateLastActive(const QString &token)
{
    // Запрос для обновления даты и времени в записи таблицы "Users" у пользователя с заданным токеном
    QString updateQuery = "UPDATE Users SET last_active = :datetime WHERE token = :token";

    QSqlQuery query(db);
    // Создание объекта запроса и привязка значений
    query.prepare(updateQuery);
    query.bindValue(":datetime", QDateTime::currentDateTime());  // Текущая дата и время
    query.bindValue(":token", token);  // Заданный токен пользователя

    // Выполнение запроса
    if (query.exec())
    {
        qDebug() << "User last_active updated successfully.";
    }
    else
    {
        qDebug() << "Failed to update user last_active:" << query.lastError().text();
    }


}

QString HttpServer::getLatestCoords(const QString &token, const int &areaId, QDateTime time, int interval = 60)//интервал - отрезок от начала промежутка врмени
                                                                                          //за который получаем координаты до конца
{
    QSqlDatabase db = QSqlDatabase::database(); // получаем активное подключение к базе данных

    int groupId = getGroupIdByToken(token);
    if (!checkGroupAccessToArea(groupId, areaId))
    {
        //если нет доступа возвращаем с кодом нет доступа
        return "{\n    \"code\": 403\n}\n";
    }

    QString queryStr = "SELECT DISTINCT ON (Devices.mac_address) "
            "Devices.mac_address, "
            "Devices.name, "
            "DevicesInAreas.X, "
            "DevicesInAreas.Y, "
            "DevicesInAreas.DateTime "
            "FROM Devices "
            "JOIN DevicesInAreas ON Devices.id = DevicesInAreas.DeviceId "
            "JOIN GroupAccesDevices gad ON Devices.id = gad.Device_id "
            "WHERE DevicesInAreas.AreaId = :areaId "
            "AND DevicesInAreas.DateTime > :timeFrom "
            "AND DevicesInAreas.DateTime < :timeTo "
            "AND gad.Group_id = :groupId "
            "ORDER BY Devices.mac_address, DevicesInAreas.DateTime DESC";
    QSqlQuery query(db);
    query.prepare(queryStr);
    query.bindValue(":groupId", groupId);
    query.bindValue(":areaId", areaId);
    query.bindValue(":timeFrom", time.addSecs(-interval));
    query.bindValue(":timeTo", time);

    if(!query.exec())
    {
        qDebug() << "Failed to connect to the database" << db.lastError().text();;
    }

    //пакуем каждый объект в элемент массива json
    QJsonArray coordsArray;
    while (query.next())
    {
        QJsonObject coordObject;
        coordObject.insert("mac", query.value(0).toString());
        coordObject.insert("name", query.value(1).toString());
        coordObject.insert("x", query.value(2).toDouble());
        coordObject.insert("y", query.value(3).toDouble());
        coordObject.insert("dateTime", query.value(4).toDateTime().toString(Qt::ISODate));
        qDebug() << query.value(4).toDateTime().toString(Qt::ISODate);
        coordsArray.append(coordObject);
    }

    //заполняем файл json массивом
    QJsonObject resultObject;
    resultObject.insert("code", 200);
    resultObject.insert("coords", coordsArray);

    QJsonDocument jsonDoc(resultObject);
    //возвращаем в виде строки чтобы потом отправить
    QString result = jsonDoc.toJson(QJsonDocument::Indented);

    UpdateLastActive(token);
    return result;
}

