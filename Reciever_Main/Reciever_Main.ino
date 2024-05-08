#include <esp_now.h>
#include <WiFi.h>
//Servo Code
#include <ESP32Servo.h>
#define PIN_SG90 23 // Output pin used
#define PIN_SG91 21

Servo sg90;
Servo sg91;

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
    char a[32];
    int b;
    float c;
    bool d;
} struct_message;

// Create a struct_message called myData
struct_message myData;

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  //Serial.print("Char: ");
  //Serial.println(myData.a);
  Serial.print("Int: ");
  Serial.println(myData.b);
  //Serial.print("Float: ");
  //Serial.println(myData.c);
  //Serial.print("Bool: ");
  //Serial.println(myData.d);
  Serial.println();

  if(myData.b==1)//BIODEGRADABLE WASTE
  {   
    Serial.println("BIOD"); 
    digitalWrite(LED_BUILTIN,HIGH);
    delay(3000);
    digitalWrite(LED_BUILTIN,LOW);
    //rotation from 90 to 0°
    for (int pos = 90; pos >= 0; pos -= 1) 
    {
      sg90.write(pos);
      sg91.write(90-pos);
      delay(10000);
    }
    // Rotation from 0° to 90
    for (int pos = 0; pos <= 90; pos += 1) 
    {
      sg90.write(pos);
      sg91.write(90-pos);
      break;
    }
  }
  else if(myData.b==2)//NON-BIODEGRADABLE WASTE
  {
    Serial.println("Non-BIOD");
    digitalWrite(LED_BUILTIN,HIGH);
    delay(5000);
    digitalWrite(LED_BUILTIN,LOW);
    //rotation from 90 to 180°
    for (int pos = 90; pos <= 180; pos += 1) 
    {
      sg90.write(pos);
      sg91.write(180-pos);
      delay(10000);
    }
    // Rotation from 180° to 90
    for (int pos = 180; pos >= 90; pos -= 1)
    {
      sg90.write(pos);
      sg91.write(180-pos);
      break;
    }
  }
  else{
    Serial.println("error");
  }

}
 
void setup() {
  pinMode(LED_BUILTIN,OUTPUT);
  //Servo Code
  sg90.setPeriodHertz(50); // PWM frequency for SG90
  sg90.attach(PIN_SG90, 500, 2400); // Minimum and maximum pulse width (in µs) to go from 0° to 180
  sg91.setPeriodHertz(50); // PWM frequency for SG91
  sg91.attach(PIN_SG91, 500, 2400); // Minimum and maximum pulse width (in µs) to go from 0° to 180

  // Initialize Serial Monitor
  Serial.begin(115200);
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

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