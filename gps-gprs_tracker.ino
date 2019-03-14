#include <SoftwareSerial.h>

SoftwareSerial SIM(2, 3);//RX, TX

#define DEBUG true


void(* resetFunc) (void) = 0;//перезагрузка

void setup()
{
  //настройка SIM808 при первом включении
  SIM.begin(19200);
  Serial.begin(115200);
  String Sett[] = {"AT+CFUN=1", "AT+CGNSPWR=1", "AT+CGNSSEQ=GGA"};
  Serial.println("********SIM808 SETTINGS***********");
  for (byte i = 0 ; i < 3; i ++) {
    commandSIM(Sett[i], 10,false, DEBUG);
  }
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

void getGPS()
{
  commandSIM("AT+CGNSPWR=1", 10,false, DEBUG);
  commandSIM("AT+CGNSSEQ=GGA", 10,false, DEBUG);
  
  commandSIM("AT+CGPSINF=2", 1000, false, DEBUG);
}

void parseGPSdata()
{
  
}

void commandSIM(String command, int timeout,boolean GetData, boolean debug) //вывод ответа на AT команду
{
  String dataSIM808 = "";
  long int t = millis();
  SIM.println(command);
  while (!SIM.available())//ожидание ответа
    if ((t + 10000) < millis())
    {
      Serial.println("Error connect to SIM808...RESET");
      delay(1000);  
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
  //Serial.println(dataSIM808);
  int i = 0;
  String event = "";
  while(dataSIM808[i]!=':')
  {
    while(dataSIM808[i]!='+'){i++;}
    event += dataSIM808[i];
    i++;
   
  } 
  //Serial.println(dataSIM808[2]);  
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
  char *ATInfo[] = {"name: ", "ATI", "sim: ", "AT+COPS?","phone nomber: ","+7(969)705-57-85", "functionality mode: ", "AT+CFUN?", "GPS power: ", "AT+CGPSPWR?", "GPS mode: ", "AT+CGPSRST?", "GPS parsed mode: ", "AT+CGNSSEQ?"};
  Serial.println("********SIM808 info***********");
  for (byte i = 0 ; i < 14; i += 2) {
    Serial.print(ATInfo[i]);
    commandSIM(ATInfo[i + 1], 10,false, DEBUG);
  }
}
