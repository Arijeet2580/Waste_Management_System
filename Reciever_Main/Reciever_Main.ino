#include <esp_now.h>
#include <WiFi.h>
#include <ESP32Servo.h>
#include <DHT.h>
#include <FirebaseESP32.h>
#include <Wire.h>
#include <DHT.h>
#include <NTPClient.h>      // For NTP time sync
#include <WiFiUdp.h>        // Required for NTP
//Alert LED Code
#define BIO_LED 25
#define NBIO_LED 26

#define DHT_SENSOR_PIN  33
#define DHT_SENSOR_TYPE DHT22
DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE); 

// Ultrasonic Sensor Pins
#define TRIG_PIN 26
#define ECHO_PIN 25

// Variables
int count = 0;
float distance;
int duration;

FirebaseData firebaseData;
FirebaseJson json;

FirebaseConfig config;    // Firebase configuration
FirebaseAuth auth;        // Firebase authentication

// NTP Client for time synchronization
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000);  // GMT+5:30 (19800 sec offset)

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
  //Moupriya Code
    // Update NTP time
  timeClient.update();
  String timestamp = timeClient.getFormattedTime();

  // Read ultrasonic sensor
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(20);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.034 / 2;
  count++;

  // Read DHT sensor
  float humi = dht_sensor.readHumidity();
  float tempC = dht_sensor.readTemperature();
  float tempF = dht_sensor.readTemperature(true);

  if (isnan(tempC)  isnan(tempF)  isnan(humi)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.println("Humidity: " + String(humi) + "%");
    Serial.println("Temperature: " + String(tempC) + "°C  ~  " + String(tempF) + "°F");
  }
  
  Serial.println("Serial no: " + String(count));
  Serial.println("Distance: " + String(distance));
  Serial.println("Timestamp: " + timestamp);

  // Prepare JSON for Firebase
  json.set("/distance", distance);
  json.set("/no", count);
  json.set("/humidity", humi);
  json.set("/temperature", tempC);
  json.set("/timestamp", timestamp);  // Add timestamp

  String path = "/Sensor/" + String(millis());

  // Send to Firebase
  if (Firebase.setJSON(firebaseData, path.c_str(), json)) {
    Serial.println("Data sent to Firebase");
  } else {
    Serial.println("Firebase error: " + firebaseData.errorReason());
  }
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
  //Moupriya Code
  Serial.begin(9600);
  dht_sensor.begin();

  // Initialize Ultrasonic Sensor
  pinMode(TRIG_PIN, OUTPUT); 
  pinMode(ECHO_PIN, INPUT); 
  digitalWrite(TRIG_PIN, LOW); 

  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  Serial.println(WiFi.localIP());

  // Initialize NTP Client
  timeClient.begin();
  timeClient.update();  // Fetch current time

  // Initialize Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  //Servo Code
  sg1.attach(sg1pin);
  sg2.attach(sg2pin);

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

