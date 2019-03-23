#include <SoftwareSerial.h>

SoftwareSerial SIM(2, 3);//RX, TX

#define DEBUG true
float latitudeNow = 0, longitudeNow = 0, latitudeLast = 0, longitudeLast = 0;
int countSatelliteLast = 0, countSatelliteNow = 0;
String ID = "5c8a8175c9ea0e65e0e20ad8";
boolean isFuel = true, isWork = true, isPayload = true;

void(* resetFunc) (void) = 0;//перезагрузка

void setup()  //настройка SIM808 при первом включении
{
  SIM.begin(19200);
  Serial.begin(115200);

  long int t = millis();
  while ( (t + 5000) > millis()) //ожидание включения модуля
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
  for (byte i = 0 ; i < 5; i ++) commandSIM(Sett[i], 100, false, DEBUG);

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
    "AT+SAPBR=2,1" //Проверяем как настроилось
  };
  for (byte i = 0 ; i < 6; i ++) {
    commandSIM(gprsAT[i],2000, false, DEBUG);
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

  long int t = millis();
  while (1)
  {
    serialListen();
    if ((t + 1000) < millis()) // проверка состояния генератора каждую минуту
    {
      commandSIM("AT+CGPSINF=2", 1000, true, DEBUG);
      checkGeneratorStatus();
      break;
    }
  }
}
void checkGeneratorStatus()
{
  String Send = "";
  float R = 0.0010;
  if (pow(latitudeNow - latitudeLast, 2) + pow(longitudeNow - longitudeLast, 2) >= pow(R, 2))
  {
    Serial.println("Оно не в кругу");
    Send += "&lat=" + String(latitudeNow, 4);
    Send += "&lon=" + String(longitudeNow, 4);
    Send += "&countSatellite=" +  String(countSatelliteNow);
  }
  else   Serial.println("Оно в кругу");
  if (Send != "")HttpSend(Send);

}
void HttpSend(String Send)
{
  Send = String("AT+HTTPPARA=\"URL\",http://gt0008.herokuapp.com/api/v1/tracker/update?idTracker=" + ID + Send);
  Serial.println(Send);
  commandSIM("AT+HTTPINIT", 1000, false, DEBUG);
  commandSIM("AT+HTTPPARA=\"CID\",1", 1000, false, DEBUG);
  commandSIM(Send, 1000, false, DEBUG);
  commandSIM("AT+HTTPACTION=0", 5000, true, DEBUG);
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
  Serial.println(Code);
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
