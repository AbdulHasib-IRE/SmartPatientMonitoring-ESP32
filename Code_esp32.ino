#include <WiFi.h>
#include <ThingSpeak.h>
#include <ESP_Mail_Client.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "DHT.h"
#include "HX711.h"
#include <WebServer.h>
#include <time.h>

// SpO2 Calculation Variables
#define SAMPLING_RATE 400
#define AVERAGING 4
#define ADC_RANGE 4096
#define SAMPLE_COUNT 25

// WiFi Credentials
const char* ssid = "####################";
const char* password = "**************";

// ThingSpeak Settings
unsigned long channelID = %%%%%%%%%%%;
const char* writeAPIKey = "&&&&&&&&&&&&&&";
WiFiClient client;

// Email Configuration
#define SENDER_EMAIL "!!!!!!!!!!!!!!!!!!!!!!!!"
#define SENDER_APP_PASSWORD "*********************"
#define RECIPIENT_EMAIL "sm.abdulhasib.bd@gmail.com"
SMTPSession smtp;
bool emailSent = false;

// MAX30102 Sensor
MAX30105 max30102;
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
float beatsPerMinute;
int beatAvg;
long lastBeat = 0;
float spo2 = 0.0;

// DHT11 Sensor
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// HX711 Load Cell
#define LOADCELL_DOUT_PIN 12
#define LOADCELL_SCK_PIN 13
HX711 scale;
int weightReading;
#define CALIBRATION_FACTOR -478.507

// AD8232 ECG Sensor
#define LO_PLUS_PIN 34
#define LO_MINUS_PIN 35
#define ECG_OUTPUT_PIN 36
int ecgValue = 0;
int leadStatus = 1;

// Buzzer
#define BUZZER_PIN 5

// Serial Communication (to Arduino)
#define ARDUINO_RX 16
#define ARDUINO_TX 17

// Web Server
WebServer server(80);

// Sensor Data Variables
float humidity = 0;
float dhtTemp = 0;
String statusMessage = "System OK";

// Timing Control
unsigned long lastUpload = 0;
unsigned long lastSerialSend = 0;
unsigned long lastBuzzerCheck = 0;
unsigned long lastDebugPrint = 0;
unsigned long lastMax30102Read = 0;
unsigned long lastDhtRead = 0;
unsigned long lastLoadCellRead = 0;
unsigned long lastEcgRead = 0;
bool buzzerState = false;
const unsigned long max30102Interval = 10; // ms
const unsigned long dhtInterval = 2000; // ms
const unsigned long loadCellInterval = 1000; // ms
const unsigned long ecgInterval = 10; // ms

