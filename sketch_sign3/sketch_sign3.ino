// Данный код предназначен для сигнализации мотоцикла на основе платформы Arduino-nano
// При подаче питания: в первый момент времени ардуино ожидает инициализации датчика наклона,
// затем ожидает 30 секунд инициализации GSM-модуля, иначе необходим принудительный RESET.
// После инициализации переходит к основному циклу и ожидает входящего звонка с определенного номера.
// При входящем звонке с нужного номера запоминает начальный угол наклона и переходит в рабочий режим.
// С некоторым интервалом времени, до момента повторного входящего с того же номера, сравнивает начальный угол наклона с текущим.
// В случае отклонения от начального угла производит исходящий вызов на заданный номер.



// нужно реализовать:
// программный ресет GSM и ARDUINO в случае тайм-аута
// переход в спящий режим GSM после готовности модуля
// переход в спящий режим ARDUINO после перехода в спящий режим GSM модуля 
// выход ARDUINO из спящего режима по событию из модуля GSM (ЗАПУСК СИГНАЛИЗАЦИИ)
// уход в спящий режим ARDUINO по событию из модуля GSM (ОТКЛЮЧЕНИЕ СИГНАЛИЗАЦИИ)
// отправку sms с GPS-координатами/напряжением/балансом при получении sms с нужной командой

#include <SoftwareSerial.h>                     // Библиотека для работы с портами
#include <TinyGPS.h>                            // Библиотека для работы с GPS модулем
#include <I2Cdev.h>                             // Библиотека для работы с I2C устройствами
#include <MPU6050.h>                            // Библиотека для работы с MPU6050

#define TO_DEG 57.29577951308232087679815481410517033f  // Кооэффициент для расчета
#define T_OUT 1000                              // Задержка 1 сек

SoftwareSerial Two_Serial(5, 4);                // Выводы SIM800L Tx Rx подключены к выводам Arduino 5 и 4
TinyGPS gps;                                    // Объект для GPS
MPU6050 accel;                                  // Объект для MPU6050
float angle_ax0;                                // Переменная для начального угла
float angle_ax;                                 // Переменная для угла поворота
long int t_next;                                // Переменная для расчета времени
int play = 0;                                   // Начальный индекс состояния сигнализации - "Неактивна"
long lat = 0, lon;                              // Переменные для широты и долготы модуля GPS
bool newdata = false;                           // Начальное значение функции проверки наличия данных
unsigned long start;                            // Время старта
unsigned long time, date;                       // Переменные для времени и даты модуля GPS
String List_Phones = "+79*********";            // Белый список телефонов
String resp = "";
int timeout_gsm = 30;                           // time_out инициализации GSM

