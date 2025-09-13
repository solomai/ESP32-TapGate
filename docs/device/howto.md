## How to Build

To build this project you need **ESP-IDF 5.3 (or higher)**.  

### Install ESP-IDF
1. Follow the official installation guide: [ESP-IDF Installation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html)  
2. Recommended: use the **ESP-IDF Tools Installer** for Windows, which sets up Python, CMake, Ninja, Git, and environment variables automatically.  

### Build Instructions
- Open **ESP-IDF 5.3 PowerShell** (installed with ESP-IDF).  
  This ensures all required environment variables are configured.  
- From there, you can also start your favorite IDE (e.g., VS Code, CLion).  

```bash
# Navigate to your project folder
cd path\to\esp32-tapgate

# Clean, build, flash, and monitor
idf.py build flash monitor
```

[← Back to ESP32 MCU Documentation](../esp32-mcu.md)  
[← Back to main README](../../README.md)