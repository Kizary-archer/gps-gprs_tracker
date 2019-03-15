#include <SoftwareSerial.h>

SoftwareSerial SIM(2, 3);//RX, TX

#define DEBUG true
#define PHONE +7(969)705-57-85
#define ID 0000001

String latitude, longitude, state, satellite;

void(* resetFunc) (void) = 0;//перезагрузка

void setup()  //настройка SIM808 при первом включении
{
  SIM.begin(19200);
  Serial.begin(115200);
  char *Sett[] = {"AT+CFUN=1", "AT+CGNSPWR=1", "AT+CGNSSEQ=GGA"};
  Serial.println("********SIM808 SETTINGS***********");
  for (byte i = 0 ; i < sizeof(Sett); i ++) {
    commandSIM(Sett[i], 10, false, DEBUG);
  }
  initGPRS();
  if (DEBUG)SIM808info(); //вывод информации о модуле
  Serial.println("******************************");
  Serial.println("Enter command:");

}

void loop()
{
  serialListen();
  getGPS();
  delay(100);
}

void initGPRS()
{
  char *gprsAT[] = {  //массив АТ команд
    "AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"",  //Установка настроек подключения
    "AT+SAPBR=3,1,\"APN\",\"internet.beeline.ru\"",
    "AT+SAPBR=3,1,\"USER\",\"beeline\"",
    "AT+SAPBR=3,1,\"PWD\",\"beeline\"",
    "AT+SAPBR=1,1",  //Устанавливаем GPRS соединение
    "AT+HTTPINIT",  //Инициализация http сервиса
    "AT+HTTPPARA=\"CID\",1"  //Установка CID параметра для http сессии
  };
  for (byte i = 0 ; i < 7; i ++) {
    commandSIM(gprsAT[i], 5000, false, DEBUG);
  }
}

void getGPS()
{
  commandSIM("AT+CGNSPWR=1", 10, false, DEBUG);
  commandSIM("AT+CGNSSEQ=GGA", 10, false, DEBUG);
  commandSIM("AT+CGPSINF=2", 1000, true, DEBUG);
}

void parseGPSdata(String dataSendGPS)
{

  String GPSdata[5]; //  latitude,longitude,state,satellite
  int coma[5] = {2, 4, 6, 7};
  byte CountComa = 0;
  int i = 0, j = 0, L = dataSendGPS.length();
  while (i < L)
  {
    if (dataSendGPS[i] == ',')CountComa++;

    i++;
    if (coma[j] == CountComa) {
      while (dataSendGPS[i] != ',')
      {
        GPSdata[j] += dataSendGPS[i];
        i++;
      }
      j++;
    }
  }
  latitude = GPSdata[0];
  longitude = GPSdata[1];
  state = GPSdata[2];
  satellite = GPSdata[3];
  if (DEBUG)
  {
    Serial.println("-------------");
    Serial.print("latitude: ");
    Serial.println(latitude);
    Serial.print("longitude: ");
    Serial.println(longitude);
    Serial.print("state: ");
    Serial.println(state);
    Serial.print("satellite: ");
    Serial.println(satellite);
    Serial.println("-------------");
  }

}

void commandSIM(String command, int timeout, boolean GetData, boolean debug) //вывод ответа на AT команду
{
  String dataSIM808 = "";
  long int t = millis();
  SIM.println(command);
  while (!SIM.available())//ожидание ответа
    if ((t + 10000) < millis())
    {
      Serial.println("Error connect to SIM808...RESET");
      resetFunc();
    }

  t = millis();
  while ( (t + timeout) > millis()) {
    while (SIM.available()) {
      dataSIM808 += char(SIM.read());
    }
  }
  if (debug) Serial.print(dataSIM808);
  if (GetData)eventSIM808(dataSIM808);
}

void eventSIM808(String dataSIM808)//события с модуля
{
  int i = 0;
  String event = "";
  while (dataSIM808[i] != '+') {
    i++;
  }
  i++;
  while (dataSIM808[i] != '=')
  {
    event += dataSIM808[i];
    i++;
  }
  if (event = "CGPSINF")parseGPSdata(dataSIM808);
}

void serialListen()//отправка команд в ручном режиме
{
  while (Serial.available())
  {
    SIM.write(Serial.read());
    delay(10);
  }
  while (SIM.available())
  {
    Serial.write(SIM.read());
    delay(10);
  }
}

void SIM808info()//вывод информации о настройках
{
  char *ATInfo[] = {"name: ", "ATI","sim: ", "AT+COPS?","functionality mode: ", "AT+CFUN?","GPS power: ", "AT+CGPSPWR?","GPS mode: ", "AT+CGPSRST?","GPS parsed mode: ", "AT+CGNSSEQ?"};
  Serial.println("********SIM808 info***********");
  for (byte i = 0 ; i < sizeof(ATInfo) / 2; i += 2) {
    Serial.print(ATInfo[i]);
    commandSIM(ATInfo[i + 1], 10, false, DEBUG);
  }
}
