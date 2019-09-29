#include <SoftwareSerial.h>                           // Библиотека для работы с портами
#include <TinyGPS.h>                                  // Библиотека для работы с GPS модулем

SoftwareSerial GPS_Serial(5, 4);                      // Номера пинов, к которым подключен модуль (RX, TX)
TinyGPS gps;                                          // Создать gps объект

long lat, lon;                                        // Переменные для широты и долготы
bool newdata = false;
unsigned long start;
unsigned long time, date;                             // Переменные для времени и даты

void setup()                                          // Начальные установки
{
  pinMode(10, OUTPUT);                                // 10 вывод Arduino = A - вывод мультиплексора
  pinMode(11, OUTPUT);                                // 11 вывод Arduino = B - вывод мультиплексора
  pinMode(12, OUTPUT);                                // 12 вывод Arduino = С - вывод мультиплексора
  digitalWrite(10, HIGH);                             // A - "0"
  digitalWrite(11, LOW);                              // B - "0"
  digitalWrite(12, LOW);                              // C - "0"
  
  Serial.begin(9600);                                 // Установка скорости обмена Arduino с компьютером
  GPS_Serial.begin(9600);                             // Установка скорости обмена Arduino с GPS приемником
  Serial.println(" Инициализация GPS...");
}

void loop()                                           // Основной цикл
{
  if (millis() - start > 1000)                        // Установка задержки в одну секунду между обновлением данных
  {
    newdata = readgps();
    if (newdata)
    {
      start = millis();
      gps.get_position(&lat, &lon);                   // Получить широту и долготу
      gps.get_datetime(&date, &time);                 // Получить дату и время
      Serial.print(" Lat: "); Serial.print(lat);      // Сообщение в монитор порта
      Serial.print(" Long: "); Serial.print(lon);
      Serial.print(" Date: "); Serial.print(date);
      Serial.print(" Time: "); Serial.println(time);
    }
  }
}

bool readgps()                                        // Функция проверки наличия данных
{
  while (GPS_Serial.available())
  {
    int b = GPS_Serial.read();
    if('\r' != b)                                     // В библиотеке TinyGPS имеется ошибка: не обрабатываются данные с \r и \n
    {
      if (gps.encode(b))
        return true;
    }
  }
  return false;
}