// Minified HTML with Chart.js CDN and debugging
const char index_html[] PROGMEM = R"=====(
<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Patient Monitor</title><script src="https://cdn.jsdelivr.net/npm/chart.js"></script><style>*{box-sizing:border-box;margin:0;padding:0;font-family:'Segoe UI',sans-serif}body{background:linear-gradient(135deg,#1a2a6c,#b21f1f);min-height:100vh;padding:20px}.container{max-width:1400px;margin:0 auto}header{padding:20px 0}h1{color:#fff;font-size:1.8rem;text-shadow:2px 2px 4px rgba(0,0,0,0.5)}.dashboard{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:15px}.card{background:rgba(255,255,255,0.95);border-radius:12px;box-shadow:0 8px 20px rgba(0,0,0,0.2);padding:15px}.card-title{font-size:1.1rem;color:#2c3e50;margin-bottom:10px;border-bottom:1px solid #3498db}.sensor-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:8px}.sensor-item{padding:10px;border-radius:8px;text-align:center;background:#f8f9fa}.sensor-name{font-weight:600;margin-bottom:4px;color:#2c3e50;font-size:0.85rem}.sensor-value{font-size:1.3rem;font-weight:700}.normal-range{font-size:0.8rem;color:#555;margin-top:4px}.status-badge{padding:3px 8px;border-radius:15px;font-size:0.75rem;font-weight:600;display:inline-block}.normal{background:#2ecc71;color:#fff}.warning{background:#f39c12;color:#fff}.alert{background:#e74c3c;color:#fff}.chart-container{height:160px;margin-top:10px}.system-status{margin-top:12px;padding:10px;background:rgba(0,0,0,0.2);border-radius:10px;color:#fff;display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:10px}.status-card{padding:10px;border-radius:8px;text-align:center;background:rgba(255,255,255,0.15)}</style></head><body><div class="container"><header><h1>Patient Monitoring</h1></header><div class="dashboard"><div class="card"><h2 class="card-title">ECG</h2><div class="chart-container"><canvas id="ecgChart"></canvas></div><div class="sensor-item"><div class="sensor-name">ECG Value</div><div class="sensor-value" id="ecgValue">0</div><div class="normal-range">Check leads if disconnected</div><div class="status-badge normal">Normal</div></div></div><div class="card"><h2 class="card-title">Pulse & Oxygen</h2><div class="sensor-grid"><div class="sensor-item"><div class="sensor-name">Heart Rate</div><div class="sensor-value" id="bpmValue">0</div><div class="normal-range">Normal: 60-100 BPM</div><div class="status-badge normal">BPM</div></div><div class="sensor-item"><div class="sensor-name">SpO2</div><div class="sensor-value" id="spo2Value">0%</div><div class="normal-range">Normal: 95-100%</div><div class="status-badge normal">Oxygen</div></div></div><div class="chart-container"><canvas id="pulseChart"></canvas></div></div><div class="card"><h2 class="card-title">Temperature</h2><div class="sensor-grid"><div class="sensor-item"><div class="sensor-name">Body Temp</div><div class="sensor-value" id="bodyTempValue">0°C</div><div class="normal-range">Normal: 36.5-37.5°C</div><div class="status-badge normal">Normal</div></div><div class="sensor-item"><div class="sensor-name">Humidity</div><div class="sensor-value" id="humidityValue">0%</div><div class="normal-range">No specific range</div></div></div></div><div class="card"><h2 class="card-title">Weight</h2><div class="sensor-item"><div class="sensor-name">Current Weight</div><div class="sensor-value" id="weightValue">0 g</div><div class="normal-range">Normal: >100 g</div><div class="status-badge normal">OK</div></div><div class="chart-container"><canvas id="weightChart"></canvas></div></div></div><div class="system-status"><div class="status-card"><div class="sensor-name">WiFi</div><div class="sensor-value">Connected</div></div><div class="status-card"><div class="sensor-name">Uptime</div><div class="sensor-value" id="uptimeValue">0:00:00</div></div><div class="status-card"><div class="sensor-name">ThingSpeak</div><div class="sensor-value">Active</div></div></div></div><script>const ecgCtx=document.getElementById("ecgChart").getContext("2d"),pulseCtx=document.getElementById("pulseChart").getContext("2d"),weightCtx=document.getElementById("weightChart").getContext("2d");const ecgChart=new Chart(ecgCtx,{type:"line",data:{datasets:[{data:[],borderColor:"#e74c3c",borderWidth:2,tension:.4}]},options:{responsive:!0,maintainAspectRatio:!1,scales:{y:{min:0,max:4095},x:{display:!1}}}}),pulseChart=new Chart(pulseCtx,{type:"line",data:{datasets:[{data:[],borderColor:"#3498db",borderWidth:2}]},options:{responsive:!0,maintainAspectRatio:!1,scales:{y:{min:0,max:200},x:{display:!1}}}}),weightChart=new Chart(weightCtx,{type:"line",data:{datasets:[{data:[],borderColor:"#9b59b6",borderWidth:2}]},options:{responsive:!0,maintainAspectRatio:!1,scales:{y:{min:0,max:1000},x:{display:!1}}}});function updateData(){fetch("/data").then(r=>r.json()).then(d=>{console.log("Data received:",d);document.getElementById("ecgValue").textContent=d.ecg===0?"No Signal":d.ecg;const ecgBadge=document.querySelector("#ecgValue").nextElementSibling.nextElementSibling;ecgBadge.className=d.ecg===0?"status-badge warning":"status-badge normal";ecgBadge.textContent=d.ecg===0?"Disconnected":"Connected";console.log("ECG Chart data:",d.ecg);updateChart(ecgChart,d.ecg,100);document.getElementById("bpmValue").textContent=d.bpm===0?"No Signal":d.bpm;const bpmBadge=document.querySelector("#bpmValue").nextElementSibling.nextElementSibling;bpmBadge.className=d.bpm>=60&&d.bpm<=100?"status-badge normal":d.bpm>0?"status-badge alert":"status-badge warning";bpmBadge.textContent=d.bpm>=60&&d.bpm<=100?"Normal":d.bpm>0?"Abnormal":"No Signal";console.log("Pulse Chart data:",d.bpm);updateChart(pulseChart,d.bpm,20);document.getElementById("spo2Value").textContent=d.spo2===0?"No Signal":d.spo2+"%";const spo2Badge=document.querySelector("#spo2Value").nextElementSibling.nextElementSibling;spo2Badge.className=d.spo2>=95?"status-badge normal":d.spo2>0?"status-badge alert":"status-badge warning";spo2Badge.textContent=d.spo2>=95?"Normal":d.spo2>0?"Low":"No Signal";document.getElementById("bodyTempValue").textContent=d.temp===0?"No Signal":d.temp+"°C";const tempBadge=document.querySelector("#bodyTempValue").nextElementSibling.nextElementSibling;tempBadge.className=d.temp>=36.5&&d.temp<=37.5?"status-badge normal":d.temp>0?"status-badge alert":"status-badge warning";tempBadge.textContent=d.temp>=36.5&&d.temp<=37.5?"Normal":d.temp>0?"Abnormal":"No Signal";document.getElementById("humidityValue").textContent=d.humidity===0?"No Signal":d.humidity+"%";document.getElementById("weightValue").textContent=d.weight===0?"No Signal":d.weight+" g";const weightBadge=document.querySelector("#weightValue").nextElementSibling.nextElementSibling;weightBadge.className=d.weight<100?"status-badge alert":d.weight<300?"status-badge warning":"status-badge normal";weightBadge.textContent=d.weight<100?"Low":d.weight<300?"Warning":"Normal";console.log("Weight Chart data:",d.weight);updateChart(weightChart,d.weight,15);document.getElementById("uptimeValue").textContent=d.uptime;console.log("Uptime updated:",d.uptime);}).catch(e=>console.error("Fetch error:",e));}function updateChart(e,t,a){console.log("Updating chart with value:",t);e.data.datasets[0].data.length>a&&e.data.datasets[0].data.shift();e.data.datasets[0].data.push(t);e.update();}updateData();setInterval(updateData,1000);</script></body></html>
)=====";

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleData() {
  unsigned long currentMillis = millis();
  unsigned long total_seconds = currentMillis / 1000;
  unsigned long days = total_seconds / 86400;
  unsigned long remaining_seconds = total_seconds % 86400;
  unsigned long hours = remaining_seconds / 3600;
  remaining_seconds %= 3600;
  unsigned long minutes = remaining_seconds / 60;
  unsigned long seconds = remaining_seconds % 60;
  
  String daysStr = String(days);
  String hoursStr = String(hours);
  if (hoursStr.length() < 2) hoursStr = "0" + hoursStr;
  String minutesStr = String(minutes);
  if (minutesStr.length() < 2) minutesStr = "0" + minutesStr;
  String secondsStr = String(seconds);
  if (secondsStr.length() < 2) secondsStr = "0" + secondsStr;
  
  String uptime;
  if (days > 0) {
    uptime = daysStr + "d " + hoursStr + ":" + minutesStr + ":" + secondsStr;
  } else {
    uptime = hoursStr + ":" + minutesStr + ":" + secondsStr;
  }
  
  String json = "{";
  json += "\"bpm\":" + String(beatAvg) + ",";
  json += "\"spo2\":" + String(spo2, 1) + ",";
  json += "\"ecg\":" + String(leadStatus == 0 ? 0 : ecgValue) + ",";
  json += "\"temp\":" + String(dhtTemp, 1) + ",";
  json += "\"humidity\":" + String(humidity, 1) + ",";
  json += "\"weight\":" + String(weightReading) + ",";
  json += "\"uptime\":\"" + uptime + "\"";
  json += "}";
  server.send(200, "application/json", json);
  Serial.println("Served /data: " + json);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, ARDUINO_RX, ARDUINO_TX);
  
  Serial.println("\nStarting Patient Monitoring System");
  Serial.println("Initializing sensors...");

  // Initialize MAX30102
  if (!max30102.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 error! Check wiring");
  } else {
    Serial.println("MAX30102 OK");
  }
  max30102.setup(0x1F, 4, 2, 400, 411, 4096);
  
  // Initialize DHT11
  dht.begin();
  Serial.println("DHT11 initialized");

  // Initialize HX711
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(CALIBRATION_FACTOR);
  scale.tare();
  Serial.println("Load cell calibrated");

  // Initialize AD8232
  pinMode(LO_PLUS_PIN, INPUT);
  pinMode(LO_MINUS_PIN, INPUT);
  Serial.println("ECG sensor ready");

  // Initialize Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  byte attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Synchronize time
    configTime(6 * 3600, 0, "pool.ntp.org");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        now = time(nullptr);
    }
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial.print("Current time: ");
    Serial.print(asctime(&timeinfo));
    
    ThingSpeak.begin(client);
    Serial.println("ThingSpeak initialized");
    
    server.on("/", handleRoot);
    server.on("/data", handleData);
    server.begin();
    Serial.println("HTTP server started");
  } else {
    Serial.println("\nWiFi connection failed!");
  }
  
  delay(2000);
}

