//#include <QCoreApplication>
//#include "mytcpserver.h"



//int main(int argc, char *argv[])
//{
//    QCoreApplication a(argc, argv);
//    // Создаем сервер



//        //принимаем запрос на данные с конкретным временем
//        //генерим данные с таким временем
//        //отсылаем

//    MyTcpServer server;




//    return a.exec();
//}

#include <QCoreApplication>
#include <QDebug>
#include "httpserver.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    HttpServer server;


    // Запуск сервера
    server.startServer();

    return a.exec();
}

