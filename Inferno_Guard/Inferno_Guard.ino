#include <Arduino.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <ThingSpeak.h>
#include <DHTesp.h>
#include <base64.h>
#include <HTTPClient.h>

//Motor control pins Description
#define enableRightMotor  18  //Right motor enable pin
#define rightMotorPin1    19  //Right motor pin 1
#define rightMotorPin2    21  //Right motor pin 2
#define enableLeftMotor   5   //Left motor enable pin
#define leftMotorPin1     22  //Left motor pin 1
#define leftMotorPin2     23  //Left motor pin 2
#define MotorSpeed        50 // Adjust PWM speed (0-255 for analogWrite)

//Water Pump control pins Description
#define IN1 26
#define IN2 27

//Flame Sensor Pin Description
#define RightSensorPin 33
#define StraightSensorPin 32
#define LeftSensorPin 34

//Threshold Definition
#define NearThreshold 1500
#define FarThreshold 3800
int FlameDetectionStatus = 0;

// Servo Motor Description
Servo myservo;        // Create a servo object
int servoPin = 15; 

// DHT11 Sensor Description
DHTesp d;
TempAndHumidity data;
float Humidity, Temperature;

// Smoke Sensor Description
#define MQ2_PIN 35
int safeThreshold = 1000;
int moderateThreshold = 1500;
int poorThreshold = 2500;
int SmokeIntensity;

const char *ssid = "your SSID";
const char *password = "your";

WiFiServer server(80);
WiFiClient client;

long myChannelNumber = 1234567;  // your ThingSpeak Channel ID
const char* myWriteAPIKey = "write API key"; // your ThingSpeak Channel Write API Key

// Twilio credentials
const char* accountSid = " ";  // Your Twilio AccountSid
const char* authToken = "";  // Your Twilio Auth Token
const char* twilioPhoneNumber = " "; // Your Twilio phone number
const char* recipientPhoneNumber = " "; // The phone number to send the SMS to

bool flameAlertSent = false;
bool smokeAlertSent = false;

void setup() {
  Serial.begin(115200);

  //Motor Pin
  pinMode(rightMotorPin1, OUTPUT);
  pinMode(rightMotorPin2, OUTPUT);
  pinMode(leftMotorPin1, OUTPUT);
  pinMode(leftMotorPin2, OUTPUT);
  pinMode(enableRightMotor, OUTPUT);
  pinMode(enableLeftMotor, OUTPUT);
    
  //water pump pin
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  //Sensor Pin 
  pinMode(RightSensorPin, INPUT);
  pinMode(StraightSensorPin, INPUT);
  pinMode(LeftSensorPin, INPUT);

  myservo.attach(servoPin);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  ThingSpeak.begin(client); 

  d.setup(4, DHTesp::DHT11);

  pinMode(MQ2_PIN, INPUT);

  // Create FreeRTOS tasks
  xTaskCreatePinnedToCore(MainOperationTask, "MainOperation", 2048, NULL, 1, NULL, 1);  // Core 1
  xTaskCreatePinnedToCore(SendAlertMsgTask, "SendAlertMsg", 4096, NULL, 1, NULL, 0); // Core 0 Priority 1
  xTaskCreatePinnedToCore(SensorReadingTask, "SensorReading", 2048, NULL, 2, NULL, 0); // Core 0 Priority 2
  xTaskCreatePinnedToCore(ThingSpeakOperationTask, "ThingSpeakOperation", 2048, NULL, 3, NULL, 0); // Core 0, Priority 3
}

void MainOperationTask(void *pvParameters) {
  while (true) {
    MainOperation();
  }
}

void SendAlertMsgTask(void *pvParameters){
  while (true) {
    if (FlameDetectionStatus == 1 && !flameAlertSent){
      SendAlertMsg("Flame Detected! Activating fire-fighting protocol.");
      flameAlertSent = true;
    } else if (FlameDetectionStatus == 0) {
      flameAlertSent = false; // Reset when danger passes
    }

    if (SmokeIntensity == 2 && !smokeAlertSent){
      SendAlertMsg("Smoke Detected! Activating fire-fighting protocol.");
      smokeAlertSent = true;
    } else if (SmokeIntensity < 2) {
      smokeAlertSent = false; // Reset on safe levels
    }

    vTaskDelay(5000 / portTICK_PERIOD_MS); // 5 sec delay for conservative checking
  }
}


