# Patient Monitoring System with ESP32 and Arduino Uno

## Project Overview

This project develops a sophisticated patient monitoring system that collects and displays critical health metrics in real-time, leveraging an ESP32 microcontroller as the primary controller and an Arduino Uno for local visualization. The system monitors heart rate (BPM), blood oxygen saturation (SpO2), electrocardiogram (ECG), temperature, humidity, and weight using advanced sensors. It offers a web-based dashboard for interactive data visualization, email notifications for critical health conditions, and cloud-based data logging via ThingSpeak for long-term analysis. The Arduino Uno interfaces with a 128x64 SSD1306 OLED display to show real-time sensor data, including invalid data for debugging purposes, received via serial communication from the ESP32. Designed for reliability and ease of use, this system is suitable for home health monitoring, remote patient care, clinical settings, research, and educational applications.

### Key Features

| Feature                     | Description                                                                 |
|-----------------------------|-----------------------------------------------------------------------------|
| **Real-Time Monitoring**    | Collects data from MAX30102, AD8232, DHT11, and HX711 sensors with high accuracy. |
| **Web Interface**           | Displays interactive graphs for ECG, heart rate, and weight, updated every second. |
| **Email Alerts**            | Sends notifications for abnormal SpO2 (< 95%), heart rate (< 60 or > 100 BPM), or temperature (> 37.5°C), with a 15-second debounce. |
| **ThingSpeak Integration**  | Logs data every 15 seconds for remote access and analysis.                   |
| **Local OLED Display**      | Arduino Uno with SSD1306 OLED shows valid and invalid data for real-time feedback. |
| **Buzzer Alerts**           | Activates for low weight conditions (< 100g).                                |
| **Robust Error Handling**   | Manages invalid serial data and communication issues with detailed logging.  |

## Hardware Requirements

### Components

| Component                   | Specifications                                                                 | Quantity |
|-----------------------------|-------------------------------------------------------------------------------|----------|
| ESP32 DevKit                | Main microcontroller (e.g., DOIT ESP32 DevKit V1, 38-pin)                     | 1        |
| Arduino Uno R3              | For OLED display                                                              | 1        |
| MAX30102 Breakout Board     | Measures heart rate and SpO2 (I2C interface)                                  | 1        |
| AD8232 Breakout Board       | Captures ECG signals (analog interface)                                       | 1        |
| DHT11 Sensor                | Monitors temperature and humidity (digital interface)                         | 1        |
| HX711 Load Cell Amplifier   | Measures weight with a load cell (digital interface, e.g., 5kg load cell)     | 1        |
| SSD1306 OLED (128x64)       | I2C interface, connected to Arduino Uno                                       | 1        |
| Buzzer                      | Active or passive, for low weight alerts                                      | 1        |
| Jumper Wires                | Male-to-male for breadboard connections                                      | ~30      |
| Breadboard or PCB           | For prototyping                                                               | 1        |
| 4.7kΩ Resistor              | Pull-up for DHT11 DATA pin                                                    | 1        |
| 100Ω Resistor               | Optional for buzzer current limiting                                          | 1        |
| Power Supply                | USB cables for ESP32 and Arduino Uno (5V, ~500mA total) or 5V 1A external    | 2        |

### Current Draw Estimates

| Component       | Approx. Current Draw |
|-----------------|----------------------|
| ESP32 DevKit    | ~200mA (with WiFi)   |
| Arduino Uno     | ~50mA                |
| MAX30102        | ~10mA                |
| AD8232          | ~3mA                 |
| DHT11           | ~2.5mA               |
| HX711           | ~1.5mA               |
| SSD1306 OLED    | ~20mA                |
| Buzzer (Active) | ~10-30mA             |
| **Total**       | ~300-500mA           |

### Circuit Diagram

Below is a textual schematic of the connections. 

#### ESP32 Connections

| Pin/Port       | Connection                              |
|----------------|-----------------------------------------|
| 3.3V           | MAX30102 VCC, AD8232 VCC                |
| 5V             | DHT11 VCC, HX711 VCC                    |
| GND            | Common GND (Uno, sensors, OLED, buzzer) |
| GPIO 21 (SDA)  | MAX30102 SDA                            |
| GPIO 22 (SCL)  | MAX30102 SCL                            |
| GPIO 36        | AD8232 OUTPUT                           |
| GPIO 34        | AD8232 LO+                              |
| GPIO 35        | AD8232 LO-                              |
| GPIO 4         | DHT11 DATA (with 4.7kΩ pull-up to 5V)   |
| GPIO 12        | HX711 DT                                |
| GPIO 13        | HX711 SCK                               |
| GPIO 5         | Buzzer Positive (Negative to GND or via 100Ω resistor) |
| GPIO 17 (TX)   | Arduino Uno RX (pin 0)                  |

