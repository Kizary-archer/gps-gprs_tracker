#include <SoftwareSerial.h>

SoftwareSerial SIM(2, 3);//RX, TX

void setup()
{
  //настройка SIM808 при первом включении
  SIM.begin(9600);
  Serial.begin(9600);

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
  if (Serial.available() > 0)
    SIM.write(Serial.read());
  if (SIM.available() > 0)
    Serial.write(SIM.read());

}
void SIM808info()
{
  String ATInfo[] = {"port speed:","AT+IPR?","port settings:","AT+ICF?","name","AT+GMM","sim""AT+COPS?"};
  Serial.println("********SIM808 info***********");


  SIM.println("AT+GMM");
  Serial.write(SIM.read());
  
  SIM.println("AT+COPS?");
  delay(1000);
  Serial.write(SIM.read());
}
