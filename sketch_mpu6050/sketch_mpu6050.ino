#include <I2Cdev.h>                                       // Библиотека для работы с I2C устройствами
#include <MPU6050.h>                                      // Библиотека для работы с MPU6050
#define TO_DEG 57.29577951308232087679815481410517033f    // Кооэффициент для расчета
#define T_OUT 1000                                        // Задержка 1000 миллисекунд

MPU6050 accel;                                            // Объект для MPU6050

float angle_ax;                                           // Переменная для угла поворота
long int t_next;                                          // Переменная для расчета времени
long result;

float clamp(float v, float minv, float maxv){
  if( v>maxv )
   return maxv;
  else if( v<minv )
   return minv;
  return v;
  }

void setup(){
  Serial.begin(9600);                                     // Установка скорости обмена Arduino с компьютером
  accel.initialize();                                     // Инициализация датчика
  Serial.println(accel.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");
  while (accel.testConnection() == 0)
    {
      result = accel.testConnection();
      Serial.println(result);
      Serial.println("No connect");
      delay(1000);
    }
  }

void loop(){
  long int t = millis();
  if( t_next < t )
  {
    int16_t ax_raw, ay_raw, az_raw, gx_raw, gy_raw, gz_raw;
    float ay,gx;
    
    t_next = t + T_OUT;
    accel.getMotion6(&ax_raw, &ay_raw, &az_raw, &gx_raw, &gy_raw, &gz_raw);
 
    // сырые данные акселерометра нужно преобразовать в единицы гравитации
    // при базовых настройках 1G = 4096
    
    //ay = ay_raw / 4096.0;
    ay = ay_raw / 16384.0;
    
    // на случай, если на акселерометр действуют посторонние силы, которые могут
    // увеличить ускорение датчика свыше 1G, установим границы от -1G до +1G  
    
    ay = clamp(ay, -1.0, 1.0);

    // функция acos возвращает угол в радианах, так что преобразуем
    // его в градусы при помощи коэффициента TO_DEG
    
    if( ay >= 0){
      angle_ax = 90 - TO_DEG*acos(ay);
      }
    else{
      angle_ax = TO_DEG*acos(-ay) - 90;
      }
    Serial.println(angle_ax);                             // вывод в порт угла поворота вокруг оси X
  }
}
