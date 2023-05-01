#ifndef DATAGENERATOR_H
#define DATAGENERATOR_H

#include <QString>
#include <QVector>
#include <QDateTime>
#include <currentcoord.h>
//#include "coordinate.h"
//#include <device.h>

//время, интервал времени, макс количество точек
//запрос картинки
//отбор устройств в помещении и доступных мне
//сохранение отредактированных файлов

//поддержка множества клиентов
//авторизация сервер выдает идентификатор, после долгого периода неактивности он его забывает, при отключении клиента тоже забывается
//надо как-то хранить токены
//токены идут от сервера после верного логина и пароля, + от клиента в последующих запросах, токен позволяет фильтровать данные которые идут в ответ
//привязка координаты к id плана

//расписать процесс взаимодействия по уровням

//база данных



class DataGenerator
{
public:
    //DataGenerator();
    QVector <CurrentCoord> coords;//mac, name, x, y, date_time
    QString GenerateMacAddress();
    QString GenerateName();
    QString createJsonData();
    void GenerateCoordinate(QDateTime time = QDateTime::currentDateTime());
};

#endif // DATAGENERATOR_H
