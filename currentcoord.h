#ifndef CURRENTCOORD_H
#define CURRENTCOORD_H

#include <QString>
#include <QDateTime>

class CurrentCoord
{
public:
    CurrentCoord(QString Mac, QString Name, int x,int y, QDateTime dateTime);

    QString Name;
    QString Mac;

    int x;
    int y;

    QDateTime dateTime;
};

#endif // CURRENTCOORD_H