void SensorReadingTask(void *pvParameters){
  while (true) {
    SensorReading();
    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Update ThingSpeak every 15 seconds (adjust as needed)
  }
}

void ThingSpeakOperationTask(void *pvParameters) {
  while (true) {
    sendDataToThingSpeak(Humidity, Temperature, SmokeIntensity);
    vTaskDelay(5000 / portTICK_PERIOD_MS);  // Update ThingSpeak every 15 seconds (adjust as needed)
  }
}

void MainOperation() {

  int RSen = analogRead(RightSensorPin);
  int SSen = analogRead(StraightSensorPin);
  int LSen = analogRead(LeftSensorPin);

  if (RSen < NearThreshold){
    FlameDetectionStatus = 1;
    TurnOnWaterPump();
    sweepRight();
    TurnOffWaterPump();
  }
  else if (SSen < NearThreshold){
    FlameDetectionStatus = 1;
    TurnOnWaterPump();
    sweepFront();
    TurnOffWaterPump();
  }
  else if (LSen < NearThreshold){
    FlameDetectionStatus = 1;
    TurnOnWaterPump();
    sweepLeft();
    TurnOffWaterPump();
  }
  else if (NearThreshold < RSen && RSen  < FarThreshold){
    FlameDetectionStatus = 1;
    MoveBackward();
    delay(200);
    MoveRight();
    delay(500);
    StopMoving();
  }
  else if (NearThreshold < SSen && SSen < FarThreshold){
    FlameDetectionStatus = 1;
    MoveForward();
    delay(500);
    StopMoving();
  }
  else if (NearThreshold < LSen && LSen < 3500){
    FlameDetectionStatus = 1;
    MoveBackward();
    delay(200);
    MoveLeft();
    delay(500);
    StopMoving();
  }
  else {
    FlameDetectionStatus = 0;
    StopMoving();
  }
}


void SendAlertMsg(String message) {
  HTTPClient http;

  // Construct the Twilio API URL
  String url = "https://api.twilio.com/2010-04-01/Accounts/" + String(accountSid) + "/Messages.json";

  // Set the necessary headers for authentication
  String auth = String(accountSid) + ":" + String(authToken);
  String base64Auth = base64::encode(auth.c_str());

  // Prepare the POST data
  String postData = "To=" + String(recipientPhoneNumber) + "&From=" + String(twilioPhoneNumber) + "&Body=" + message;

  // Set up the HTTP request
  http.begin(url);
  http.addHeader("Authorization", "Basic " + base64Auth);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // Send the POST request
  int httpResponseCode = http.POST(postData);

  if (httpResponseCode > 0) {
    Serial.println("SMS sent successfully!");
    Serial.println("HTTP Response code: " + String(httpResponseCode));
  } else {
    Serial.println("Error sending SMS. HTTP Response code: " + String(httpResponseCode));
  }

  // Close the HTTP connection
  http.end();
}

void SensorReading() {

  data = d.getTempAndHumidity();
  Humidity = float(data.humidity);
  Temperature = float(data.temperature);

  int mq2Value = analogRead(MQ2_PIN);  // Read analog value from MQ-2 sensor

  // Display the raw value for reference
  Serial.print("MQ-2 Sensor Reading: ");
  Serial.println(mq2Value);

  // Determine air quality based on thresholds
  if (mq2Value < safeThreshold) {
    SmokeIntensity = 0;
    Serial.println("Air Quality: Safe");
  } else if (mq2Value < moderateThreshold) {
    SmokeIntensity = 1;
    Serial.println("Air Quality: Moderate");
  } else {
    SmokeIntensity = 2;
    Serial.println("Air Quality: Dangerous! Immediate action required!");
  }
}

void sendDataToThingSpeak(float Humidity, float Temperature, int SmokeIntensity ) {
  ThingSpeak.setField(1, Humidity);
  ThingSpeak.setField(2, Temperature);
  ThingSpeak.setField(3, SmokeIntensity);

  int responseCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (responseCode == 200) {
      Serial.println(" Data sent to ThingSpeak successfully.");
  } else {
      Serial.print("Error sending data to ThingSpeak. Response code: ");
      Serial.println(responseCode);
  }
}

