#include "datagenerator.h"
#include <iomanip>
#include <random>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

//режим REALTIME

//вход - мак адреса, выход - <Мак + координата>, делаем updateCoord у объектов у которых совпал мак
//если мака нет в списке, то есть он пришел впервые, то генерация(запрос) имени и добавление в список

void DataGenerator::GenerateCoordinate(QDateTime time)
{
    if(coords.size() == 0)
    {
        CurrentCoord *c = new CurrentCoord(GenerateMacAddress(), GenerateName(), ((-50 + 228) + rand()% 100),
                                           ((-50 + 228) + rand()% 100), time);
        //имитация что появилось новое устройство
        coords.append(*c);
        return;
    }

    //для каждого мака придумываем координату
    for(int i = 0; i < coords.size(); i++)
    {
        coords[i].x = -20 + coords[i].x + rand() % 40;
        coords[i].y = -20 + coords[i].y + rand() % 40;
        coords[i].dateTime = time;
    }

    int v = 0 + rand() % 100;//вероятность появления нового устройства в сети
    if (v >= 95)
    {
        CurrentCoord *c = new CurrentCoord(GenerateMacAddress(), GenerateName(), ((-50 + 228) + rand()% 100),
                                           ((-50 + 228) + rand()% 100), time);
        //имитация что появилось новое устройство
        coords.append(*c);
    }

    else if (v <= 5)
    {
        if(coords.size() > 4)
        {
            //удаляем одно старое, как будто оно исчезло из помещения
            int a = 0 + rand() % coords.size();
            coords.removeAt(a);
        }
    }
}

QString DataGenerator::createJsonData()
{
    QJsonArray jsonArray;
    for (const auto& coord : coords)
    {
        QJsonObject obj
        {
            {"Mac", coord.Mac},
            {"Name", coord.Name},
            {"x", coord.x},
            {"y", coord.y},
            {"dateTime", coord.dateTime.toString()}
        };
        jsonArray.append(obj);
    }
    QJsonObject jsonObj
    {
        {"coords", jsonArray}
    };
    QJsonDocument jsonDoc(jsonObj);
    return jsonDoc.toJson(QJsonDocument::Indented);
}


QString DataGenerator::GenerateMacAddress()
{
    // Создаем генератор случайных чисел
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);

        // Генерируем 6 байт MAC-адреса
        std::string mac_address;
        //собираем в строку
        for (int i = 0; i < 6; i++)
        {
            int byte = dis(gen);
            std::stringstream ss;
            ss << std::hex << std::setw(2) << std::setfill('0') << byte;
            mac_address += ss.str();
            if (i != 5) mac_address += ":";
        }

        return QString::fromStdString(mac_address);
}

QString DataGenerator::GenerateName()
{
    // Создаем массив возможных имен
    std::vector<std::string> names = {"Александр", "Алексей", "Анатолий", "Андрей", "Антон", "Аркадий", "Вадим",
                                      "Валентин", "Валерий", "Василий", "Виталий", "Владислав", "Григорий",
                                      "Даниил", "Евгений", "Егор", "Игорь", "Кирилл", "Леонид",
                                      "Михаил", "Никита", "Николай", "Роман", "Руслан",
                                      "Станислав", "Степан", "Фёдор", "Юрий"};

    // Создаем генератор случайных чисел
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, names.size() - 1);

    // Генерируем случайное имя из массива
    return QString::fromStdString(names[dis(gen)]);
}
