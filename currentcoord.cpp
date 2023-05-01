#include "currentcoord.h"

CurrentCoord::CurrentCoord(QString Mac, QString Name, int x,int y, QDateTime dateTime)
{
    this->Name = Name;
    this->Mac = Mac;
    this->x = x;
    this->y = y;
    this->dateTime = dateTime;
}