- **AD8232 Electrodes**: Connect to RA (Right Arm), LA (Left Arm), RL (Right Leg) on the body.
- **HX711 Load Cell**: Connect to E+, E-, A+, A- (match wire colors per datasheet).

#### Arduino Uno Connections

| Pin/Port       | Connection                              |
|----------------|-----------------------------------------|
| 5V             | SSD1306 VCC                             |
| GND            | Common GND (ESP32, OLED)                |
| A4 (SDA)       | SSD1306 SDA                             |
| A5 (SCL)       | SSD1306 SCL                             |
| Pin 0 (RX)     | ESP32 TX (GPIO 17)                      |

#### Power and Ground

- **Power Supply**:
  - ESP32: USB or 5V to VIN (~500mA capacity).
  - Arduino Uno: USB or 7-12V barrel jack.
- **Common Ground**: Essential for stable communication; connect all GND pins.
- **Decoupling**: Add 0.1µF ceramic capacitors across VCC/GND of each sensor to minimize noise.

#### Wiring Best Practices
- Use short wires (<10cm) for serial (ESP32 TX to Uno RX) to reduce noise.
- Employ shielded cables for ECG electrodes and serial connections.
- Secure connections with soldered headers or firm breadboard contacts.
- Verify continuity with a multimeter for TX-RX and GND.
- Optionally, add a 1kΩ pull-down resistor on Uno RX to mitigate noise.

## Software Requirements

### Arduino IDE