void sendEmailAlert(String subject, String content) {
  if(!emailSent) {
    Serial.println("Preparing email alert: " + subject);
    
    ESP_Mail_Session session;
    session.server.host_name = "smtp.gmail.com";
    session.server.port = 465;
    session.login.email = SENDER_EMAIL;
    session.login.password = SENDER_APP_PASSWORD;
    
    SMTP_Message message;
    message.sender.email = SENDER_EMAIL;
    message.subject = subject;
    message.addRecipient("", RECIPIENT_EMAIL);
    message.text.content = content.c_str();
    
    if (smtp.connect(&session)) {
      Serial.println("SMTP connected");
      if (MailClient.sendMail(&smtp, &message)) {
        emailSent = true;
        Serial.println("Email sent successfully");
      } else {
        Serial.println("Email sending failed: " + smtp.errorReason());
      }
    } else {
      Serial.println("SMTP connection failed: " + smtp.errorReason());
    }
  }
}

void readECG() {
  ecgValue = analogRead(ECG_OUTPUT_PIN);
  leadStatus = 1;
  if ((digitalRead(LO_PLUS_PIN) == HIGH) || (digitalRead(LO_MINUS_PIN) == HIGH)) {
    leadStatus = 0;
    ecgValue = 0;
    Serial.println("WARNING: ECG lead disconnected!");
  }
  Serial.print("ECG reading: "); Serial.println(ecgValue);
}

