#include <SoftwareSerial.h>                                 // Библиотека для работы с портами

SoftwareSerial GSM_Serial(5, 4);                            // Выводы SIM800L Tx Rx подключены к выводам Arduino 4 и 5
int index = 5;
String List_Phones = "+79*********";                        // Белый список телефонов
String resp = "";
int timeout_gsm = 30; 
  
void setup()                                                // Начальные установки
{
  Serial.begin(9600);                                       // Установка скорости обмена Arduino с компьютером
  GSM_Serial.begin(9600);                                   // Инициализация последовательной связи с Arduino и SIM800L
  pinMode(13, OUTPUT);                                      // 13 вывод Arduino - светодиод
  pinMode(10, OUTPUT);                                      // 10 вывод Arduino = A - вывод мультиплексора
  pinMode(11, OUTPUT);                                      // 11 вывод Arduino = B - вывод мультиплексора
  pinMode(12, OUTPUT);                                      // 12 вывод Arduino = С - вывод мультиплексора
  digitalWrite(10, LOW);                                    // A - "0"
  digitalWrite(11, LOW);                                    // B - "0"
  digitalWrite(12, LOW);                                    // C - "0"
  
  Serial.println("Инициализация GSM...");                   // Печать текста в Serial Monitor
  
  while(1)
  {
    GSM_Serial.println("AT+COPS?");
    if(GSM_Serial.find("+COPS: 0"))
    {
      digitalWrite(13, HIGH);
      delay(2000);
      digitalWrite(13, LOW);
      delay(2000);
      Serial.println("Инициализация GSM - ОК");
      break;
    }
    if(timeout_gsm == 0)
    {
      while(1)
      {
        digitalWrite(13, HIGH);
        delay(1000);
        Serial.println("Инициализация GSM - ERROR");
        Serial.println("Выполните - RESET");
      }
    }
    delay(1000);
    timeout_gsm -= 1;
  }
  delay(1000);                                              // Пауза 1 с
  
  GSM_Serial.println("AT");                                 // Отправка команды AT модулю SIM800L
  updateSerial();
  GSM_Serial.println("AT+IPR=9600");                        // Установка скорости модуля
  updateSerial();
  GSM_Serial.println("AT+CSCB=1");                          // Запрет приема специальных сообщений
  updateSerial();
  GSM_Serial.println("AT+CLIP=1");                          // Вкл АОН
  updateSerial();
  GSM_Serial.println("AT+CPAS");                            // Запрос состояния модуля /0-готов/2-неизвестно/3-входящий/4-соединение
  updateSerial();
  GSM_Serial.println("AT+CSQ");                             // Запрос уровня сигнала /0-115дБ/1-112дБ/2..30-110..54дБ/31-52дБ/99-нет сигнала
  updateSerial();
  GSM_Serial.println("AT+CBC");                             // Запрос параметров питания
  updateSerial();                                           // 0-не заряжается/1-заряжается/2-зарядка окончена
                                                            // 1-100% заряда
                                                            // Напряжение питания (мВ)
                                                
  GSM_Serial.println("AT+CMGF=1");                          // Включить TextMode для SMS
  updateSerial();
  GSM_Serial.println("AT+CSCS=\"GSM\"");                    // Режим кодировки
  updateSerial();
  //GSM_Serial.println("AT&W");                             // Сохранение настроек текущего профиля
  //updateSerial();
  GSM_Serial.println("AT+CMGL=\"ALL\",1");                  // Список всех сообщений SMS
  //updateSerial();
  //GSM_Serial.println("AT+CMGR=1");                        // Прочесть первое сообщение

  if(GSM_Serial.find("New test"))
  {
    Serial.println("Новый тест");
  }
  if(GSM_Serial.find("Gps"))
  {
    Serial.println("Координаты GPS");
  }
  if(GSM_Serial.find("No"))
  {
    Serial.println("Нет");
  }
  
  updateSerial();
  
  //GSM_Serial.print("AT+CMGS=\"");                         // Отправка смс
  //GSM_Serial.print("+79*********");                       // Номер для отправки
  //GSM_Serial.println("\"");                               // Закрытие команды отправки
  //updateSerial();
  //GSM_Serial.println("my message");                       // Вводим сообщение
  //updateSerial();
  //GSM_Serial.println((char)26);                           // Уведомляем GSM-модуль об окончании ввода
  //updateSerial();
  //GSM_Serial.println();
  //updateSerial();                                         // Ожидаем отправки
}
 
void loop()                                                 // Основной цикл
{
  updateSerial();
  //if(GSM_Serial.find("RING"))                             // Если входящий вызов
  //{
  //  Serial.println("Входящий вызов");                     // Сообщение в монитор
  //  resp = GSM_Serial.readString();                       // Сохранить строку
  //  int phoneindex = resp.indexOf("+CLIP: \"");           // Есть ли информация об определении номера, если да, то phoneindex>-1
  //  String innerPhone = "";                               // Переменная для хранения определенного номера
  //  if (phoneindex >= 0)                                  // Если информация была найдена
  //  {
  //    phoneindex += 8;                                    // Парсим строку и ...
  //    innerPhone = resp.substring(phoneindex, resp.indexOf("\"", phoneindex));      // ...получаем номер
  //    Serial.println("Number: " + innerPhone);            // Выводим номер в монитор порта
  //  }
                                                            // Проверяем, чтобы длина номера была больше 6 цифр, 
                                                            // и номер должен быть в списке
  //  if (innerPhone.length() >= 7 && List_Phones.indexOf(innerPhone) >= 0)
  //  {
  //    Serial.println("Белый номер");
  //  }
  //  else
  //  {
  //    Serial.println("Черный номер");                     // Если нет, то 
  //  }
  //  /////////////////////////////////////////////////////////////////////
  //  for(int i=0; i<5; i++)
  //  {
  //    digitalWrite(led, HIGH);
  //    delay(250);
  //    digitalWrite(led, LOW);
  //    delay(250);
  //  }
    
  //  GSM_Serial.println("ATH");                            // Положить трубку
  //  Serial.println("Сбросил");                            // Сообщение в монитор
  //  delay(5000);                                          // Пауза 5 с
  //  GSM_Serial.println("ATD+79*********;");               // Номер телефона для вызова
  //  updateSerial();
  //  delay(15000);
  //  GSM_Serial.println("ATH");                            // Положить трубку
  //}
}
  
void updateSerial()                                         // Функция отслеживания входящих-исходящих данных
{
  delay(50);
  while (Serial.available()) 
  {
    GSM_Serial.write(Serial.read());                        // Переадресация с последовательного порта Arduino IDE на последовательный порт SIM800L
  }
  while(GSM_Serial.available()) 
  {
    Serial.write(GSM_Serial.read());                        // Получаем данные с SIM800L и отправляем в монитор порта Arduino
  }
}
