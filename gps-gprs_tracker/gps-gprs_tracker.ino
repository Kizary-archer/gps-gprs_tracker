#include <SoftwareSerial.h>

SoftwareSerial SIM(2, 3);//RX, TX

#define DEBUG true

void setup()
{
  //настройка SIM808 при первом включении
  SIM.begin(19200);
  Serial.begin(115200);
  String Sett[] = {"AT+CFUN=1", "AT+CGPSPWR=1"};
  Serial.println("********SIM808 SETTINGS***********");
  for (byte i = 0 ; i < 2; i ++) {
    commandSIM(Sett[i],10,DEBUG);
  }
  SIM808info();//вывод информации о модуле
  Serial.println("******************************");
  Serial.println("Enter command:");

}
void getGPS()
{
  // SIM.println("");
}
void loop()
{
  serialListen();
  delay(100);
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
  SIM.println(command);
  while (!SIM.available())delay(10); //ожидание ответа
  if (debug) {
    long int time = millis();
    while ( (time + timeout) > millis()) {
      while (SIM.available()) {
        out += char(SIM.read());
      }
    }
    Serial.print(out);
  }
}
void SIM808info()
{
  String ATInfo[] = {"name: ", "ATI", "sim: ", "AT+COPS?", "functionality mode: ", "AT+CFUN?", "GPS power", "AT+CGPSPWR?"};
  Serial.println("********SIM808 info***********");
  for (byte i = 0 ; i < 8; i += 2) {
    Serial.print(ATInfo[i]);
    commandSIM(ATInfo[i],10,DEBUG);
  }
}