void checkHeartRate(long irValue) {
  if (checkForBeat(irValue)) {
    long delta = millis() - lastBeat;
    lastBeat = millis();
    beatsPerMinute = 60 / (delta / 1000.0);
    if (beatsPerMinute > 20 && beatsPerMinute < 255) {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++) beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }
  Serial.print("Heart rate: "); Serial.println(beatAvg);
}

void calculateSpO2(long red, long ir) {
  static long redBuffer[SAMPLE_COUNT];
  static long irBuffer[SAMPLE_COUNT];
  static byte spo2Count = 0;
  
  for (int i = SAMPLE_COUNT - 1; i > 0; i--) {
    redBuffer[i] = redBuffer[i-1];
    irBuffer[i] = irBuffer[i-1];
  }
  
  redBuffer[0] = red;
  irBuffer[0] = ir;
  if (spo2Count++ < SAMPLE_COUNT) return;
  spo2Count = 0;
  float redDC = 0, irDC = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    redDC += redBuffer[i];
    irDC += redBuffer[i];
  }
  redDC /= SAMPLE_COUNT;
  irDC /= SAMPLE_COUNT;
  if (irDC < 50000) {
    spo2 = 0;
    Serial.println("SpO2: No signal (IR < 50000)");
    return;
  }
  float redAC = 0, irAC = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    redAC += pow(redBuffer[i] - redDC, 2);
    irAC += pow(irBuffer[i] - irDC, 2);
  }
  redAC = sqrt(redAC / SAMPLE_COUNT);
  irAC = sqrt(irAC / SAMPLE_COUNT);
  float ratio = (redAC / redDC) / (irAC / irDC);
  spo2 = 110.0 - 18.0 * ratio;
  spo2 = constrain(spo2, 70.0, 100.0);
  Serial.print("SpO2: "); Serial.println(spo2);
}

void sendToArduino() {
  // Sanitize status message: remove commas, newlines, limit to 20 chars
  String shortStatus = statusMessage;
  shortStatus.replace(",", " ");
  shortStatus.replace("\n", "");
  shortStatus.replace("\r", "");
  shortStatus = shortStatus.substring(0, min(shortStatus.length(), 20U));
  
  // Ensure valid numeric fields
  int validBpm = (beatAvg >= 0) ? beatAvg : 0;
  int validSpo2 = (spo2 >= 0) ? (int)spo2 : 0;
  float validTemp = (dhtTemp >= 0) ? dhtTemp : 0.0;
  int validHumidity = (humidity >= 0) ? (int)humidity : 0;
  int validEcg = (ecgValue >= 0 && leadStatus == 1) ? ecgValue : 0;
  int validWeight = (weightReading >= 0) ? weightReading : 0;
  
  String data = String(validBpm) + "," + 
                String(validSpo2) + "," + 
                String(validTemp, 1) + "," + 
                String(validHumidity) + "," + 
                String(validEcg) + "," + 
                String(validWeight) + "," + 
                shortStatus;
  Serial2.println(data);
  Serial.print("Sent to Arduino: ");
  Serial.println(data);
}

