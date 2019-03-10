#include <SoftwareSerial.h>

SoftwareSerial SIM(2, 3);//RX, TX

void setup()
{
  //настройка SIM808 при первом включении
  SIM.begin(19200);
  Serial.begin(115200);

  SIM808info();//вывод информации о модуле
  //Serial.println("Enter command:");

}
void loop()
{
  serialListen();
  delay(100);
}
void serialListen()
{
  while (Serial.available() > 0)
  {
    SIM.write(Serial.read());
    delay(10);
  }
  OutSIM();

}
void SIM808info()
{
  String ATInfo[] = {"name: ", "ATI", "sim: ", "AT+COPS?","functionality mode: ","AT+CFUN?"};
  Serial.println("********SIM808 info***********");
  // byte s = ATInfo.length;
  for (byte i = 0 ; i < 6; i += 2) {
    Serial.print(ATInfo[i]);
    SIM.println(ATInfo[i + 1]);
    while (!SIM.available())delay(10); //ожидание ответа
    OutSIM();
  }
}
void OutSIM() //вывод ответа на AT команду
{
  while (SIM.available()) {
    Serial.write(SIM.read());
    delay(10);
  }
}
