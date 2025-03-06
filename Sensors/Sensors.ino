#include <DHT.h>
#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN,DHTTYPE);
#define MQ137 36
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  dht.begin();
  pinMode(MQ137,INPUT);
}
/*5-200PPM   MQ137 GAS DETECTION VALUE*/
/* BIODEGRADABLE DETECTED NEAR TO 40PPM ----> 735ADC*/
void loop() {
  // put your main code here, to run repeatedly:
  delay(2000);
  //As Sampling Period is 2 Seconds so we need delay
  double humi = dht.readHumidity();
  double temp = dht.readTemperature();
  //For Farhenhiet Param True need to be Passed 
  if(isnan(humi)|| isnan(temp)){
    Serial.println("Failed to read from DHT Sensor");
    return ;
  }
  Serial.printf("Humidity: %d, Temperature: %d\n",humi,temp);
  float SV = analogRead(MQ137);
  float ppm = (195*(SV/4095))+5;
  Serial.printf("MQ137 Val: %.2f | PPM: %.2f\n",SV,ppm);
}
