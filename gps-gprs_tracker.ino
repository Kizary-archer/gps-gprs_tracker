#include <SoftwareSerial.h>
#include <EEPROM.h>

SoftwareSerial SIM(2, 3);//RX, TX

#define DEBUG true
#define SaveisFuel 0
#define SaveisWork 1
#define SaveisPayload 2

//digital pins
#define SIM808_on 4 //вывод модуля из сна
#define Pin_isFuel 5 //наличие топлива
#define Pin_isPayload 8 //наличие нагрузки

float latitudeNow = 0, longitudeNow = 0, latitude = 0, longitude = 0;
int countSatellite = 0, countSatelliteNow = 0;
String ID = "5c8a814dc8cab2307ff8bc94";
boolean isFuel, isWork = true, isPayload;

void(* resetFunc) (void) = 0;//перезагрузка

void setup()  //настройка SIM808 при первом включении
{
  SIM.begin(19200);
  Serial.begin(115200);

  pinMode(SIM808_on, OUTPUT);
  pinMode(Pin_isFuel, INPUT);
  pinMode(Pin_isPayload, INPUT);

  SIM.println("AT");
  long int t = millis();
  Serial.print("Wait connect");
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
    resetFunc();
  }
  delay(2000);
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
    "AT+SAPBR=3,1,\"APN\",\"internet.beeline.ru\"",
    "AT+SAPBR=3,1,\"USER\",\"beeline\"",
    "AT+SAPBR=3,1,\"PWD\",\"beeline\"",
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
    if ((t + 1000) < millis()) // проверка состояния генератора каждую минуту
    {
      checkGeneratorStatus();
      commandSIM("AT+HTTPACTION=2", 1000, true, DEBUG);
      delay(100);
      break;
    }
  }
}
void checkGeneratorStatus()
{
  String Send = "";
  float R = 0.0001;
  commandSIM("AT+CGPSINF=2", 1000, true, DEBUG); //обновление координат
  if ((pow(latitudeNow - latitude, 2) + pow(longitudeNow - longitude, 2) >= pow(R, 2)) || (countSatelliteNow > countSatellite ))
  {
    Serial.println("Оно не в кругу");
    Send += "&lat=" + String(latitudeNow, 4);
    Send += "&lon=" + String(longitudeNow, 4);
    Send += "&countSatellite=" +  String(countSatelliteNow);
    if (countSatelliteNow <= countSatellite )countSatellite = 0;
  }
  else   Serial.println("Оно в кругу");

  if (digitalRead(Pin_isFuel) != EEPROM.read(SaveisFuel))
  {
    isFuel = digitalRead(Pin_isFuel);
    Send += "&isFuel=" +  String(isFuel);
  }
  if (digitalRead(Pin_isPayload) != EEPROM.read(SaveisPayload))
  {
    isPayload = digitalRead(Pin_isPayload);
    Send += "&isPayload=" +  String(isPayload);
  }
  if (Send != "")HttpSend(Send);

}
void HttpSend(String Send)
{
  Send = String("AT+HTTPPARA=\"URL\",http://gt001.herokuapp.com/api/v1/tracker/update?idTracker=" + ID + Send);
  commandSIM("AT+HTTPINIT", 100, false, DEBUG);
  commandSIM("AT+HTTPPARA=\"CID\",1", 100, false, DEBUG);
  commandSIM(Send, 100, false, DEBUG);
  commandSIM("AT+HTTPACTION=0", 5000, false, DEBUG);
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
  delay(1000);
  SIM.println(command);
  long int t = millis();
  while (!SIM.available())//ожидание ответа
  {
    if ((t + 5000) < millis())
    {
      Serial.println("Error connect to SIM808...reset");
      delay(1000);
      resetFunc();
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
  if (event == "HTTPACTION")parseHTTPdata(dataSIM808);
  else if (event == "CGPSINF")parseGPSdata(dataSIM808);
}

void parseHTTPdata(String dataSIM808)
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
  Serial.println(Code);
  if (Code == "200")
  {
    EEPROM.update(SaveisFuel, isFuel);
    EEPROM.update(SaveisPayload, isPayload);
  }
  else if ((Code == "601") || (Code == "603"))resetFunc(); // перезагрузка при ошибке сети
}
void parseGPSdata(String dataSendGPS)
{
  String GPSdata[4]; //  latitude,longitude,state,satellite
  int coma[4] = {2, 4, 6, 7};
  byte CountComa = 0;
  int i = 0, j = 0, L = dataSendGPS.length();
  long int t = millis();
  while (i < L)
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
  if (GPSdata[2].toInt())
  {
    latitude = latitudeNow;
    longitude = longitudeNow;
    if (countSatelliteNow > countSatellite )countSatellite = countSatelliteNow;
    latitudeNow = atof(GPSdata[0].c_str());
    longitudeNow = atof(GPSdata[1].c_str());
    countSatelliteNow = GPSdata[3].toInt();
  }
  if (DEBUG)
  {
    Serial.println("-------------");
    Serial.print("latitude: ");
    Serial.println(latitudeNow, 4);
    Serial.print("longitude: ");
    Serial.println(longitudeNow, 4);
    Serial.print("state: ");
    Serial.println(GPSdata[2]);
    Serial.print("satellite: ");
    Serial.println(countSatelliteNow);
    Serial.println("-------------");
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
