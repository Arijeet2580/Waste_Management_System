#define LED = 25;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(LED,OUTPUT);
}
/*5-200PPM   MQ137 GAS DETECTION VALUE*/
/* BIODEGRADABLE DETECTED NEAR TO 40PPM ----> 735ADC*/
void loop() {
  // put your main code here, to run repeatedly:
  float SV = analogRead(A0);
  int ppm = (195*(SV/4095))+5;
  Serial.print("ADC Val: ");
  Serial.println(SV);
  Serial.print("PPM:");
  Serial.println(ppm);
}
