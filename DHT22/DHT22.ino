#include <DHT.h>
#define DHTPIN 4 //Digital Pin Connected to DHT Sensor
#define DHTTYPE DHT22

DHT dht(DHTPIN,DHTTYPE);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  dht.begin();  
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(2000);
  //As Sampling Period is 2 Seconds so we need delay
  double humi = dht.readHumidity();
  double temp = dht.readTemperature();
  //For Farhenhiet Param True need to be Passed 
  if(isNan(humi)|| isNan(temp)){
    Serial.println("Failed to read from DHT Sensor");
    return ;
  }
  Serial.printf("Humidity: %d",humi);
  Serial.print(", ");
  Serial.printf("Temperature: %d",temp);
  Serial.println();

}