float clamp(float v, float minv, float maxv)
{
  if( v>maxv )
   return maxv;
  else if( v<minv )
   return minv;
  return v;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup()                                    // Начальные установки
{
  Serial.begin(9600);                           // Установка скорости обмена Arduino и компьютера
  Two_Serial.begin(9600);                       // Установка скорости обмена Arduino и модулей GPS,GSM
  
  pinMode(13, OUTPUT);                          // Инициализация свеодиода на Arduino
                                                // Конфигурируем выводы для мультиплексора
  pinMode(10, OUTPUT);                          // 10 вывод Arduino = A - вывод мультиплексора
  pinMode(11, OUTPUT);                          // 11 вывод Arduino = B - вывод мультиплексора
  pinMode(12, OUTPUT);                          // 12 вывод Arduino = С - вывод мультиплексора
  digitalWrite(10, LOW);                        // A - "0"
  digitalWrite(11, LOW);                        // B - "0"
  digitalWrite(12, LOW);                        // C - "0"
  
  accel.initialize();                           // Инициализация MPU6050
  delay(1000);                                  // Пауза 1 секунда
  Serial.println(accel.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");
  while (accel.testConnection() != 1)           // Цикл пока нет подключения к MPU6050
    {
      Serial.println("No connect");             // Сообщение в монитор порта
      delay(1000);                              // Пауза 1 секунда
    }

  Serial.println("Инициализация GSM...");       // Печать текста в Serial Monitor
  while(1)
  {
    Two_Serial.println("AT+COPS?");             // Поиск сети
    if(Two_Serial.find("+COPS: 0"))             // Если сеть найдена
    {
      digitalWrite(13, HIGH);
      delay(2000);
      digitalWrite(13, LOW);
      delay(2000);
      Serial.println("Инициализация GSM - ОК");
      break;
    }
    if(timeout_gsm == 0)                        // Если время поиска сети вышло
    {
      while(1)
      {
        digitalWrite(13, HIGH);
        delay(1000);
        Serial.println("Инициализация GSM - ERROR");
        Serial.println("Выполните - RESET");    // Сообщение в монитор порта

        //////////////// Здесь должен быть Reset GSM-модуля и Arduino///////////////
      }
    }
    delay(1000);
    timeout_gsm -= 1;
  }
  delay(1000);                                  // Пауза 1 с

  Two_Serial.println("AT");                     // Запрос готовности модуля к работе
  updateSerial();
  Two_Serial.println("AT+IPR=9600");            // Установка скорости модуля
  updateSerial();
  Two_Serial.println("AT+CSCB=1");              // Запрет приема специальных сообщений
  updateSerial();
  Two_Serial.println("AT+CPAS");                // Запрос состояния модуля /0-готов/2-неизвестно/3-входящий/4-соединение
  updateSerial();
  Two_Serial.println("AT+CLIP=1");              // Вкл АОН
  updateSerial();
  Two_Serial.println("AT+CCALR?");              // Готовность совершать звонки /0-не готов/1-готов
  updateSerial();
  Two_Serial.println("AT+CSQ");                 // Запрос уровня сигнала /0-115дБ/1-112дБ/2..30-110..54дБ/31-52дБ/99-нет сигнала
  updateSerial();
  Two_Serial.println("AT+CMGF=1\r");            // Включить TextMode для SMS
  updateSerial();
  Two_Serial.println("AT+CSCS=\"GSM\"");        // Режим кодировки
  updateSerial();
  //Two_Serial.println("AT+CMGDA=\"DEL ALL\""); // Удаление всех сообщений
  //updateSerial();
  Two_Serial.println("AT+CBC");                 // Запрос параметров питания
  updateSerial();                               // 0-не заряжается/1-заряжается/2-зарядка окончена
                                                // 1-100% заряда
                                                // Напряжение питания (мВ)
  Two_Serial.println("AT&W");                   // Сохранение настроек текущего профиля

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  Two_Serial.println("AT+CMGL=\"ALL\",1");      // Список всех сообщений SMS
  //Two_Serial.println("AT+CMGR=1");            // Прочесть первое сообщение
  
  if(Two_Serial.find("New test"))               // Если в ответе модуля присутствует данный текст
  {
    Serial.println("Новый тест");               // Сообщение в монитор порта
  }
  if(Two_Serial.find("Gps"))
  {
    Serial.println("Координаты GPS");
  }
  if(Two_Serial.find("No"))
  {
    Serial.println("Нет");
  }
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  
  updateSerial();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop()                                                             // Основной цикл
{
  while(play == 0)                                                      // Если неактивна, то ждем входящего вызова
  {
    delay(2000);
    if(Two_Serial.find("RING"))                                         // Если входящий вызов
    {
      ring_number();                                                    // Функция определяет номер телефона
      
                                                                        // Проверяем, чтобы длина номера была больше 6 цифр, 
                                                                        // и номер должен быть в списке
      if (innerPhone.length() >= 7 && List_Phones.indexOf(innerPhone) >= 0)
      {
        Serial.println("Белый номер");                                  // Если номер в списке
        Serial.println("ЗАПУСК СИГНАЛИЗАЦИИ");                          // Сообщение в монитор
        indicator();                                                    // Мигание светодиодом
        Two_Serial.println("ATH");                                      // Положить трубку
        Serial.println("Сбросил");                                      // Сообщение в монитор
      
        int16_t ax_raw, ay_raw, az_raw, gx_raw, gy_raw, gz_raw;
        float ay,gx;
        accel.getMotion6(&ax_raw, &ay_raw, &az_raw, &gx_raw, &gy_raw, &gz_raw); // Получаем данные из акселерометра
        //ay = ay_raw / 4096.0;                                         // Если нужна бОльшая чувствительность
        ay = ay_raw / 16384.0;                                          // Для реального изменения угла
        ay = clamp(ay, -1.0, 1.0);
      
        if( ay >= 0)                                                    // Если угол положительный
        {
          angle_ax0 = 90 - TO_DEG*acos(ay);                             // Переводим угол в градусы
        }
        else
        {
          angle_ax0 = TO_DEG*acos(-ay) - 90;                            // Переводим угол в градусы
        }
        
        Serial.println(String("Начальный угол: ") + String(angle_ax0)); // Сообщение в монитор с начальным положением
    
        play = 1;                                                       // Индекс состояния - "Активна"
        digitalWrite(13, HIGH);                                         // Светодиод включаем
        delay(5000);
      }
      else
      {
        Serial.println("Черный номер");                                 // Если номера нет в списке
        Two_Serial.println("ATH");                                      // Положить трубку
        Serial.println("Сбросил");                                      // Сообщение в монитор
      }
    }
  }

  while(play == 1)                                                      // Если активна, то ждем входящего вызова
  {
    delay(2000);
    if(Two_Serial.find("RING"))                                         // Если входящий вызов
    {
      ring_number();                                                    // Функция определяет номер телефона
      
                                                                        // Проверяем, чтобы длина номера была больше 6 цифр, 
                                                                        // и номер должен быть в списке
      if (innerPhone.length() >= 7 && List_Phones.indexOf(innerPhone) >= 0)
      {
        Serial.println("Белый номер");                                  // Если номер в списке
        Serial.println("ОТКЛЮЧЕНИЕ СИГНАЛИЗАЦИИ");                      // Сообщение в монитор
        indicator();                                                    // Мигание светодиодом
        Two_Serial.println("ATH");                                      // Положить трубку
        Serial.println("Сбросил");                                      // Сообщение в монитор

        play = 0;                                                       // Индекс состояния - "Неактивна"
        digitalWrite(13, LOW);                                          // Погасить светодиод
        delay(5000);                                                    // Пауза 5 секунд
      }
      else
      {
        Serial.println("Черный номер");                                 // Если номера нет в списке
        Two_Serial.println("ATH");                                      // Положить трубку
        Serial.println("Сбросил");                                      // Сообщение в монитор
      }
    }
    
    long int t = millis();
    if( t_next < t )
    {
      int16_t ax_raw, ay_raw, az_raw, gx_raw, gy_raw, gz_raw;
      float ay,gx;
      t_next = t + T_OUT;
      accel.getMotion6(&ax_raw, &ay_raw, &az_raw, &gx_raw, &gy_raw, &gz_raw);
      //ay = ay_raw / 4096.0;
      ay = ay_raw / 16384.0;
      ay = clamp(ay, -1.0, 1.0);
      if( ay >= 0)
      {
        angle_ax = 90 - TO_DEG*acos(ay);
      }
      else
      {
        angle_ax = TO_DEG*acos(-ay) - 90;
      }
      
      Serial.print(" Angle: "); Serial.println(angle_ax);           // вывод в порт угла поворота
      
      if(abs(abs(angle_ax)-abs(angle_ax0))>10)                      // Если угол отличается на более чем 10 градусов
      {
        Serial.println("Изменение угла больше 10 градусов");        // Сообщение в монитор
        Two_Serial.println("ATD+79*********;");                     // Номер телефона для вызова
        updateSerial();                                             // Отслеживаем ответ от GSM модуля
        delay(25000);                                               // Пауза 15 с
        Two_Serial.println("ATH");                                  // Положить трубку
        delay(10000);                                               // Пауза 10 с
      }
    }

    //date_gps();                                                     // Получаем GPS данные
  }
  updateSerial();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void updateSerial()                                                 // Функция отслеживания входящих-исходящих данных
{
  delay(50);                                                        // Пауза 500 мс
  while (Serial.available())
  {
    Two_Serial.write(Serial.read());                                // Переадресация с порта Arduino IDE на порт SIM800L
  }
  while(Two_Serial.available()) 
  {
    Serial.write(Two_Serial.read());                                // Получаем данные с SIM800L и отправляем в монитор порта Arduino
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void go_sms()                                                       // Функция отправки sms
{
  Two_Serial.print("AT+CMGS=\"");                                   // Команда отправки смс
  Two_Serial.print("+79*********");                                 // Номер для отправки
  Two_Serial.println("\"");                                         // Закрытие команды отправки
  updateSerial();                                                   // Ждем ответа
  Two_Serial.println("my message");                                 // Вводим сообщение
  updateSerial();                                                   // Ждем ответа
  Two_Serial.println((char)26);                                     // Уведомляем GSM-модуль об окончании ввода
  updateSerial();                                                   // Ждем ответа
  Two_Serial.println();
  updateSerial();                                                   // Ожидаем отправки
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void indicator()                                                    // Функция мигания светодиодом
{
  for(int i=0; i<5; i++)                                            // Цикл мигания светодиодом
    {
      digitalWrite(13, LOW);
      delay(250);
      digitalWrite(13, HIGH);
      delay(250);
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void date_gps()                                                     // Функция получения данных с GPS
{
  digitalWrite(10, HIGH);                                           // A - "1"...переключаем мультиплексор на GPS модуль
  while(lat == 0)
  {
    if (millis() - start > 1000)                                    // Установка задержки в одну секунду между обновлением данных
    {
      newdata = readgps();
      if (newdata)
      {
        start = millis();
        gps.get_position(&lat, &lon);                               // Получить широту и долготу
        gps.get_datetime(&date, &time);                             // Получить дату и время
      
        Serial.print(" Lat: "); Serial.print(lat);                  // Сообщение в монитор порта
        Serial.print(" Long: "); Serial.print(lon);                 // Сообщение в монитор порта
        Serial.print(" Date: "); Serial.print(date);                // Сообщение в монитор порта
        Serial.print(" Time: "); Serial.println(time);              // Сообщение в монитор порта
      }
    }
  }
  lat = 0;
  digitalWrite(10, LOW);                                            // A - "0"...переключаем мультиплексор на GSM модуль
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool readgps()                                                      // Функция проверки наличия данных
{
  while (Two_Serial.available())
  {
    int b = Two_Serial.read();
    if('\r' != b)                                                   // В библ. TinyGPS есть ошибка: не обрабатываются данные с \r и \n
    {
      if (gps.encode(b))
        return true;
    }
  }
  return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ring_number()                                                  // Функция определяет номер телефона
{
  Serial.println("Входящий вызов");                                 // Сообщение в монитор
  resp = Two_Serial.readString();                                   // Записать строку
  int phoneindex = resp.indexOf("+CLIP: \"");                       // Есть ли информация об определении номера, если да, то phoneindex > -1
  String innerPhone = "";                                           // Переменная для хранения определенного номера
  if (phoneindex >= 0)                                              // Если информация была найдена
  {
    phoneindex += 8;                                                // Парсим строку и получаем номер
    innerPhone = resp.substring(phoneindex, resp.indexOf("\"", phoneindex));
    Serial.println("Number: " + innerPhone);                        // Выводим номер в монитор порта
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
