#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// ===================================================================
// APPLICATION CONFIGURATION
// ===================================================================
const char* attendanceUrl = "https://internal.hmifunma.web.id/api/attendance/tap";
const char* apiToken = "rN0QjGbq8gIium1wqgfXObMyCxM62bWP2kPOGODoUIdWVYY7EfY1yhAgsQlBSXht";

// ===================================================================
// HARDWARE CONFIGURATION
// ===================================================================
#define SS_PIN      D4
#define RST_PIN     D3
#define BUZZER_PIN  D8

#define I2C_ADDR    0x27  // I2C Address
#define LCD_COLUMNS 16
#define LCD_ROWS    2

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_ROWS);

// ===================================================================
// MAIN FUNCTIONS
// ===================================================================

void setup() {
  Serial.begin(115200);
  SPI.begin();
  
  Wire.begin(D2, D1); // SDA: D2, SCL: D1
  lcd.init();
  lcd.backlight();
  lcdPrintCentered("Menunggu", 0);
  lcdPrintCentered("Instalasi", 1);

  mfrc522.PCD_Init();
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  setupWifiManager();
  
  Serial.println("\n=================================");
  Serial.println("RFID Attendance System Ready");
  Serial.println("Please tap your card...");
  Serial.println("=================================");
  
  lcd.clear();
  lcdPrintCentered("Selamat Datang", 0);
  lcdPrintCentered("Vistara Abhiyasa", 1);
  
  playTone(2000, 150); // Beep indicating device is ready
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }

  String rfid_uid = getRfidUid();
  Serial.print("\nCard detected! UID: ");
  Serial.println(rfid_uid);

  sendAttendanceRequest(rfid_uid); // This function now controls the LCD display delay.

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  
  // The delay(3000) pause is removed from here and moved to processServerResponse.
  
  lcd.clear();
  lcdPrintCentered("Selamat Datang", 0);
  lcdPrintCentered("Vistara Abhiyasa", 1);
  Serial.println("\nPlease tap your card...");
}

// ===================================================================
// HELPER FUNCTIONS
// ===================================================================

/**
 * Displays text in the center of the specified LCD line.
 */
void lcdPrintCentered(String text, int row) {
  int textLength = text.length();
  int startCol = (LCD_COLUMNS - textLength) / 2;
  
  if (startCol < 0) {
    startCol = 0;
  }
  
  lcd.setCursor(startCol, row);
  lcd.print(text);
}

/**
 * Set up WiFi connections via WiFiManager and display the status on the LCD.
 */
void setupWifiManager() {
  WiFiManager wm;
  
  lcd.clear();
  lcdPrintCentered("Menghubungkan", 0);
  lcdPrintCentered("ke Internet...", 1);

  if (handleDoubleReset(&wm)) {
    wm.setConfigPortalTimeout(300);
  } else {
    wm.setConfigPortalTimeout(180);
  }

  if (!wm.autoConnect("Core HMIF")) {
    Serial.println("Failed to connect and hit timeout. Restarting...");
    lcd.clear();
    lcdPrintCentered("Gagal Terhubung", 0);
    delay(3000);
    ESP.restart();
  }
  
  clearRtcFlag();
  
  lcd.clear();
  lcdPrintCentered("Berhasil", 0);
  lcdPrintCentered("Terhubung!", 1);
  
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  delay(2000);
}

/**
 * Send the card's UID to the server to record attendance.
 */
void sendAttendanceRequest(String rfid_uid) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Disconnected.");
    lcd.clear();
    lcdPrintCentered("Koneksi Internet", 0);
    lcdPrintCentered("Terputus!", 1);
    handleBuzzer("connection_error");
    delay(3000); // Pause to display error
    return;
  }
  
  WiFiClientSecure client;
  HTTPClient http;
  client.setInsecure();
  
  lcd.clear();
  lcdPrintCentered("Tunggu Sebentar", 0);
  lcdPrintCentered("........", 1);

  if (http.begin(client, attendanceUrl)) {
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");
    http.addHeader("X-API-TOKEN", apiToken);

    String jsonPayload = "{\"rfid_uid\":\"" + rfid_uid + "\"}";
    int httpCode = http.POST(jsonPayload);

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("Server Response: " + payload);
      processServerResponse(payload); // JSON process for LCD & Buzzer (including delay)
    } else {
      Serial.printf("HTTP POST failed, error code: %d\n", httpCode);
      lcd.clear();
      lcdPrintCentered("Koneksi Gagal", 0);
      handleBuzzer("connection_error");
      delay(3000); // Pause to display error
    }
    http.end();
  } else {
    Serial.printf("[HTTP] Unable to connect to URL\n");
    lcd.clear();
    lcdPrintCentered("URL Error", 0);
    handleBuzzer("connection_error");
    delay(3000); // Pause to display error
  }
}

/**
 * Processing JSON responses from the server and activating the LCD and buzzer.
 */