void printCoreData() {
  Serial.print("[CORE] HR:");
  Serial.print(beatAvg == 0 ? "No Signal" : String(beatAvg));
  Serial.print("bpm SpO2:");
  Serial.print(spo2 == 0 ? "No Signal" : String(spo2, 1));
  Serial.print("% Temp:");
  Serial.print(dhtTemp == 0 ? "No Signal" : String(dhtTemp, 1));
  Serial.print("C Hum:");
  Serial.print(humidity == 0 ? "No Signal" : String(humidity, 1));
  Serial.print("% ECG:");
  Serial.print(leadStatus == 0 ? "No Signal" : String(ecgValue));
  Serial.print(" W:");
  Serial.print(weightReading == 0 ? "No Signal" : String(weightReading));
  Serial.print("g Status:");
  Serial.println(statusMessage);
}

void checkAlerts() {
  statusMessage = "System OK";
  
  if(spo2 >= 5 && spo2 < 95 && !emailSent) {
    String subject = "LOW SpO2 ALERT!";
    String content = "Current SpO2: " + String(spo2, 1) + "% (Normal: 95-100%)";
    sendEmailAlert(subject, content);
    statusMessage = "ALERT: Low Oxygen!";
  }
  
  if(beatAvg >= 5 && (beatAvg < 60 || beatAvg > 100) && !emailSent) {
    String subject = "ABNORMAL PULSE ALERT!";
    String content = "Current BPM: " + String(beatAvg) + " (Normal: 60-100)";
    sendEmailAlert(subject, content);
    statusMessage = "ALERT: Abnormal Pulse!";
  }
  
  if(dhtTemp > 37.5 && !emailSent) {
    String subject = "HIGH TEMP ALERT!";
    String content = "Current Temp: " + String(dhtTemp, 1) + "°C (Normal: 36.5-37.5°C)";
    sendEmailAlert(subject, content);
    statusMessage = "ALERT: High Temp!";
  }
  
  if(weightReading < 100) {
    if(millis() - lastBuzzerCheck > 1000) {
      buzzerState = !buzzerState;
      digitalWrite(BUZZER_PIN, buzzerState);
      lastBuzzerCheck = millis();
      statusMessage = "ALERT: Low Weight!";
      if(buzzerState) Serial.println("ALERT: Low weight detected!");
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerState = false;
  }
}

void uploadToThingSpeak() {
  ThingSpeak.setField(1, beatAvg);
  ThingSpeak.setField(2, spo2);
  ThingSpeak.setField(3, dhtTemp);
  ThingSpeak.setField(4, humidity);
  ThingSpeak.setField(5, ecgValue);
  ThingSpeak.setField(6, weightReading);
  
  int status = ThingSpeak.writeFields(channelID, writeAPIKey);
  if(status == 200) {
    Serial.println("Data uploaded to ThingSpeak");
  } else {
    Serial.println("ThingSpeak upload failed. Error: " + String(status));
  }
}

void loop() {
  server.handleClient();
  
  if (millis() - lastMax30102Read >= max30102Interval) {
    long irValue = max30102.getIR();
    long redValue = max30102.getRed();
    checkHeartRate(irValue);
    calculateSpO2(redValue, irValue);
    lastMax30102Read = millis();
    
    if (millis() - lastBeat > 10000) {
      beatAvg = 0;
    }
  }
  
  if (millis() - lastDhtRead >= dhtInterval) {
    humidity = dht.readHumidity();
    dhtTemp = dht.readTemperature();
    if (isnan(humidity)) humidity = 0;
    if (isnan(dhtTemp)) dhtTemp = 0;
    Serial.print("Temp: "); Serial.print(dhtTemp); Serial.print(" Hum: "); Serial.println(humidity);
    lastDhtRead = millis();
  }
  
  if (millis() - lastLoadCellRead >= loadCellInterval) {
    if (scale.is_ready()) weightReading = abs(scale.get_units(5));
    Serial.print("Weight: "); Serial.println(weightReading);
    lastLoadCellRead = millis();
  }
  
  if (millis() - lastEcgRead >= ecgInterval) {
    readECG();
    lastEcgRead = millis();
  }
  
  checkAlerts();
  
  if (millis() - lastSerialSend > 1000) { // 1000ms interval
    sendToArduino();
    lastSerialSend = millis();
  }
  
  if (millis() - lastDebugPrint > 2000) {
    printCoreData();
    lastDebugPrint = millis();
  }
  
  if (millis() - lastUpload > 15000 && WiFi.status() == WL_CONNECTED) {
    uploadToThingSpeak();
    lastUpload = millis();
    emailSent = false;
  }
}
