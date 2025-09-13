# ESP-WROOM-32 Target Specification

## 1. General Information
- Manufacturer: Espressif Systems  
- Module Type: Wi-Fi + Bluetooth Combo Module  
- MCU: ESP32-D0WDQ6, dual-core Xtensa® 32-bit LX6  
- Clock Frequency: up to 240 MHz  
- Operating Voltage: 2.2 V – 3.6 V (typical 3.3 V)  
- Flash Memory: 4 MB (external SPI flash, variants up to 16 MB available)  
- SRAM ~520 KB  
- Operating Temperature Range: –40 °C to +85 °C  
- Module Dimensions: 18 mm × 25.5 mm × 3.1 mm  

---

## 2. Wireless Connectivity
### Wi-Fi
- IEEE 802.11 b/g/n  
- Frequency range: 2.4–2.5 GHz  
- Data rate: up to 150 Mbps  
- Security: WEP, WPA/WPA2/WPA3, enterprise support  

### Bluetooth
- Version: v4.2 BR/EDR and BLE  
- Profiles: GATT, GAP, SPP  

---

## 3. Peripherals & Interfaces
- GPIO: up to 34 programmable pins  
- ADC: up to 18 channels, 12-bit resolution  
- DAC: 2 channels, 8-bit  
- UART: 3  
- SPI: 4  
- I²C: 2  
- I²S: 2 (audio support)  
- CAN: 1 (via TWAI controller)  
- PWM: up to 16 channels  
- Touch Sensors: up to 10 capacitive touch GPIOs  
- RMT: Remote Control peripheral for IR or custom signal generation  
- Timers: multiple general-purpose 64-bit timers  
- RTC: Real-Time Clock with Deep Sleep/ULP coprocessor  

---

## 4. Power Management
- Active Mode Consumption: ~160–260 mA (Wi-Fi TX)  
- Light Sleep: ~0.8 mA  
- Deep Sleep: ~10 µA  
- Hibernate Mode: ~5 µA  

---

## 5. Security
- Hardware AES, SHA, RSA, ECC, RNG accelerators  
- Secure Boot support  
- Flash Encryption support  

[← Back to ESP32 MCU Documentation](../esp32-mcu.md)  
[← Back to main README](../../README.md)