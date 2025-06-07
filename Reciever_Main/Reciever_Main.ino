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

// WiFi credentials
#define WIFI_SSID "MouPriya"
#define WIFI_PASSWORD "h9nxcjvu"

// Firebase credentials
#define FIREBASE_HOST "https://sensor-data-e1f18-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "AIzaSyCG0V9n4ynFrGtaYy8zttaLXrW5yy7FF9U"

// Alert LED Pins
#define BIO_LED 23
#define NBIO_LED 22
#define BIN_FULL_LED 21
#define USER_DETECTED_LED 20  // New LED for user proximity detection

// DHT22 Sensor
#define DHT_SENSOR_PIN 33
#define DHT_SENSOR_TYPE DHT22
DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

// Ultrasonic Sensor Pins for Bin Level Monitoring
#define BIN_TRIG_PIN 26
#define BIN_ECHO_PIN 25

// Ultrasonic Sensor Pins for User Proximity Detection
#define PROX_TRIG_PIN 14  // New trigger pin for proximity sensor
#define PROX_ECHO_PIN 12  // New echo pin for proximity sensor

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

// Bin Level Configuration
#define BIN_HEIGHT 50.0        // Total bin height in cm
#define BIN_FULL_THRESHOLD 85  // Bin considered full at 85%
#define BIN_WARNING_THRESHOLD 70 // Warning at 70%

// User Proximity Configuration
#define USER_DETECTION_DISTANCE 100.0  // Detect user within 100cm
#define USER_CLOSE_DISTANCE 30.0       // User is very close within 30cm
#define PROXIMITY_CHECK_INTERVAL 1000   // Check proximity every 1 second

// Variables
int count = 0;
float binDistance;           // Distance from sensor to waste surface
float binLevel;             // Calculated bin fill level percentage
float userDistance;         // Distance to user for proximity detection
long binDuration, proxDuration;
bool binFull = false;
bool binWarning = false;
bool userDetected = false;
bool userClose = false;
unsigned long lastProximityCheck = 0;
unsigned long userDetectedTime = 0;
unsigned long lastUserActivity = 0;

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

// Function to calculate bin fill level
float calculateBinLevel(float distance) {
    // If distance is greater than bin height, bin is empty
    if (distance >= BIN_HEIGHT) {
        return 0.0;
    }
    
    // Calculate fill level percentage
    float fillHeight = BIN_HEIGHT - distance;
    float fillPercentage = (fillHeight / BIN_HEIGHT) * 100.0;
    
    // Ensure percentage is within bounds
    if (fillPercentage < 0) fillPercentage = 0;
    if (fillPercentage > 100) fillPercentage = 100;
    
    return fillPercentage;
}

// Function to get bin status string
String getBinStatus(float level) {
    if (level >= BIN_FULL_THRESHOLD) {
        return "FULL";
    } else if (level >= BIN_WARNING_THRESHOLD) {
        return "WARNING";
    } else if (level > 30) {
        return "MEDIUM";
    } else if (level > 10) {
        return "LOW";
    } else {
        return "EMPTY";
    }
}

// Function to get user proximity status
String getUserProximityStatus() {
    if (userClose) {
        return "USER CLOSE";
    } else if (userDetected) {
        return "USER DETECTED";
    } else {
        return "NO USER";
    }
}

// Function to read user proximity
void readUserProximity() {
    digitalWrite(PROX_TRIG_PIN, HIGH);
    delayMicroseconds(20);
    digitalWrite(PROX_TRIG_PIN, LOW);
    proxDuration = pulseIn(PROX_ECHO_PIN, HIGH);
    userDistance = proxDuration * 0.034 / 2;
    
    // Update proximity status
    bool prevUserDetected = userDetected;
    bool prevUserClose = userClose;
    
    userClose = (userDistance <= USER_CLOSE_DISTANCE && userDistance > 0);
    userDetected = (userDistance <= USER_DETECTION_DISTANCE && userDistance > 0);
    
    // Log proximity changes
    if (userDetected != prevUserDetected || userClose != prevUserClose) {
        Serial.println("User Distance: " + String(userDistance) + " cm");
        Serial.println("Proximity Status: " + getUserProximityStatus());
        
        if (userDetected) {
            userDetectedTime = millis();
            lastUserActivity = millis();
        }
    }
}

// Function to handle user proximity alerts and actions
void handleUserProximity() {
    // Control user detection LED
    if (userClose) {
        digitalWrite(USER_DETECTED_LED, HIGH);  // Solid on when user is very close
    } else if (userDetected) {
        // Blink LED when user is detected but not too close
        static unsigned long lastBlinkProx = 0;
        static bool proxLedState = false;
        if (millis() - lastBlinkProx > 300) {
            proxLedState = !proxLedState;
            digitalWrite(USER_DETECTED_LED, proxLedState);
            lastBlinkProx = millis();
        }
    } else {
        digitalWrite(USER_DETECTED_LED, LOW);   // Off when no user detected
    }
    
    // Activate display backlight or wake up system when user is detected
    if (userDetected && (millis() - userDetectedTime < 10000)) { // Keep active for 10 seconds after detection
        // System is in active mode - display is bright and responsive
    }
}

