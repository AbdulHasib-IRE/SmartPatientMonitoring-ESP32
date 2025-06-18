#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Data variables
int bpm = 0;
int spo2 = 0;
float temp = 0.0;
int humidity = 0;
int ecg = 0;
int weight = 0;
char statusMsg[32] = ""; // Fixed-size buffer

void setup() {
  Serial.begin(115200); // Communication with ESP32
  Serial.setTimeout(200); // Timeout to 200ms
  
  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1); // Halt forever
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Patient Monitor"));
  display.println(F("Initializing..."));
  display.display();
  delay(2000);
}

bool parseData(const char* data) {
  // Log received data for debugging
  Serial.print(F("Received data: "));
  Serial.println(data);
  
  // Validate length
  int len = strlen(data);
  if (len > 60 || len < 5) {
    Serial.print(F("Invalid data length: "));
    Serial.println(len);
    return false;
  }

  // Count commas
  int commaCount = 0;
  for (int i = 0; data[i]; i++) {
    if (data[i] == ',') commaCount++;
  }
  if (commaCount != 6) {
    Serial.print(F("Invalid format: comma count = "));
    Serial.println(commaCount);
    return false;
  }

  // Temporary buffer
  char tempStr[61];
  strncpy(tempStr, data, sizeof(tempStr) - 1);
  tempStr[sizeof(tempStr) - 1] = '\0';
  
  // Parse fields
  char* token = strtok(tempStr, ",");
  int field = 0;
  bool valid = true;

  while (token && field < 7) {
    // Validate numeric fields
    if (field < 6) {
      for (char* p = token; *p; p++) {
        if (!isdigit(*p) && *p != '.' && *p != '-') {
          Serial.print(F("Invalid number in field "));
          Serial.println(field);
          valid = false;
          break;
        }
      }
    }
    
    if (!valid) break;

    switch (field) {
      case 0: bpm = atoi(token); break;
      case 1: spo2 = atoi(token); break;
      case 2: temp = atof(token); break;
      case 3: humidity = atoi(token); break;
      case 4: ecg = atoi(token); break;
      case 5: weight = atoi(token); break;
      case 6: strncpy(statusMsg, token, sizeof(statusMsg) - 1); statusMsg[sizeof(statusMsg) - 1] = '\0'; break;
    }
    
    token = strtok(NULL, ",");
    field++;
  }

  if (field != 7 || token) {
    Serial.println(F("Parsing failed: incorrect field count"));
    return false;
  }

  return true;
}

void updateOLED() {
  display.clearDisplay();
  
  // Display header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("BPM:"));
  display.print(bpm);
  display.print(F(" SpO2:"));
  display.print(spo2);
  display.println(F("%"));
  
  // Display vital signs
  display.print(F("T:"));
  display.print(temp, 1);
  display.print(F("C H:"));
  display.print(humidity);
  display.println(F("%"));
  
  // Display measurements
  display.print(F("ECG:"));
  display.print(ecg);
  display.print(F(" W:"));
  display.print(weight);
  display.println(F("g"));
  
  // Display status message with wrapping
  display.setCursor(0, 48);
  if (strlen(statusMsg) > 21) {
    char firstLine[22];
    strncpy(firstLine, statusMsg, 21);
    firstLine[21] = '\0';
    display.println(firstLine);
    display.println(&statusMsg[21]);
  } else {
    display.println(statusMsg);
  }
  
  display.display();
}

void displayInvalidData(const char* data, const char* errorReason) {
  display.clearDisplay();
  
  // Display error header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("Invalid Data Received"));
  
  // Display raw data (truncate to fit ~21 chars per line)
  display.setCursor(0, 16);
  int len = strlen(data);
  if (len > 21) {
    char firstLine[22];
    strncpy(firstLine, data, 21);
    firstLine[21] = '\0';
    display.println(firstLine);
    if (len > 42) {
      display.println(F("... (truncated)"));
    } else {
      display.println(&data[21]);
    }
  } else {
    display.println(data);
  }
  
  // Display error reason
  display.setCursor(0, 48);
  int reasonLen = strlen(errorReason);
  if (reasonLen > 21) {
    char reasonLine[22];
    strncpy(reasonLine, errorReason, 21);
    reasonLine[21] = '\0';
    display.println(reasonLine);
  } else {
    display.println(errorReason);
  }
  
  display.display();
}

void loop() {
  if (Serial.available()) {
    char data[64];
    int len = 0;
    unsigned long start = millis();
    while (millis() - start < 200 && len < sizeof(data) - 1) {
      if (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
          if (len > 0) { // Ignore empty lines
            data[len] = '\0';
            break;
          }
        } else {
          data[len++] = c;
        }
      }
    }
    if (len > 0) {
      data[len] = '\0';
      if (parseData(data)) {
        updateOLED();
        // Debug output
        Serial.print(F("Parsed: "));
        Serial.print(bpm); Serial.print(F(" BPM, "));
        Serial.print(spo2); Serial.print(F("%, "));
        Serial.print(temp, 1); Serial.print(F("°C, "));
        Serial.print(humidity); Serial.print(F("%, "));
        Serial.print(ecg); Serial.print(F(", "));
        Serial.print(weight); Serial.print(F("g, "));
        Serial.println(statusMsg);
      } else {
        // Determine error reason
        const char* errorReason;
        int commaCount = 0;
        for (int i = 0; data[i]; i++) {
          if (data[i] == ',') commaCount++;
        }
        if (len < 5) {
          errorReason = "Data too short";
        } else if (commaCount != 6) {
          errorReason = "Wrong comma count";
        } else {
          errorReason = "Invalid format";
        }
        displayInvalidData(data, errorReason);
        Serial.println(F("Parse failed"));
      }
    } else {
      Serial.println(F("No valid data received"));
    }
  }
}