void MoveForward() {
  digitalWrite(rightMotorPin1, HIGH);
  digitalWrite(rightMotorPin2, LOW);
  digitalWrite(leftMotorPin1, HIGH);
  digitalWrite(leftMotorPin2, LOW);
  analogWrite(enableRightMotor, MotorSpeed);
  analogWrite(enableLeftMotor, MotorSpeed); 
}

void MoveLeft() {
  digitalWrite(rightMotorPin1, HIGH);
  digitalWrite(rightMotorPin2, LOW);
  digitalWrite(leftMotorPin1, LOW);
  digitalWrite(leftMotorPin2, LOW);
  analogWrite(enableRightMotor, MotorSpeed);
  analogWrite(enableLeftMotor, 0);
}

void MoveRight() {
  digitalWrite(rightMotorPin1, LOW);
  digitalWrite(rightMotorPin2, LOW);
  digitalWrite(leftMotorPin1, HIGH);
  digitalWrite(leftMotorPin2, LOW);
  analogWrite(enableRightMotor, 0);
  analogWrite(enableLeftMotor, MotorSpeed);
}

void MoveBackward() {
  digitalWrite(rightMotorPin1, LOW);
  digitalWrite(rightMotorPin2, HIGH);
  digitalWrite(leftMotorPin1, LOW);
  digitalWrite(leftMotorPin2, HIGH);
  analogWrite(enableRightMotor, MotorSpeed);
  analogWrite(enableLeftMotor, MotorSpeed);
}

void StopMoving() {
  digitalWrite(rightMotorPin1, LOW);
  digitalWrite(rightMotorPin2, LOW);
  digitalWrite(leftMotorPin1, LOW);
  digitalWrite(leftMotorPin2, LOW);
  analogWrite(enableRightMotor, 0);
  analogWrite(enableLeftMotor, 0);
}

void TurnOnWaterPump() {
  // Turn ON pump
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

void TurnOffWaterPump() { 
  // Turn OFF pump
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}

void sweepRight() {
  for (int repeat = 0; repeat < 2; repeat++) {
    // Sweep from 90° to 0° smoothly
    for (int pos = 90; pos >= 0; pos--) {  
      myservo.write(pos);
      delay(5); // Smooth movement
    }
    delay(200); // Pause at 0° before moving to 90°

    // Sweep from 0° to 90° smoothly
    for (int pos = 0; pos <= 90; pos++) {  
      myservo.write(pos);
      delay(5); // Smooth movement
    }
    delay(200); // Pause at 90° before the next cycle
  }
}

void sweepLeft() {
  for (int repeat = 0; repeat < 2; repeat++) {
    // Sweep from 90° to 180° smoothly
    for (int pos = 90; pos <= 180; pos++) {  
      myservo.write(pos);
      delay(5); // Smooth movement
    }
    delay(200); // Pause at 180° before moving to 90°

    // Sweep from 180° to 90° smoothly
    for (int pos = 180; pos >= 90; pos--) {  
      myservo.write(pos);
      delay(5); // Smooth movement
    }
    delay(200); // Pause at 90° before the next cycle
  }
}

void sweepFront() {
  for (int repeat = 0; repeat < 1; repeat++) {
    // Sweep from 90° to 30° smoothly
    for (int pos = 90; pos >= 30; pos--) {  
      myservo.write(pos);
      delay(5); // Smooth movement
    }
    delay(200); // Pause at 30° before moving to 120°

    // Sweep from 30° to 120° smoothly
    for (int pos = 30; pos <= 120; pos++) {  
      myservo.write(pos);
      delay(5); // Smooth movement
    }
    delay(200); // Pause at 120° before returning to 30°

    // Sweep from 120° to 30° smoothly
    for (int pos = 120; pos >= 30; pos--) {  
      myservo.write(pos);
      delay(5); // Smooth movement
    }
    delay(200); // Pause at 30° before moving to 120° again

    // Sweep from 30° to 120° smoothly again
    for (int pos = 30; pos <= 120; pos++) {  
      myservo.write(pos);
      delay(5); // Smooth movement
    }
    delay(200); // Pause before next cycle
  }
}

void loop() {
  // Not used, required by Arduino core
}