// Function to update OLED display with bin level and user proximity
void updateOLED(String wasteType, float humidity, float temperature, float methanePPM, float binLevel) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Line 1: Waste Type (larger text) or User Status
    display.setTextSize(2);
    display.setCursor(0, 0);
    if (userClose) {
        display.print("READY");
    } else if (wasteType == "Biodegradable") {
        display.print("BIO");
    } else if (wasteType == "Non-Biodegradable") {
        display.print("NON-BIO");
    } else {
        display.print("READY");
    }
    
    // Line 2: Bin Level and User Proximity
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.print("BIN:" + String(binLevel, 1) + "% ");
    if (userDetected) {
        display.print("USER:" + String(userDistance, 0) + "cm");
    } else {
        display.print(getBinStatus(binLevel));
    }
    
    // Line 3: Environmental data
    display.setCursor(0, 32);
    if (userDetected) {
        display.print("H:" + String(humidity, 1) + "%  T:" + String(temperature, 1) + "C");
    } else {
        display.print("H:" + String(humidity, 1) + "%  T:" + String(temperature, 1) + "C");
    }
    
    // Line 4: Methane level or proximity status
    display.setCursor(0, 44);
    if (userClose) {
        display.print(">>> PLACE WASTE <<<");
    } else {
        display.print("CH4: " + String(methanePPM, 0) + " PPM");
    }
    
    // Line 5: Status or warning
    display.setCursor(0, 56);
    if (binLevel >= BIN_FULL_THRESHOLD) {
        display.print(">>> BIN FULL <<<");
    } else if (binLevel >= BIN_WARNING_THRESHOLD) {
        display.print(">> BIN WARNING <<");
    } else if (userDetected) {
        display.print("User: " + getUserProximityStatus());
    } else {
        display.print("Status: Active");
    }
    
    display.display();
}

// Function to read bin level
void readBinLevel() {
    digitalWrite(BIN_TRIG_PIN, HIGH);
    delayMicroseconds(20);
    digitalWrite(BIN_TRIG_PIN, LOW);
    binDuration = pulseIn(BIN_ECHO_PIN, HIGH);
    binDistance = binDuration * 0.034 / 2;
    binLevel = calculateBinLevel(binDistance);
    
    // Update bin status flags
    binFull = (binLevel >= BIN_FULL_THRESHOLD);
    binWarning = (binLevel >= BIN_WARNING_THRESHOLD);
    
    Serial.println("Bin Distance: " + String(binDistance) + " cm");
    Serial.println("Bin Level: " + String(binLevel) + "% (" + getBinStatus(binLevel) + ")");
}

