#include <SoftwareSerial.h>

SoftwareSerial SIM(2, 3);//RX, TX

#define DEBUG true
#define PHONE +7(969)705-57-85
float latitudeNow, longitudeNow, latitudeLast = 0, longitudeLast = 0;
int countSatelliteLast, countSatelliteNow;
String ID = "5c8a8175c9ea0e65e0e20ad8";
boolean isFuel = true, isWork = true, isPayload = true;

void(* resetFunc) (void) = 0;//перезагрузка

void setup()  //настройка SIM808 при первом включении
{
  SIM.begin(19200);
  Serial.begin(115200);

  long int t = millis();
  while ( (t + 1000) > millis()) //ожидание включения модуля
  {
    while (SIM.available()) Serial.write(SIM.read());
  }
  Serial.println("\n********SIM808 SETTINGS***********");
  char *Sett[] = {
    "AT+CFUN=1",
    "AT+CGNSPWR=1",
    "AT+CGNSSEQ=GGA",
    "AT+GSMBUSY=1",
    "AT+CLIP=0"
  };
  for (byte i = 0 ; i < 5; i ++) commandSIM(Sett[i], 10, false, DEBUG);

  initGPRS();
  if (DEBUG)SIM808info(); //вывод информации о модуле
  Serial.println("******************************");
  Serial.println("Enter command:");

}
void initGPRS()
{
  char *gprsAT[] = {
    "AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"",  //Установка настроек подключения
    "AT+SAPBR=3,1,\"APN\",\"internet.beeline.ru\"",
    "AT+SAPBR=3,1,\"USER\",\"beeline\"",
    "AT+SAPBR=3,1,\"PWD\",\"beeline\"",
    "AT+SAPBR=1,1",  //Устанавливаем GPRS соединение
    "AT+HTTPINIT",  //Инициализация http сервиса
    "AT+HTTPPARA=\"CID\",1"  //Установка CID параметра для http сессии
  };
  for (byte i = 0 ; i < 7; i ++) {
    commandSIM(gprsAT[i], 100, false, DEBUG);
  }
}


void SIM808info()//вывод информации о настройках
{
  char *ATInfo[] = {"name: ", "ATI",
                    "sim: ", "AT+COPS?",
                    "functionality mode: ", "AT+CFUN?",
                    "GPS power: ", "AT+CGPSPWR?",
                    "GPS mode: ", "AT+CGPSRST?",
                    "GPS parsed mode: ", "AT+CGNSSEQ?",
                    "call mode: ", "AT+GSMBUSY?"
                   };
  Serial.println("********SIM808 info***********");
  for (byte i = 0 ; i < sizeof(ATInfo) / 2; i += 2) {
    Serial.print(ATInfo[i]);
    commandSIM(ATInfo[i + 1], 10, false, DEBUG);
  }
}

void loop()
{
  serialListen();
  commandSIM("AT+CGPSINF=2", 1000, true, DEBUG);
  // HttpSend();
  checkGeneratorStatus();
  delay(5000);
}
void checkGeneratorStatus()
{
  String Send = "";
  float R = 0.0010;
  if (pow(latitudeNow - latitudeLast, 2) + pow(longitudeNow - longitudeLast, 2) >= pow(R, 2))
  {
    Serial.println("Оно не в кругу");
    if (latitudeNow != latitudeLast)Send += "&lat=" + String(latitudeNow, 4);
    if (longitudeNow != longitudeLast)Send += "&lon=" + String(longitudeNow, 4);
  }
  else   Serial.println("Оно в кругу");
  if (countSatelliteLast != countSatelliteNow)Send += "&countSatellite=" +  String(countSatelliteNow);
    Serial.println(Send);
  //HttpSend(Send)

}
void HttpSend(String Send)
{
  //  String Send = "idTracker=" + ID + "&isFuel=" + isFuel + "&isWork=" + isWork + "&isPayload=" + isPayload + "&countSatellite=" + countSatellite + "&lat=" + latitude + "&lon=" + longitude + "";
  //commandSIM("AT+HTTPPARA=\"URL\",\"http://gt0008.herokuapp.com/api/v1/tracker/update?" + Send + "\"", 100, false, DEBUG);
  //commandSIM("AT+HTTPACTION=0", 5000, true, DEBUG);
}

void commandSIM(String command, int timeout, boolean GetData, boolean debug) //вывод ответа на AT команду
{
  String dataSIM808 = "";
  long int t = millis();
  SIM.println(command);
  while (!SIM.available())//ожидание ответа
  {
    if ((t + 5000) < millis())
    {
      Serial.println("Error connect to SIM808...RESET");
      delay(1000);
      resetFunc();
    }
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
  if (event == "CGPSINF")parseGPSdata(dataSIM808);
  else if (event = "HTTPACTION")parseHTTPresultCode(dataSIM808);
}

void parseGPSdata(String dataSendGPS)
{

  String GPSdata[4]; //  latitude,longitude,state,satellite
  int coma[4] = {2, 4, 6, 7};
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
  //convert
  if (GPSdata[2].toInt())
  {
    latitudeLast = latitudeNow;
    longitudeLast = longitudeNow;
    countSatelliteLast = countSatelliteNow;
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

void parseHTTPresultCode(String dataHTTPSend)
{
  String Code;
  int i = 0, CountComa = 0;
  while (i < dataHTTPSend.length())
  {
    if (dataHTTPSend[i] == ',') {
      i++;
      while (dataHTTPSend[i] != ',')
      {
        Code += dataHTTPSend[i];
        i++;
      }
    }
    i++;
  }
  if (Code.toInt() == 200) Serial.println("the message is delivered");
  else Serial.println("the message is not delivered");
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
