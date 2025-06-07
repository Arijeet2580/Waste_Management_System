#include <esp_now.h>
#include <WiFi.h>
#include <ESP32Servo.h>
#include <DHT.h>
#include <FirebaseESP32.h>
#include <Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WiFi credentials - Replace with your actual credentials
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// Firebase credentials - Replace with your actual Firebase config
#define FIREBASE_HOST "your-project.firebaseio.com"
#define FIREBASE_AUTH "your_firebase_secret_key"

// Alert LED Pins
#define BIO_LED 23
#define NBIO_LED 22

// DHT22 Sensor
#define DHT_SENSOR_PIN 33
#define DHT_SENSOR_TYPE DHT22
DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

// Ultrasonic Sensor Pins
#define TRIG_PIN 26
#define ECHO_PIN 25

// TGS2611 Methane Gas Sensor Pin (ESP32 analog pin)
#define TGS2611_PIN 36

// Servo Pins
#define SERVO1_PIN 18
#define SERVO2_PIN 19

// OLED Display (I2C)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Variables
int count = 0;
float distance;
long duration;

// Firebase objects
FirebaseData firebaseData;
FirebaseJson json;
FirebaseConfig config;
FirebaseAuth auth;

// NTP Client for time synchronization
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000); // GMT+5:30

// Servo objects
Servo sg1;
Servo sg2;

// Structure to receive ESP-NOW data
typedef struct struct_message {
    int b;
    float c;
} struct_message;

struct_message myData;

// Function to update OLED display
void updateOLED(String wasteType, float humidity, float temperature, float methanePPM, float distance) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Line 1: Waste Type (larger text)
    display.setTextSize(2);
    display.setCursor(0, 0);
    if (wasteType == "Biodegradable") {
        display.print("BIO");
    } else if (wasteType == "Non-Biodegradable") {
        display.print("NON-BIO");
    } else {
        display.print("UNKNOWN");
    }
    
    // Line 2: Humidity and Temperature
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.print("H:" + String(humidity, 1) + "%  T:" + String(temperature, 1) + "C");
    
    // Line 3: Methane level
    display.setCursor(0, 32);
    display.print("Methane: " + String(methanePPM, 0) + " PPM");
    
    // Line 4: Distance
    display.setCursor(0, 44);
    display.print("Distance: " + String(distance, 1) + " cm");
    
    // Add status indicator
    display.setCursor(0, 56);
    display.print("Status: Active");
    
    display.display();
}

