# InfernoGuard: Autonomous Firefighting Robot 

InfernoGuard is an ESP32-based autonomous firefighting robot designed to detect flames and hazardous smoke, extinguish fires, and send real-time alerts via SMS using the Twilio API. Sensor data such as temperature, humidity, and smoke levels are also uploaded to ThingSpeak for remote monitoring.

##  Features

-  Flame detection using IR sensors (Right, Left, Center)
-  Smoke level detection using MQ-2
-  DHT11 for temperature & humidity
-  Real-time alert system using Twilio SMS API
-  WiFi-enabled data logging to ThingSpeak
-  Servo-based sweeping water sprayer
-  Autonomous decision-making via FreeRTOS tasks

##  Hardware Used

- ESP32 Dev Board
- DHT11 Sensor
- MQ-2 Gas Sensor
- 3 x IR Flame Sensors
- L298N Motor Driver
- 4 x Motors (2 Left + 2 Right)
- Water Pump
- Servo Motor
- 12V Battery

##  Cloud Integration

- **ThingSpeak**: Real-time dashboard for temperature, humidity, and smoke levels
- **Twilio**: Sends SMS alerts during emergencies

##  FreeRTOS Tasks

- MainOperationTask: Flame detection and robot movement
- SendAlertMsgTask: Sends alerts using Twilio API
- SensorReadingTask: Reads from DHT11 and MQ2
- ThingSpeakOperationTask: Uploads to ThingSpeak

##  Future Improvements

- Integrate camera + AI for visual flame recognition
- Add GPS to log fire location
- Add LoRa/mesh for remote areas
- Use water-level sensor to alert low reservoir
