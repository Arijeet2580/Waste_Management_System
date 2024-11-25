#include <esp_now.h>
#include <WiFi.h>
//Alert LED Code
#define BIO_LED 25
#define NBIO_LED 26
//Servo Code
#include <ESP32Servo.h>
//DHT22 Code
#include <DHT.h>
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN,DHTTYPE);
Servo sg1;
Servo sg2;

int sg1pin=18;
int sg2pin=19;

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
    //char a[32];
    int b;
    float c;
    //bool d;
} struct_message;

// Create a struct_message called myData
struct_message myData;

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Int: ");
  Serial.println(myData.b);
  //Serial.print("Float: ");
  //Serial.println(myData.c);
  Serial.println();
  sg1.write(90);
  delay(1);
  sg2.write(90);

  // MQ137 NH3 DETECTION
  float nh3=analogRead(A0);
  float nh3ppm = (195*(nh3/4095))+5;
  // DHT22 Sensor 
  delay(2000);
  //As Sampling Period is 2 Seconds so we need delay  
  double humi = dht.readHumidity();
  double temp = dht.readTemperature();

  if(isnan(humi)|| isnan(temp)){
    Serial.println("Failed to read from DHT Sensor");
    return ;
  }  
  if((myData.b==1)||(nh3ppm >= 40) || (humi >= 74.5))//BIODEGRADABLE WASTE
  {   
    Serial.println("Biodegradable"); 
    digitalWrite(BIO_LED,HIGH);
    sg1.write(0);
    delay(1);
    sg2.write(180);
    delay(5000);//Halt delay 5 seconds
    sg1.write(90);
    delay(1);
    sg2.write(0);
    delay(10);//For Servo Working physically
    //Alert LED
    digitalWrite(BIO_LED,LOW);
  }
  else if((myData.b==2) ||(nh3ppm < 40) || (humi < 74.5) )//NON-BIODEGRADABLE WASTE
  {
    Serial.println("Non-Biodegradable");
    digitalWrite(NBIO_LED,HIGH);
    sg1.write(180);
    delay(1);
    sg2.write(0);
    delay(5000);//Halt delay 5 Seconds
    sg1.write(90);
    delay(1);
    sg2.write(90);
    digitalWrite(BIO_LED,LOW);
    delay(10); //For Servo Working physically
  }
  else{
    Serial.println("The Coming Dataset is not from any conditions given");
  }
  delay(1);//For Servo Working physically
}
 
void setup() {
  //Servo Code
  sg1.attach(sg1pin);
  sg2.attach(sg2pin);

  // Initialize Serial Monitor
  Serial.begin(115200);
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  //DHT22 Code
  dht.begin();

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
}
 
void loop(){
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);
}