// Function to handle bin full alerts
void handleBinAlerts() {
    if (binFull) {
        digitalWrite(BIN_FULL_LED, HIGH);
        Serial.println("*** BIN FULL ALERT - PLEASE EMPTY ***");
    } else if (binWarning) {
        // Blink warning LED
        static unsigned long lastBlink = 0;
        static bool ledState = false;
        if (millis() - lastBlink > 500) {
            ledState = !ledState;
            digitalWrite(BIN_FULL_LED, ledState);
            lastBlink = millis();
        }
    } else {
        digitalWrite(BIN_FULL_LED, LOW);
    }
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
    
    // Check bin level before processing waste
    readBinLevel();
    
    // Check user proximity
    readUserProximity();
    
    // If bin is full, don't process new waste
    if (binFull) {
        Serial.println("*** BIN FULL - CANNOT ACCEPT MORE WASTE ***");
        updateOLED("BIN FULL", 0, 0, 0, binLevel);
        return;
    }
    
    // Update NTP time
    timeClient.update();
    String timestamp = timeClient.getFormattedTime();
    
    count++;
    lastUserActivity = millis(); // Record user activity
    
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
    Serial.println("Methane Level: " + String(methane_ppm) + " PPM");
    Serial.println("User Distance: " + String(userDistance) + " cm");
    Serial.println("User Status: " + getUserProximityStatus());
    Serial.println("Timestamp: " + timestamp);
    
    // Prepare JSON for Firebase with bin level and proximity data
    json.clear();
    json.set("/bin_distance", binDistance);
    json.set("/bin_level_percent", binLevel);
    json.set("/bin_status", getBinStatus(binLevel));
    json.set("/bin_full", binFull);
    json.set("/bin_warning", binWarning);
    json.set("/user_distance", userDistance);
    json.set("/user_detected", userDetected);
    json.set("/user_close", userClose);
    json.set("/user_status", getUserProximityStatus());
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
    
    // Waste classification logic and display update
    String wasteType = "";
    
    if ((myData.b == 1) || (methane_ppm >= 500) || (humi >= 74.5)) {
        // BIODEGRADABLE WASTE (organic waste produces methane during decomposition)
        wasteType = "Biodegradable";
        Serial.println("Classification: Biodegradable Waste");
        digitalWrite(BIO_LED, HIGH);
        digitalWrite(NBIO_LED, LOW);
        
        // Update OLED display with bin level and user proximity
        updateOLED(wasteType, humi, tempC, methane_ppm, binLevel);
        
        // Move servos for biodegradable waste (only if bin not full)
        if (!binFull) {
            sg1.write(0);
            sg2.write(180);
            delay(5000); // Wait 5 seconds
            
            // Return to neutral position
            sg1.write(90);
            sg2.write(90);
        }
        digitalWrite(BIO_LED, LOW);
        
    } else if ((myData.b == 2) || (methane_ppm < 500) || (humi < 74.5)) {
        // NON-BIODEGRADABLE WASTE
        wasteType = "Non-Biodegradable";
        Serial.println("Classification: Non-Biodegradable Waste");
        digitalWrite(NBIO_LED, HIGH);
        digitalWrite(BIO_LED, LOW);
        
        // Update OLED display with bin level and user proximity
        updateOLED(wasteType, humi, tempC, methane_ppm, binLevel);
        
        // Move servos for non-biodegradable waste (only if bin not full)
        if (!binFull) {
            sg1.write(180);
            sg2.write(0);
            delay(5000); // Wait 5 seconds
            
            // Return to neutral position
            sg1.write(90);
            sg2.write(90);
        }
        digitalWrite(NBIO_LED, LOW);
        
    } else {
        wasteType = "Unknown";
        Serial.println("Unknown waste type - no classification");
        
        // Update OLED display with bin level and user proximity
        updateOLED(wasteType, humi, tempC, methane_ppm, binLevel);
        
        // Turn off both LEDs
        digitalWrite(BIO_LED, LOW);
        digitalWrite(NBIO_LED, LOW);
    }
    
    delay(100); // Small delay for servo stability
    Serial.println("----------------------------");
}

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 Smart Waste Segregation with Dual Ultrasonic Sensors Starting...");
    
    // Initialize DHT sensor
    dht_sensor.begin();
    
    // Initialize LED pins
    pinMode(BIO_LED, OUTPUT);
    pinMode(NBIO_LED, OUTPUT);
    pinMode(BIN_FULL_LED, OUTPUT);
    pinMode(USER_DETECTED_LED, OUTPUT);
    digitalWrite(BIO_LED, LOW);
    digitalWrite(NBIO_LED, LOW);
    digitalWrite(BIN_FULL_LED, LOW);
    digitalWrite(USER_DETECTED_LED, LOW);
    
    // Initialize ultrasonic sensor pins for bin level monitoring
    pinMode(BIN_TRIG_PIN, OUTPUT);
    pinMode(BIN_ECHO_PIN, INPUT);
    digitalWrite(BIN_TRIG_PIN, LOW);
    
    // Initialize ultrasonic sensor pins for user proximity detection
    pinMode(PROX_TRIG_PIN, OUTPUT);
    pinMode(PROX_ECHO_PIN, INPUT);
    digitalWrite(PROX_TRIG_PIN, LOW);
    
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
    display.println("Bin Monitor");
    display.println("+ User Detect");
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
    
    // Take initial readings
    readBinLevel();
    readUserProximity();
    
    // Display ready message on OLED with bin level
    updateOLED("READY", 0, 0, 0, binLevel);
    
    Serial.println("System ready - monitoring bin level and user proximity...");
    Serial.println("Bin Height: " + String(BIN_HEIGHT) + " cm");
    Serial.println("Full Threshold: " + String(BIN_FULL_THRESHOLD) + "%");
    Serial.println("Warning Threshold: " + String(BIN_WARNING_THRESHOLD) + "%");
    Serial.println("User Detection Distance: " + String(USER_DETECTION_DISTANCE) + " cm");
    Serial.println("User Close Distance: " + String(USER_CLOSE_DISTANCE) + " cm");
}

void loop() {
    // Continuously monitor bin level
    static unsigned long lastBinCheck = 0;
    if (millis() - lastBinCheck > 5000) { // Check bin level every 5 seconds
        readBinLevel();
        handleBinAlerts();
        lastBinCheck = millis();
    }
    
    // Continuously monitor user proximity
    if (millis() - lastProximityCheck > PROXIMITY_CHECK_INTERVAL) {
        readUserProximity();
        handleUserProximity();
        lastProximityCheck = millis();
    }
    
    // Update display if no recent activity (but user might be present)
    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 10000) { // Update display every 10 seconds
        if (millis() - lastUserActivity > 30000) { // 30 seconds of no waste activity
            updateOLED("READY", 0, 0, 0, binLevel);
        }
        lastDisplayUpdate = millis();
    }
    
    // Handle bin full alerts
    handleBinAlerts();
    
    // Handle user proximity alerts
    handleUserProximity();
    
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
