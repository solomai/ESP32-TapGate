# 🔐 TapGate v.1 ( prototype )

**TapGate** is a standalone extension module for gate automation systems that adds the ability to control gate via a smartphone application.

### Goal
Develop a hardware - software system (IoT + mobile app) for remote control of standard electronic gate controllers.

### Device Interaction
A single IoT device can be controlled by multiple instances of the mobile app:

```bash
             / 1 mobile app 
 IoT (ESP32) - 2 mobile app
             \ n mobile app
```

### Prototype Scope
- Choose a technology stack for rapid development of working code.
- Design a communication protocol between devices (registration, connection, command transmission, state notifications).
- Develop software modules for communication over Bluetooth and over the Internet as reusable components (submodules) for future versions.


### Key Features

- **Local control via Bluetooth**  
  Control the gate automation system from a smartphone at short range, without requiring an internet connection.

- **Remote control via Wi-Fi/Internet**  
  Operate the system over the internet using MQTT (supports both sequred anonymous and authenticated access) when Bluetooth is not available.
  Supports anonymous MQTT servers when no external server is available.
  Encrypts and validates data sent over anonymous MQTT for added security.

- **Home Assistant integration**  
  Integration with Home Assistant is possible via a trusted MQTT broker, either over the internet or within a local network.

### Communication: Parallel Use of Bluetooth and MQTT channels

Communication between the mobile client and the IoT device is implemented simultaneously across all available channels: Bluetooth (BLE) and MQTT (over Wi-Fi/Internet). This approach was chosen because it minimizes latency, the user always gets a response from the fastest channel, and ensures maximum reliability in scenarios where a connection may drop at any moment.<br>
At the same time, the system preserves the ability to control the device locally via BLE even without an Internet connection. Combining both channels provides flexibility across different usage scenarios - from local offline control to global access via MQTT-making the system universal and resilient to connection loss, while parallel operation reduces delays when establishing connections or when one of the channels is unstable.

## 📜 License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## 📜 Disclaimer
This component is provided "as is", without warranty of any kind. The authors are not liable for any damages arising from the use of this component. Use it at your own risk.

## ✍️ Author
Andrii Solomai