#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <MsTimer2.h>
#include <avr/wdt.h>

SoftwareSerial SIM(2, 3);//RX, TX

#define SaveisFuel 0
#define SaveisWork 1
#define SaveisPayload 2

//digital pins
#define SIM808_on 4 //вывод модуля из сна
#define Pin_isFuel 5 //наличие топлива
#define Pin_isWork 12 //работа генератора

float latitudeNow = 0, longitudeNow = 0, latitude = 0, longitude = 0;
int countSatellite = 0, countSatelliteNow = 0, state = 0;
String ID = "5c8a814a5841bc45ced73831";
boolean isFuel, isWork, DEBUG = false;

void setup()  //настройка SIM808 при первом включении
{
  SIM.begin(19200);
  Serial.begin(115200);

  pinMode(SIM808_on, OUTPUT);
  pinMode(Pin_isFuel, INPUT);
  pinMode(Pin_isWork, INPUT);

  MsTimer2::set(200, timerInterupt);
  MsTimer2::start();
  wdt_enable(WDTO_4S);

  SIM.println("AT");
  long int t = millis();
  Serial.print("\nWait connect");
  while ( (t + 5000) > millis()) //ожидание включения модуля
  {
    delay(500);
    Serial.print(".");
    if (SIM.available()) break;
  }
  if (!SIM.available()) {
    digitalWrite(SIM808_on, HIGH);
    delay(2000);
    digitalWrite(SIM808_on, LOW);
    MsTimer2::stop();
    delay(4000);
  }
  Serial.println("\nDEBUG (y/n)");
  t = millis();
  while ( (t + 5000) > millis()) //ожидание включения модуля
  {
    if (char(Serial.read()) == 'y') DEBUG = true;
  }
  Serial.println("DEBUG: ");
  Serial.println(DEBUG);
  Serial.println("\n********SIM808 SETTINGS***********");
  char *Sett[] = {
    "AT+GSMBUSY=1",
    "AT+CLIP=0"
  };
  for (byte i = 0 ; i < 2; i ++) commandSIM(Sett[i], 100, false, DEBUG);

  initGPRS();
  initGPS();
  if (DEBUG)SIM808info(); //вывод информации о модуле
  Serial.println("******************************");
  Serial.println("Enter command:");

}
void initGPS()
{
  char *Sett[] = {
    "AT+CGNSPWR=1",
    "AT+CGPSRST=1",
    "AT+CGNSSEQ=GGA"
  };
  for (byte i = 0 ; i < 3; i ++) commandSIM(Sett[i], 100, false, DEBUG);
}
void initGPRS()
{
  char *gprsAT[] = {
    "AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"",  //Установка настроек подключения
    "AT+SAPBR=3,1,\"APN\",\"internet.tele2.ru\"",
    "AT+SAPBR=3,1,\"USER\",\"tele2\"",
    "AT+SAPBR=3,1,\"PWD\",\"tele2\"",
    "AT+SAPBR=1,1",  //Устанавливаем GPRS соединение
    "AT+SAPBR=2,1" //Проверяем как настроилось
  };
  for (byte i = 0 ; i < 6; i ++) {
    commandSIM(gprsAT[i], 2000, false, DEBUG);
  }
}

void SIM808info()//вывод информации о настройках
{
  char *ATInfo[] = {
    "name: ", "ATI",
    "sim: ", "AT+COPS?",
    "functionality mode: ", "AT+CFUN?",
    "GPS power: ", "AT+CGPSPWR?",
    "GPS mode: ", "AT+CGPSRST?",
    "GPS parsed mode: ", "AT+CGNSSEQ?",
    "call mode: ", "AT+GSMBUSY?"
  };
  Serial.println("********SIM808 info***********");
  for (byte i = 0 ; i < 14; i += 2) {
    Serial.print(ATInfo[i]);
    commandSIM(ATInfo[i + 1], 100, false, DEBUG);
  }
}

void loop()
{
  long int t = millis();
  while (1)
  {
    serialListen();
    if ((t + 10000) < millis()) // проверка состояния генератора
    {
      GPSdata();
      checkGeneratorStatus();
      break;
    }
  }
}

