#include <esp_now.h>
#include <WiFi.h>
#include <DHT.h>

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
//DHT Initialisation
DHT dht(26,DHT22);

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Char: ");
  Serial.println(myData.a);
  Serial.print("Int: ");
  Serial.println(myData.b);
  Serial.print("Float: ");
  Serial.println(myData.c);
  Serial.print("Bool: ");
  Serial.println(myData.d);
  Serial.println();

  if(myData.b==0){    //MYDATA.B IS THE INTEGER VALUE SENT BY SENDER

    digitalWrite(LED_BUILTIN,HIGH);
    delay(5000);
    digitalWrite(LED_BUILTIN,LOW);
    delay(5000);
  }

}
 
void setup() {
  //DHT 22 Setup
  dht.begin();
  delay(1500);
  // Initialize Serial Monitor
  Serial.begin(115200);
  pinMode(LED_BUILTIN,OUTPUT);
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {
  float temp=dht.readTemperature();
  float humidity=dht.readHumidity();

}