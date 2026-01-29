# IoT RFID Attendance System (HMIF UNMA)

A smart attendance system built using the **ESP8266 (NodeMCU)** and **RC522 RFID** module. This device scans student ID cards, communicates with a backend server via a REST API, and provides real-time feedback via an LCD display and buzzer. Designed for **HMIF UNMA** (Himpunan Mahasiswa Informatika Universitas Majalengka).

## ✨ Key Features

* **WiFi Connectivity:** Uses **WiFiManager** for seamless network configuration without hardcoding credentials.
* **Real-time Attendance:** Sends RFID UIDs to a remote server (`internal.hmifunma.web.id`) via HTTPS.
* **Visual Feedback:** 16x2 I2C LCD displays user names, welcome messages, and attendance status (On Time/Late).
* **Audio Feedback:** Buzzer provides distinct tones for success, errors, warnings, and connection issues.
* **Smart Error Handling:** Handles network timeouts, invalid cards, and server errors gracefully.
* **Double Reset Detector:** Quickly resetting the device twice clears saved WiFi credentials.

## 🛠 Hardware Architecture

### Bill of Materials (BOM)

| Component | Description | Quantity |
| --- | --- | --- |
| **NodeMCU V3** | ESP8266 WiFi Microcontroller | 1 |
| **RFID-RC522** | 13.56MHz RFID Reader/Writer | 1 |
| **LCD 1602** | 16x2 Character Display | 1 |
| **I2C Module** | I2C Interface for LCD (PCF8574) | 1 |
| **Active Buzzer** | 5V Piezo Buzzer | 1 |
| **Jumper Wires** | Female-to-Female / Male-to-Female | Set |

## 🔌 Wiring Diagram

Based on the `schema.png` and source code definitions:

### Pinout Configuration

| Component | Component Pin | NodeMCU Pin (GPIO) | Description |
| --- | --- | --- | --- |
| **RFID** | SDA (SS) | **D4** (GPIO2) | Chip Select |
|  | SCK | **D5** (GPIO14) | SPI Clock |
|  | MOSI | **D7** (GPIO13) | SPI MOSI |
|  | MISO | **D6** (GPIO12) | SPI MISO |
|  | RST | **D3** (GPIO0) | Reset |
|  | 3.3V | 3V3 | Power |
|  | GND | GND | Ground |
| **LCD I2C** | SDA | **D2** (GPIO4) | I2C Data |
|  | SCL | **D1** (GPIO5) | I2C Clock |
|  | VCC | VV / 5V | Power |
|  | GND | GND | Ground |
| **Buzzer** | (+) Positive | **D8** (GPIO15) | Signal |
|  | (-) Negative | GND | Ground |

> **Note:** The specific I2C address used in the code is `0x27`. If your LCD does not light up, check if your module uses `0x3F`.

## 💻 Software Requirements

To compile and upload the code, you need the **Arduino IDE** with the **ESP8266 Board Manager** installed.

### Required Libraries

Install the following libraries via the Arduino Library Manager:

1. **MFRC522** by GithubCommunity
2. **ArduinoJson** by Benoit Blanchon (Version 6.x)
3. **WiFiManager** by tzapu
4. **LiquidCrystal_I2C** by Frank de Brabander

## 🚀 Installation & Setup

1. **Clone the Repository**
```bash
git clone https://github.com/wildanzm/iot-hmif.git

```


2. **Open in Arduino IDE**
Open the `code_v1.ino` file.
3. **Select Board**
Go to `Tools` > `Board` > `ESP8266 Boards` > `NodeMCU 1.0 (ESP-12E Module)`.
4. **Verify Configuration**
Ensure the `apiToken` and `attendanceUrl` in the code match your backend server configuration.
```cpp
const char* attendanceUrl = "https://internal.hmifunma.web.id/api/attendance/tap";
const char* apiToken = "YOUR_API_TOKEN_HERE";

```


5. **Upload**
Connect the NodeMCU via USB and click the **Upload** button.

## ⚙️ Configuration

### Connecting to WiFi (First Time Use)

1. Power on the device. The LCD will show "Connecting to Internet...".
2. If no saved WiFi is found, it creates an Access Point named **"Core HMIF"**.
3. Connect to this network using your phone/laptop.
4. A captive portal should open (or go to `192.168.4.1`).
5. Select your local WiFi network and enter the password.
6. The device will save credentials and reboot.

### Resetting WiFi Settings

To reset the saved WiFi credentials, perform a **Double Reset**:

1. Press the **RST** button on the NodeMCU.
2. Wait for the startup beep.
3. Immediately press **RST** again within 2-3 seconds.
4. The buzzer will emit a specific tone sequence indicating settings have been cleared.

## 📡 API Integration

The device sends a `POST` request to the configured endpoint.

**Request Header:**

```http
Content-Type: application/json
X-API-TOKEN: <your-token>

```

**Request Body:**

```json
{
  "rfid_uid": "A1B2C3D4"
}

```

**Expected Response Format:**
The device logic expects JSON in this structure:

```json
{
  "status": "success",
  "data": {
    "user_name": "User",
    "attendance_status": "Hadir", // or "Terlambat"
    "lateness_minutes": 0
  }
}

```
