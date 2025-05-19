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

##  Cloud Integration

- **ThingSpeak**: Real-time dashboard for temperature, humidity, and smoke levels
- **Twilio**: Sends SMS alerts during emergencies

##  FreeRTOS Tasks

- MainOperationTask: Flame detection and robot movement
- SendAlertMsgTask: Sends alerts using Twilio API
- SensorReadingTask: Reads from DHT11 and MQ2
- ThingSpeakOperationTask: Uploads to ThingSpeak

##  Why FreeRTOS? (Special Mention)

In real-time systems, operations like cloud data logging and alert message transmission can introduce **delays due to WiFi or HTTP request latency**. During these moments, if a fire is detected, the robot must **not wait** — it must respond immediately.

To solve this, I utilized the **dual-core capability of the ESP32** using **FreeRTOS**:

-  **Core 1 handles MainOperationTask**: flame detection, robot motion, and fire extinguishing.
-  **Core 0 handles Secondary Tasks**: cloud updates and alert messages.

This ensures:

- **No blocking**
- **True parallel execution**
- **Immediate firefighting even during background operations**
With FreeRTOS, InfernoGuard becomes a **real-time multi-tasking robot**, making it both **smart and fast** under critical conditions.

##  Future Improvements

- Integrate camera + AI for visual flame recognition
- Add GPS to log fire location
- Add LoRa/mesh for remote areas
- Use water-level sensor to alert low reservoir

---