// Callback function for ESP-NOW data reception
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    // Copy received data
    memcpy(&myData, incomingData, sizeof(myData));
    
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("Classification from sender: ");
    Serial.println(myData.b);
    Serial.println();
    
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
    
    // Read TGS2611 Methane sensor (ESP32 analog pin)
    float methane_raw = analogRead(TGS2611_PIN);
    float methane_voltage = (methane_raw / 4095.0) * 3.3; // Convert to voltage
    
    // TGS2611 calibration for methane concentration (adjust based on your calibration)
    // Basic formula: Rs/R0 ratio calculation for methane detection
    float Rs_R0_ratio = methane_voltage / 1.0; // Adjust R0 based on clean air calibration
    float methane_ppm;
    
    if (Rs_R0_ratio > 0.1) {
        // Approximate methane concentration calculation (logarithmic curve)
        methane_ppm = pow(10, ((log10(Rs_R0_ratio) - 0.3) / -0.8)) * 200;
        if (methane_ppm > 10000) methane_ppm = 10000; // Cap maximum reading
        if (methane_ppm < 0) methane_ppm = 0;
    } else {
        methane_ppm = 0;
    }
    
    // Display sensor readings
    if (isnan(tempC) || isnan(tempF) || isnan(humi)) {
        Serial.println("Failed to read from DHT sensor!");
    } else {
        Serial.println("Humidity: " + String(humi) + "%");
        Serial.println("Temperature: " + String(tempC) + "°C ~ " + String(tempF) + "°F");
    }
    
    Serial.println("Serial no: " + String(count));
    Serial.println("Distance: " + String(distance) + " cm");
    Serial.println("Methane Level: " + String(methane_ppm) + " PPM");
    Serial.println("Timestamp: " + timestamp);
    
    // Prepare JSON for Firebase
    json.clear();
    json.set("/distance", distance);
    json.set("/no", count);
    json.set("/humidity", humi);
    json.set("/temperature", tempC);
    json.set("/methane_ppm", methane_ppm);
    json.set("/classification", myData.b);
    json.set("/timestamp", timestamp);
    
    String path = "/Sensor/" + String(millis());
    
    // Send to Firebase
    if (Firebase.setJSON(firebaseData, path.c_str(), json)) {
        Serial.println("Data sent to Firebase successfully");
    } else {
        Serial.println("Firebase error: " + firebaseData.errorReason());
    }
    
    // Reset servos to neutral position
    sg1.write(90);
    sg2.write(90);
    delay(100);
    
    // Waste classification logic and LCD update
    String wasteType = "";
    
    if ((myData.b == 1) || (methane_ppm >= 500) || (humi >= 74.5)) {
        // BIODEGRADABLE WASTE (organic waste produces methane during decomposition)
        wasteType = "Biodegradable";
        Serial.println("Classification: Biodegradable Waste");
        digitalWrite(BIO_LED, HIGH);
        digitalWrite(NBIO_LED, LOW);
        
        // Update OLED display
        updateOLED(wasteType, humi, tempC, methane_ppm, distance);
        
        // Move servos for biodegradable waste
        sg1.write(0);
        sg2.write(180);
        delay(5000); // Wait 5 seconds
        
        // Return to neutral position
        sg1.write(90);
        sg2.write(90);
        digitalWrite(BIO_LED, LOW);
        
    } else if ((myData.b == 2) || (methane_ppm < 500) || (humi < 74.5)) {
        // NON-BIODEGRADABLE WASTE
        wasteType = "Non-Biodegradable";
        Serial.println("Classification: Non-Biodegradable Waste");
        digitalWrite(NBIO_LED, HIGH);
        digitalWrite(BIO_LED, LOW);
        
        // Update OLED display
        updateOLED(wasteType, humi, tempC, methane_ppm, distance);
        
        // Move servos for non-biodegradable waste
        sg1.write(180);
        sg2.write(0);
        delay(5000); // Wait 5 seconds
        
        // Return to neutral position
        sg1.write(90);
        sg2.write(90);
        digitalWrite(NBIO_LED, LOW);
        
    } else {
        wasteType = "Unknown";
        Serial.println("Unknown waste type - no classification");
        
        // Update OLED display
        updateOLED(wasteType, humi, tempC, methane_ppm, distance);
        
        // Turn off both LEDs
        digitalWrite(BIO_LED, LOW);
        digitalWrite(NBIO_LED, LOW);
    }
    
    delay(100); // Small delay for servo stability
    Serial.println("----------------------------");
}

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 Smart Waste Segregation System Starting...");
    
    // Initialize DHT sensor
    dht_sensor.begin();
    
    // Initialize LED pins
    pinMode(BIO_LED, OUTPUT);
    pinMode(NBIO_LED, OUTPUT);
    digitalWrite(BIO_LED, LOW);
    digitalWrite(NBIO_LED, LOW);
    
    // Initialize ultrasonic sensor pins
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    digitalWrite(TRIG_PIN, LOW);
    
    // Initialize TGS2611 analog pin
    pinMode(TGS2611_PIN, INPUT);
    
    // Initialize servos
    sg1.attach(SERVO1_PIN);
    sg2.attach(SERVO2_PIN);
    sg1.write(90); // Neutral position
    sg2.write(90); // Neutral position
    
    // Initialize OLED display
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
    }
    
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("SMART");
    display.println("WASTE");
    display.setTextSize(1);
    display.setCursor(0, 40);
    display.println("System Starting...");
    display.display();
    delay(2000);
    
    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("Connected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    // Initialize NTP Client
    timeClient.begin();
    timeClient.update();
    Serial.println("NTP time synchronized");
    
    // Initialize Firebase
    config.host = FIREBASE_HOST;
    config.signer.tokens.legacy_token = FIREBASE_AUTH;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    Serial.println("Firebase initialized");
    
    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    Serial.println("ESP-NOW initialized successfully");
    
    // Register callback function
    esp_now_register_recv_cb(OnDataRecv);
    Serial.println("ESP-NOW callback registered");
    
    // Display ready message on OLED
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("SYSTEM");
    display.println("READY");
    display.setTextSize(1);
    display.setCursor(0, 40);
    display.println("Waiting for waste...");
    display.setCursor(0, 56);
    display.println("ESP-NOW Active");
    display.display();
    
    Serial.println("System ready - waiting for data...");
}

void loop() {
    // Main loop - ESP-NOW callback handles all processing
    // Keep WiFi and NTP updated
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected, reconnecting...");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            Serial.print(".");
        }
        Serial.println("WiFi reconnected");
    }
    
    // Update time periodically
    static unsigned long lastTimeUpdate = 0;
    if (millis() - lastTimeUpdate > 60000) { // Update every minute
        timeClient.update();
        lastTimeUpdate = millis();
    }
    
    delay(100); // Small delay to prevent watchdog issues
}