- **Version**: Arduino IDE 1.8.x or 2.x ([Arduino IDE Download](https://www.arduino.cc/en/software)).
- **ESP32 Board Support**:
  - Add `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` to **File > Preferences > Additional Boards Manager URLs**.
  - Install “esp32” package via **Tools > Board > Boards Manager** (version 3.2.0 recommended).

### Libraries

#### ESP32 Libraries

| Library Name       | Description                                    | Source/Installation Link                                      |
|--------------------|------------------------------------------------|---------------------------------------------------------------|
| `WiFi`            | Built-in for WiFi connectivity                 | Included with ESP32 core                                      |
| `ThingSpeak`      | ThingSpeak API integration                     | [ThingSpeak Library](https://github.com/mathworks/thingspeak-arduino) |
| `ESP_Mail_Client` | Email alerts via SMTP                          | [ESP Mail Client](https://github.com/mobizt/ESP-Mail-Client)  |
| `MAX30105`        | MAX30102 sensor interface                      | [MAX30105 Library](https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library) |
| `heartRate`       | Heart rate calculations (with MAX30105)        | Included with MAX30105 library                                |
| `DHT`             | DHT11 sensor interface                         | [DHT Sensor Library](https://github.com/adafruit/DHT-sensor-library) |
| `HX711`           | HX711 load cell interface                      | [HX711 Library](https://github.com/bogde/HX711)               |
| `WebServer`       | Web server for dashboard                       | Included with ESP32 core                                      |
| `time`            | NTP time synchronization                       | Included with ESP32 core                                      |

#### Arduino Uno Libraries

| Library Name       | Description                                    | Source/Installation Link                                      |
|--------------------|------------------------------------------------|---------------------------------------------------------------|
| `Wire`            | Built-in for I2C communication                 | Included with Arduino core                                    |
| `Adafruit_GFX`    | Graphics library for OLED                      | [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library) |
| `Adafruit_SSD1306`| SSD1306 OLED display driver                    | [Adafruit SSD1306 Library](https://github.com/adafruit/Adafruit_SSD1306) |

- **Installation**: Use **Sketch > Include Library > Manage Libraries** in Arduino IDE to install libraries.

### Online Services

| Service       | Purpose                                      | Setup Instructions                                            |
|---------------|----------------------------------------------|---------------------------------------------------------------|
| **ThingSpeak**| Cloud-based data logging                     | Create account at [ThingSpeak](https://thingspeak.com/), set up a channel, note Channel ID and Write API Key. |
| **Gmail**     | Email alerts via SMTP                        | Enable 2FA, generate App Password at [Gmail Security](https://support.google.com/accounts/answer/185833), use `smtp.gmail.com`, port 465 (SSL). |

## Setup Instructions

### 1. Hardware Assembly

1. **Connect Sensors to ESP32**:
   - Follow the circuit diagram for MAX30102, AD8232, DHT11, HX711, and buzzer connections.
   - Ensure 4.7kΩ pull-up resistor for DHT11 DATA pin.
   - Secure ECG electrodes to RA, LA, RL positions on the body.
   - Calibrate HX711 load cell with known weights.

2. **Connect OLED to Arduino Uno**:
   - Wire SSD1306 OLED to Uno (VCC: 5V, GND, SDA: A4, SCL: A5).
   - Verify I2C address (`0x3C`) using an I2C scanner sketch if needed.

3. **Serial Communication**:
   - Connect ESP32 TX (GPIO 17) to Uno RX (pin 0).
   - Ensure common GND between ESP32 and Uno.

4. **Power Supply**:
   - Power ESP32 and Uno via USB or external 5V supply.
   - Add 0.1µF decoupling capacitors for sensors.
   - Verify sufficient current (~500mA total).

5. **Best Practices**:
   - Use short wires (<10cm) for serial and I2C connections.
   - Employ shielded cables for ECG electrodes.
   - Test continuity with a multimeter for critical connections (TX-RX, GND).

### 2. Software Configuration

1. **Install Arduino IDE**:
   - Download and install Arduino IDE 1.8.x or 2.x from [Arduino Software](https://www.arduino.cc/en/software).
   - Configure ESP32 board support as described above.

2. **Install Libraries**:
   - Open Arduino IDE, go to **Sketch > Include Library > Manage Libraries**.
   - Search and install all listed libraries for ESP32 and Uno.

3. **Configure ESP32 Code**:
   - Open `ucep_patient_esp.ino` in Arduino IDE.
   - Update the following in the code:
     - `ssid`: Your WiFi network name.
     - `password`: Your WiFi password.
     - `channelID`: Your ThingSpeak Channel ID.
     - `writeAPIKey`: Your ThingSpeak Write API Key.
     - `SENDER_EMAIL`: Your Gmail address.
     - `SENDER_APP_PASSWORD`: Your Gmail App Password.
     - `RECIPIENT_EMAIL`: Recipient’s email.

4. **Arduino Uno Code**:
   - Open `PatientMonitorUno.ino` in Arduino IDE.
   - No configuration changes are required.

### 3. Code Upload

| Device       | Steps                                                                 |
|--------------|----------------------------------------------------------------------|
| **ESP32**    | Select **ESP32 Dev Module**, set **Partition Scheme** to “Huge APP (3MB No OTA/1MB SPIFFS)”. Disconnect Uno RX (pin 0), upload, reconnect RX. |
| **Arduino Uno** | Select **Arduino Uno**. Disconnect RX (pin 0), upload, reconnect RX. |

- **Upload Process**:
  1. Open ESP32 code in Arduino IDE, select **Tools > Board > ESP32 Dev Module**.
  2. Set **Tools > Partition Scheme > Huge APP (3MB No OTA/1MB SPIFFS)** to accommodate code size.
  3. Disconnect Uno RX (pin 0) from ESP32 TX (GPIO 17) to avoid upload conflicts.
  4. Upload ESP32 code, then reconnect RX.
  5. Open Uno code, select **Tools > Board > Arduino Uno**.
  6. Disconnect RX again, upload Uno code, reconnect RX.
- **Upload Speed**: Both devices use 115200 baud for serial communication and programming.

### 4. System Verification

1. **Serial Monitor**:
   - Open Serial Monitor (115200 baud) for both devices in separate Arduino IDE instances.
   - **ESP32 Output**:
     - Expect: `WiFi connected! IP address: 192.168.x.x`, `Sent to Arduino: 75,98,36.5,45,2048,500,System OK`.
     - Sensor readings: `Heart rate: 75`, `SpO2: 98.5`, `Temp: 36.5`, `ECG reading: 2048`, `Weight: 500`.
   - **Uno Output**:
     - Valid data: `Received data: 75,98,36.5,45,2048,500,System OK`, `Parsed: 75 BPM, 98%, 36.5°C, 45%, 2048, 500g, System OK`.
     - Invalid data: `Received data: 75,98,36.5,45,2048`, `Invalid format: comma count = 4`, `Parse failed`.

2. **Web Interface**:
   - Note ESP32’s IP address from Serial Monitor (e.g., `192.168.x.x`).
   - Access `http://192.168.x.x` in a browser (Chrome/Firefox recommended).
   - Verify graphs (ECG: red, Pulse: blue, Weight: purple) update every second.
   - Check sensor values (BPM, SpO2, Temp, Humidity, ECG, Weight) and uptime.

3. **OLED Display**:
   - Uno’s OLED should show:
     - Valid data: `BPM:75 SpO2:98% T:36.5C H:45% ECG:2048 W:500g System OK`.
     - Invalid data: `Invalid Data Received 75,98,36.5,45,2048 Wrong comma count`.
   - Updates every ~1 second for valid data or when invalid data is received.

4. **Email Alerts**:
   - Simulate alerts:
     - SpO2 < 95%: Remove finger from MAX30102.
     - BPM < 60 or > 100: Modify `beatAvg` (e.g., `beatAvg = 55` in `loop()`).
     - Temp > 37.5°C: Heat DHT11 or set `dhtTemp = 38.0`.
   - Check Serial Monitor for `Email sent successfully` and inbox (`sm.abdulhasib.bd@gmail.com`).
   - Emails sent every 15 seconds at most.

## System Operation

### Data Flow and Processing

| Stage                  | Description                                                                 |
|------------------------|-----------------------------------------------------------------------------|
| **Sensor Data Collection** | MAX30102 (10ms), AD8232 (10ms), DHT11 (2s), HX711 (1s) read sensor data.   |
| **Data Validation**    | ESP32 validates data (e.g., `irValue` > 50000 for SpO2, lead-off for ECG).  |
| **Alert Processing**   | Checks SpO2 (< 95%), BPM (< 60 or > 100), Temp (> 37.5°C), Weight (< 100g). |
| **Serial Transmission**| ESP32 sends data to Uno every 1s: `<bpm>,<spo2>,<temp>,<humidity>,<ecg>,<weight>,<statusMessage>`. |
| **OLED Display**       | Uno parses data, displays valid or invalid data with error reason.          |
| **Web Interface**      | ESP32 hosts dashboard, updates graphs/values every 1s via `/data` endpoint. |
| **ThingSpeak Logging** | Uploads data every 15s (BPM, SpO2, Temp, Humidity, ECG, Weight).           |

- **Sensor Details**:
  - **MAX30102**: Calculates BPM via `checkHeartRate()` and SpO2 via `calculateSpO2()`. Invalid if `irValue` < 50000.
  - **AD8232**: Reads ECG voltage (0-4095), sets `ecgValue = 0` if leads are off (`LO+` or `LO-` HIGH).
  - **DHT11**: Reads temperature and humidity, sets to 0 if `isnan()`.
  - **HX711**: Measures weight, calibrated with `CALIBRATION_FACTOR` (-478.507), absolute value taken.
- **Alert Mechanisms**:
  - **Emails**: Triggered for SpO2 < 95% (≥ 5), BPM < 60 or > 100 (≥ 5), Temp > 37.5°C. Sent every 15s max via `emailSent` reset in `uploadToThingSpeak()`.
  - **Buzzer**: Activates for weight < 100g, toggles every 1s.
- **Data Transmission**:
  - ESP32 sanitizes `statusMessage` (no commas, newlines, max 20 chars) to ensure 6 commas in serial data.
  - Uno validates data (6 commas, numeric fields), displays errors on OLED if invalid.

### Data Rates and Intervals

| Operation              | Interval/Frequency                     | Details                                      |
|------------------------|----------------------------------------|----------------------------------------------|
| MAX30102 Reading       | Every 10ms                             | Heart rate and SpO2 calculations            |
| AD8232 Reading         | Every 10ms                             | ECG signal capture                          |
| DHT11 Reading          | Every 2 seconds                        | Temperature and humidity                    |
| HX711 Reading          | Every 1 second                         | Weight measurement                          |
| Serial to Uno          | Every 1 second                         | 115200 baud, ~11.5 KB/s theoretical         |
| Web Interface Update   | Every 1 second                         | JSON via `/data` endpoint                   |
| ThingSpeak Upload      | Every 15 seconds                       | 6 fields (BPM, SpO2, Temp, Humidity, ECG, Weight) |
| Email Alerts           | Every 15 seconds (max)                 | Debounced by `emailSent` reset              |
| Buzzer Toggle          | Every 1 second (for weight < 100g)     | Alternates on/off                           |

## Applications

| Application                | Description                                                                 |
|----------------------------|-----------------------------------------------------------------------------|
| **Home Health Monitoring** | Tracks vital signs for elderly or chronically ill, alerting caregivers via email or buzzer. |
| **Remote Patient Care**    | Logs data to ThingSpeak for remote monitoring by healthcare providers, enabling telemedicine. |
| **Clinical Settings**      | Acts as a low-cost supplementary tool for real-time patient monitoring in hospitals or clinics. |
| **Research & Development** | Facilitates testing of sensor accuracy or development of health monitoring algorithms. |
| **Educational Projects**   | Demonstrates IoT, sensor integration, web servers, and serial communication for students. |

## Troubleshooting

### Common Issues and Solutions

| Issue                     | Possible Causes                              | Solutions                                                                 |
|---------------------------|----------------------------------------------|---------------------------------------------------------------------------|
| **No OLED Display**       | Incorrect wiring, wrong I2C address, library issue | Verify VCC (5V), GND, SDA (A4), SCL (A5). Run I2C scanner for `0x3C`. Update `Adafruit_SSD1306`. |
| **Invalid Data on OLED**  | Malformed serial data, loose TX-RX connection | Check ESP32 `Sent to Arduino: ...` and Uno `Received data: ...`. Secure TX (GPIO 17) to RX (pin 0). |
| **No Email Alerts**       | WiFi down, invalid SMTP, no alert triggers   | Confirm `WiFi connected!`, check SMTP settings, ensure SpO2 < 95%, BPM < 60/> 100, Temp > 37.5°C. |
| **Sensor Issues**         | Wiring, calibration, or sensor placement     | **MAX30102**: Finger contact, `irValue` > 50000. **AD8232**: Electrodes, `LO+`/`LO-` LOW. **DHT11**: 4.7kΩ pull-up. **HX711**: Calibrate `CALIBRATION_FACTOR`. |
| **Web Interface Fails**   | No internet, wrong IP, Chart.js CDN issue    | Verify IP, ensure internet for [Chart.js](https://cdn.jsdelivr.net/npm/chart.js), check `HTTP server started`. |
| **Serial Errors**         | Noise, long wires, baud mismatch             | Use short wires (<10cm), add 1kΩ pull-down on Uno RX, confirm 115200 baud. |
| **Code Upload Fails**     | RX connected, wrong board settings           | Disconnect Uno RX during upload, set correct board and partition scheme.   |

### Debugging Tips
- **Serial Monitor**: Use 115200 baud to monitor ESP32 (`Sent to Arduino`, sensor readings) and Uno (`Received data`, parsing errors).
- **Manual Serial Test**: Send test data to Uno (e.g., `75,98,36.5,45,2048,500,OK`) via Serial Monitor to verify parsing.
- **Sensor Calibration**: Adjust `CALIBRATION_FACTOR` for HX711 using known weights; test MAX30102 with finger placement.
- **Network Check**: Ping `8.8.8.8` in a separate ESP32 sketch to confirm internet access.
- **Email Debug**: Log `smtp.errorReason()` in `sendEmailAlert()` for SMTP issues; test credentials in a minimal sketch.

## License

This project is licensed under the [MIT License](https://opensource.org/licenses/MIT):

```
MIT License

Copyright (c) 2025 [Abdul Hasib]

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

## Contributors

| Name          | Contact                     | Role                     |
|---------------|-----------------------------|--------------------------|
| [Abdul hasib]   | sm.abdulhasib.bd@gmail.com      | Developer and Maintainer |

## References

- [SparkFun MAX30105 Sensor Library](https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library)
- [Adafruit DHT Sensor Library](https://github.com/adafruit/DHT-sensor-library)
- [Bogde HX711 Load Cell Library](https://github.com/bogde/HX711)
- [Mobizt ESP Mail Client Library](https://github.com/mobizt/ESP-Mail-Client)
- [ThingSpeak API Documentation](https://thingspeak.com/docs)
- [Adafruit SSD1306 OLED Library](https://github.com/adafruit/Adafruit_SSD1306)
- [Adafruit GFX Graphics Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [MathWorks ThingSpeak Arduino Library](https://github.com/mathworks/thingspeak-arduino)
- [Fritzing Circuit Design Tool](https://fritzing.org/)
- [Tinkercad Circuits Simulator](https://www.tinkercad.com/)
- [Google Gmail App Password Setup](https://support.google.com/accounts/answer/185833)
- [Chart.js Visualization Library](https://cdn.jsdelivr.net/npm/chart.js)