void processServerResponse(String payload) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    lcd.clear();
    lcdPrintCentered("Server Error", 0);
    lcdPrintCentered("Tidak Valid", 1);
    handleBuzzer("error");
    delay(3000);
    return;
  }
  
  if (doc.containsKey("status")) {
    const char* status = doc["status"];
    
    if (strcmp(status, "success") == 0) {
      const char* attendanceStatus = doc["data"]["attendance_status"];
      String fullName = doc["data"]["user_name"];
      String firstName = getFirstName(fullName);
      
      if (strcmp(attendanceStatus, "Terlambat") == 0) {
        // --- LATE MESSAGE (3 Pages) ---
        int lateness = doc["data"]["lateness_minutes"];
        String lateMsg = String(lateness) + " Menit";
        
        lcd.clear();
        lcdPrintCentered("Selamat Datang", 0);
        lcdPrintCentered(firstName, 1);
        handleBuzzer("warning");
        delay(1500);

        lcd.clear();
        lcdPrintCentered("Terlambat!", 0);
        lcdPrintCentered(lateMsg, 1);
        delay(1500);
        
        lcd.clear();
        lcdPrintCentered("Semoga Harimu", 0);
        lcdPrintCentered("Menyenangkan", 1);
        delay(1500);
        
      } else {
        // --- MESSAGE SUCCESSFUL (2 Pages) ---
        lcd.clear();
        lcdPrintCentered("Selamat Datang", 0);
        lcdPrintCentered(firstName, 1);
        handleBuzzer("success");
        delay(1500); 

        lcd.clear();
        lcdPrintCentered("Semoga Harimu", 0);
        lcdPrintCentered("Menyenangkan", 1);
        delay(1500);
      }
      
    } else if (strcmp(status, "warning") == 0) {
      // --- MESSAGE IS ABSENT ---
      lcd.clear();
      lcdPrintCentered("Kamu Sudah", 0);
      lcdPrintCentered("Absen Hari Ini", 1);
      handleBuzzer("warning");
      delay(3000);
      
    } else {
      // --- CARD ORDER DECLINED ---
      lcd.clear();
      lcdPrintCentered("Sistem Error", 0);
      lcdPrintCentered("Kartu Ditolak", 1);
      handleBuzzer("error");
      delay(3000);
    }
  } else if (doc.containsKey("message")) {
    // --- SCHEDULE MESSAGE (from server) ---
    String message = doc["message"];
    if (message.indexOf("No active schedule") != -1) {
      lcd.clear();
      lcdPrintCentered("Info", 0);
      lcdPrintCentered("Tdk Ada Jadwal", 1);
      handleBuzzer("error");
    } else if (message.indexOf("No schedule is active") != -1) {
      lcd.clear();
      lcdPrintCentered("Info", 0);
      lcdPrintCentered("Absen Ditutup", 1);
      handleBuzzer("error");
    } else {
      lcd.clear();
      lcdPrintCentered("Gagal Absen", 0);
      lcdPrintCentered("Tidak Valid", 1);
      handleBuzzer("error");
    }
    delay(3000);
  } else {
    // --- UNKNOWN ERROR MESSAGE ---
    lcd.clear();
    lcdPrintCentered("Sistem Error", 0);
    lcdPrintCentered("Tidak Terdaftar", 1);
    handleBuzzer("error");
    delay(3000);
  }
}

/**
 * Control the buzzer sound based on the status received.
 */
void handleBuzzer(const char* status) {
  if (strcmp(status, "success") == 0) {
    playTone(3000, 80); // Sukses: 1 beep singkat
  } else if (strcmp(status, "warning") == 0) {
    playTone(1500, 50); delay(70); playTone(1500, 50); delay(70); playTone(1500, 50); // Warning/Late: 3 quick beeps
  } else if (strcmp(status, "connection_error") == 0) {
    playTone(300, 150); delay(100); playTone(300, 150); // Connection Error: 2 low beeps
  } else {
    playTone(300, 500); // Common Error: 1 long low beep
  }
}

/**
 * Take the first word (First Name) from the full name.
 */
String getFirstName(String fullName) {
  int firstSpace = fullName.indexOf(' ');
  if (firstSpace == -1) {
    return fullName;
  } else {
    return fullName.substring(0, firstSpace);
  }
}

/**
 * Check if a double reset has occurred. If so, delete the WiFi settings.
 */
bool handleDoubleReset(WiFiManager *wm) {
  const uint32_t RTC_MAGIC_FLAG = 0xDAFEDAFE;
  struct {
    uint32_t crc32;
    byte data[28];
  } rtcData;
  ESP.rtcUserMemoryRead(0, (uint32_t*)&rtcData, sizeof(rtcData));

  if (rtcData.crc32 == RTC_MAGIC_FLAG) {
    Serial.println("Double reset detected! Erasing WiFi settings.");
    wm->resetSettings();
    playTone(1000, 100); delay(100); playTone(500, 200);
    return true;
  } 
  
  rtcData.crc32 = RTC_MAGIC_FLAG;
  ESP.rtcUserMemoryWrite(0, (uint32_t*)&rtcData, sizeof(rtcData));
  return false;
}

/**
 * Clearing the double reset flag from the RTC memory.
 */
void clearRtcFlag() {
  struct {
    uint32_t crc32;
    byte data[28];
  } rtcData;
  rtcData.crc32 = 0;
  ESP.rtcUserMemoryWrite(0, (uint32_t*)&rtcData, sizeof(rtcData));
}

/**
 * Read the UID from the RFID card and convert it to a string.
 */
String getRfidUid() {
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
    uid.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  uid.toUpperCase();
  return uid;
}

/**
 * Basic function to sound the buzzer.
 */
void playTone(int frequency, int duration) {
  tone(BUZZER_PIN, frequency, duration);
}