void GPSdata()
{
  String dataSendGPS = "";
  String GPSdata[4]; //  latitude,longitude,state,satellite
  int coma[4] = {2, 4, 6, 7};
  byte CountComa = 0;
  int i = 0, j = 0, timeout = 1000;
  SIM.println("AT+CGPSINF=2");
  long int t = millis();
  while (!SIM.available())//ожидание ответа
    if ((t + 5000) < millis())
      if (repeatSend("AT"))break;

  while (SIM.available())
  {
    dataSendGPS += char(SIM.read());
    delay(10);
  }

  t = millis();
  while (i < dataSendGPS.length())
  {
    if ((t + 1000) < millis())break;

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
  //convert
  state = GPSdata[2].toInt();
  if (state)
  {
    if (countSatelliteNow > countSatellite )countSatellite = countSatelliteNow;

    latitudeNow = atof(GPSdata[0].c_str());
    longitudeNow = atof(GPSdata[1].c_str());
    countSatelliteNow = GPSdata[3].toInt();
    
    if (countSatelliteNow > 20)countSatelliteNow /= 10;
  }
  if (DEBUG)
  {
    Serial.println("-------------");
    Serial.print("latitude: ");
    Serial.println(latitudeNow, 4);
    Serial.print("longitude: ");
    Serial.println(longitudeNow, 4);
    Serial.print("state: ");
    Serial.println(state);
    Serial.print("satellite: ");
    Serial.println(countSatelliteNow);
    Serial.println("-------------");
  }
}

void checkGeneratorStatus()
{
  String Send = "";
  float R = 0.0010;
  if (state)
  {
    state = 0;
    if ((pow(latitudeNow - latitude, 2) + pow(longitudeNow - longitude, 2) >= pow(R, 2)) || (countSatelliteNow > countSatellite ))
    {
      Send += "&lat=" + String(latitudeNow, 4);
      Send += "&lon=" + String(longitudeNow, 4);
      Send += "&countSatellite=" +  String(countSatelliteNow);
      if (countSatelliteNow <= countSatellite )countSatellite = 0;
    }
  }

  if (digitalRead(Pin_isFuel) != EEPROM.read(SaveisFuel))
  {
    isFuel = digitalRead(Pin_isFuel);
    Send += "&isFuel=" +  String(isFuel);
  }
  if (digitalRead(Pin_isWork) != EEPROM.read(SaveisWork))
  {
    isWork = digitalRead(Pin_isWork);
    Send += "&isWork=" +  String(isWork);
  }
  if (Send != "")HttpSend(Send);

}
void HttpSend(String Send)
{
  Send = String("AT+HTTPPARA=\"URL\",http://gt001.herokuapp.com/api/v1/tracker/update?idTracker=" + ID + Send);
  commandSIM("AT+HTTPINIT", 100, false, DEBUG);
  commandSIM("AT+HTTPPARA=\"CID\",1", 100, false, DEBUG);
  commandSIM(Send, 100, false, DEBUG);
  commandSIM("AT+HTTPACTION=0", 5000, true, DEBUG);
}

void commandSIM(String command, int timeout, boolean GetData, boolean debug) //отправка команды
{
  String dataSIM808 = "";
  long int t = millis();
  SIM.println(command);
  while (!SIM.available())//ожидание ответа
    if ((t + 5000) < millis())
      if (repeatSend(command))break;

  while ( (t + timeout) > millis())
    while (SIM.available())
      dataSIM808 += char(SIM.read());

  if (debug) Serial.print(dataSIM808);
  if (GetData)eventSIM808(dataSIM808);
}
bool repeatSend(String command)
{
  Serial.println("Error connect to SIM808...repeat send");
  SIM.println(command);
  long int t = millis();
  while (!SIM.available())//ожидание ответа
  {
    if ((t + 5000) < millis())
    {
      Serial.println("Error connect to SIM808...reset");
      digitalWrite(SIM808_on, HIGH);
      delay(2000);
      digitalWrite(SIM808_on, LOW);
      MsTimer2::stop();
      delay(4000);
    }
  }
  return true;
}
void eventSIM808(String dataSIM808)//события с модуля
{
  int i = 0;
  String event = "";
  long int t = millis();
  while (dataSIM808[i] != '+') {
    i++;
    if ((t + 100) < millis())break;
  }
  i++;
  t = millis();
  while (dataSIM808[i] != '=')
  {
    event += dataSIM808[i];
    i++;
    if ((t + 100) < millis())break;
  }

  if (event == "HTTPACTION")
  {
    String Code = "";
    int i = 0, CountComa = 0;
    long int t = millis();
    while (i < dataSIM808.length())
    {
      if ((t + 1000) < millis())break;
      if (dataSIM808[i] == ',') {
        i++;
        while (dataSIM808[i] != ',')
        {
          Code += dataSIM808[i];
          i++;
        }
      }
      i++;
    }
    //Serial.print(Code);
    if (Code == "200")
    {
      EEPROM.update(SaveisFuel, isFuel);
      EEPROM.update(SaveisWork, isWork);
      latitude = latitudeNow;
      longitude = longitudeNow;
    }
    else
    {
      digitalWrite(SIM808_on, HIGH);
      delay(2000);
      digitalWrite(SIM808_on, LOW);
      MsTimer2::stop();
      delay(4000);
    }
  }

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

void  timerInterupt()
{
  wdt_reset();
}
