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
    commandSIM(Sett[i], 10, DEBUG);
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
  commandSIM("AT+CGNSPWR=1", 10, DEBUG);
  commandSIM("AT+CGNSSEQ=GGA", 10, DEBUG);
  //commandSIM("AT+CGNSSEQ?", 1000, DEBUG);
  commandSIM("AT+CGPSINF=2", 1000, DEBUG);
  // commandSIM("AT+CGNSPWR=0", 10, DEBUG);
}

void serialListen()
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
void commandSIM(String command, int timeout, boolean debug) //вывод ответа на AT команду
{
  String out = "";
  long int t = millis();
  SIM.println(command);
  while (!SIM.available())//ожидание ответа
    if ((t + 10000) < millis())
    {
      Serial.print("Error connect...RESET");
      delay(1000);
      resetFunc();
    }

  t = millis();
  while ( (t + timeout) > millis()) {
    while (SIM.available()) {
      out += char(SIM.read());
    }
  }
  if (debug) Serial.print(out);
}
void SIM808info()
{
  String ATInfo[] = {"name: ", "ATI", "sim: ", "AT+COPS?", "functionality mode: ", "AT+CFUN?", "GPS power: ", "AT+CGPSPWR?", "GPS mode: ", "AT+CGPSRST?", "GPS parsed mode: ", "AT+CGNSSEQ?"};
  Serial.println("********SIM808 info***********");
  for (byte i = 0 ; i < 12; i += 2) {
    Serial.print(ATInfo[i]);
    commandSIM(ATInfo[i + 1], 10, DEBUG);
  }
}
