#include <SoftwareSerial.h>

SoftwareSerial GPSmodule(2, 3);//RX, TX
//**********************************************
void setup()
{
GPSmodule.begin(9600);   
Serial.begin(9600);  
Serial.print("Please enter AT command:");  
delay(100);
}
void loop()
{
 if (Serial.available()>0)
 GPSmodule.write(Serial.read());
 if (GPSmodule.available()>0)
 Serial.write(GPSmodule.read());
}
