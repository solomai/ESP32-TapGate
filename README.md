# 🔐 TapGate

**TapGate** is a standalone extension module for gate automation systems that adds the ability to control gate status via a smartphone button.

### Key Features

- **Local control via Bluetooth**  
  Control the gate automation system from a smartphone at short range, without requiring an internet connection.

- **Remote control via Wi-Fi/Internet**  
  Operate the system over the internet using MQTT (supports both sequred anonymous and authenticated access) when Bluetooth is not available.

- **Home Assistant integration**  
  Integration with Home Assistant is possible via a trusted MQTT broker, either over the internet or within a local network.

### Key Benefits

- Uses Bluetooth for local control and fallback operation when there is no internet connection — no need to connect to an access point.
- Supports anonymous MQTT servers when no external server is available.
- Encrypts and validates data sent over anonymous MQTT for added security.
- Enables simple and reliable remote control over the internet when available.
- Compatible with gate automation and entry systems that use physical button control


## 📜 License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## 📜 Disclaimer
This component is provided "as is", without warranty of any kind. The authors are not liable for any damages arising from the use of this component. Use it at your own risk.

## ✍️ Author
Andrii Solomai