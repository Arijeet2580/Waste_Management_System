# Waste Management System
## Overview

This project implements a smart waste management system utilizing ESP-32 Cam modules to detect anomalies in waste segregation. The system identifies incorrect waste placement (e.g., non-biodegradable waste in biodegradable bins) with an 80% probability and communicates this information to a central ESP-32 Main Board. The Main Board, upon receiving the data, makes decisions based on the sensor inputs and actuates servo motors to open the appropriate waste bin lid

## Project Structure

1. **Sender_main**: 
   - Deployed on the ESP-32 Cam module.
   - Detects specific objects within its vicinity with an 80% probability threshold.
   - Sends detected data to the ESP-32 Main Board for further processing.

2. **MAC_Address**: 
   - Helps in identifying the MAC address of the Main Board.
   - The MAC address is used in the `Sender_main` code to ensure data is sent to the correct Main Board.

3. **Receiver_main**:
   - Deployed on the ESP-32 Main Board.
   - Receives data points from the ESP-32 Cam module.
   - Makes decisions based on received data and sensor inputs.
   - Controls servo motors to open the corresponding waste bin lid.

## Setup Instructions

### Hardware Requirements

- ESP-32 Cam Module
- ESP-32 Main Board
- Servo Motors
- Various sensors (e.g., proximity sensors)
- Power supply and connectors

### Software Requirements

- Arduino IDE
- Required libraries for ESP-32 and camera module

### Steps to Configure

1. **ESP-32 Cam Module Setup**:
   - Connect the ESP-32 Cam module to your computer.
   - Open `Sender_main` code in Arduino IDE.
   - Ensure the correct board and port are selected.
   - Upload the code to the ESP-32 Cam module.

2. **Identify MAC Address**:
   - Open `MAC_Address` code in Arduino IDE.
   - Upload the code to the ESP-32 Main Board.
   - Open the Serial Monitor to find and note down the MAC address of the Main Board.

### Example
   1. The MAC Address used by us are C0:49:EF:D2:B7:E8
   2. Update the Broadcast Address as 0xFFC0
   3. The Broadcast Address array will be
      **0xFFC0, 0xFF49, 0xFFEF, 0xFFD2, 0xFFB7, 0xFFE8**

3. **Configure Sender_main**:
   - Update the `Sender_main` code with the MAC address noted from the previous step.
   - Upload the updated `Sender_main` code to the ESP-32 Cam module.

4. **ESP-32 Main Board Setup**:
   - Connect the ESP-32 Main Board to your computer.
   - Open `Receiver_main` code in Arduino IDE.
   - Ensure the correct board and port are selected.
   - Upload the code to the ESP-32 Main Board.

5. **Assemble the System**:
   - Connect the servo motors to the ESP-32 Main Board.
   - Connect any additional sensors to the ESP-32 Main Board.
   - Power up the system and ensure all connections are secure.

## Usage

Once the system is set up and powered on:

1. The ESP-32 Cam module will continuously monitor the vicinity for specific objects.
2. Upon detecting an object with an 80% probability, it will send data to the ESP-32 Main Board.
3. The ESP-32 Main Board receives the data, processes it along with sensor inputs, and decides whether to open the waste bin lid.
4. If an anomaly is detected (e.g., non-biodegradable waste in a biodegradable bin), the corresponding servo motor will be actuated to open the lid.

## Future Enhancements

- Improve object detection accuracy and extend the types of detectable waste.
- Integrate additional sensors for more robust decision-making.
- Implement a mobile app for real-time monitoring and control of the system.

---

Feel free to reach out for any queries or contributions to the project.
