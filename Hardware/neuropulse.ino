#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Adafruit_ADS1X15.h>
#include <arduinoFFT.h>
#include <TFT_eSPI.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "heartRate.h"


const char* WIFI_SSID     = "Colosseum2";
const char* WIFI_PASSWORD = "Dbit@2026";


const char* MQTT_HOST     = "ace1d79032ba46d3b0b275be6d0b42f4.s1.eu.hivemq.cloud";
const int   MQTT_PORT     = 8883;
const char* MQTT_USER     = "InnovatorsArc";
const char* MQTT_PASSWORD = "Innovators07@";


const char* WAQI_TOKEN    = "f90981143616378a57a85e8c323b01c254a50090";
const float LATITUDE      = 19.0760f;
const float LONGITUDE     = 72.8777f;


#define PIN_DHT_DATA   13
#define PIN_I2C_SDA    21
#define PIN_I2C_SCL    22

#define PIN_LED_R      25
#define PIN_LED_G      26
#define PIN_LED_B      27

#define PIN_ENC_CLK    33
#define PIN_ENC_DT     34
#define PIN_ENC_SW     35


#define DHTTYPE DHT11
DHT dht(PIN_DHT_DATA, DHTTYPE);
Adafruit_ADS1115 ads;
TFT_eSPI tft = TFT_eSPI();


WiFiClientSecure mqttSecure;
PubSubClient mqttClient(mqttSecure);


WiFiClientSecure aqiSecure;
MAX30105 particleSensor;


static const uint16_t FFT_SAMPLES = 256;
static const double EEG_SAMPLE_RATE = 475.0;


double vReal[FFT_SAMPLES];
double vImag[FFT_SAMPLES];
ArduinoFFT<double> FFT(vReal, vImag, FFT_SAMPLES, EEG_SAMPLE_RATE);


uint16_t fftIndex = 0;
unsigned long lastSampleUs = 0;


static const int PPG_BUF_LEN = 100;
uint32_t irBuffer[PPG_BUF_LEN];
uint32_t redBuffer[PPG_BUF_LEN];
int ppgIndex = 0;


float temperatureC = 0.0f;
float humidityPct = 0.0f;
int aqiValue = 0;


float bpmValue = 0.0f;
float spo2Value = 0.0f;




float alphaInternal = 0.0f;  
float betaInternal  = 0.0f;  


float alphaRaw = 5.0f;
float betaRaw  = 5.0f;


float alphaBetaRatio = 1.0f;
float betaAlphaRatio = 1.0f;


float brainPotent = 5.0f;
float mentalPressure = 5.0f;
float neuralImbalance = 5.0f;
float mentalFatigue = 5.0f;




float anxietyIndicator = 5.0f;
float exertionLevel = 5.0f;
float cognitiveImpactScore = 5.0f;
float cognitiveCardiacCorrelation = 5.0f;
float cognitiveDeclineDetection = 5.0f;
float personalizedHealthScore = 5.0f;




float neuroStress = 5.0f;
float neuroCalm = 5.0f;
float cardiacStrain = 5.0f;
float oxygenRisk = 5.0f;
float envStress = 5.0f;
float comfortScore = 5.0f;
float focusScore = 5.0f;




int currentPage = 0;
int lastClkState = HIGH;
bool lastSwState = HIGH;




float dcPrevX = 0.0f;
float dcPrevY = 0.0f;
const float dcR = 0.995f;




float lastAlphaInternal = 0.0f;
float lastBetaInternal = 0.0f;
float lastFocus = 50.0f;
uint8_t declineCounter = 0;




unsigned long lastEnvReadMs = 0;
unsigned long lastAqiMs = 0;
unsigned long lastMetricsMs = 0;
unsigned long lastDisplayMs = 0;
unsigned long lastPublishMs = 0;
unsigned long lastLedMs = 0;
unsigned long lastPpgSampleMs = 0;
unsigned long lastEncoderMs = 0;




float clamp100(float x) {
  if (x < 0.0f) return 0.0f;
  if (x > 100.0f) return 100.0f;
  return x;
}




float clampRange(float x, float low, float high) {
  if (x < low) return low;
  if (x > high) return high;
  return x;
}




float normalizeTo100(float x, float minVal, float maxVal) {
  if (x <= minVal) return 0.0f;
  if (x >= maxVal) return 100.0f;
  return 100.0f * (x - minVal) / (maxVal - minVal);
}




float inverseNormalizeTo100(float x, float minVal, float maxVal) {
  return 100.0f - normalizeTo100(x, minVal, maxVal);
}




float safeDiv(float a, float b, float fallback = 0.0f) {
  if (fabs(b) < 1e-6f) return fallback;
  return a / b;
}




float dcBlock(float x) {
  float y = x - dcPrevX + dcR * dcPrevY;
  dcPrevX = x;
  dcPrevY = y;
  return y;
}




float normalizeBandTo5_100(float x) {
  const float inMin = 0.0f;
  const float inMax = 3000.0f;   


  float y = 5.0f + ((x - inMin) * 95.0f / (inMax - inMin));
  return clampRange(y, 5.0f, 100.0f);
}


float normalizeTo5_95(float x, float inMin, float inMax) {
  float y = 5.0f + (normalizeTo100(x, inMin, inMax) * 0.90f);
  return clampRange(y, 5.0f, 95.0f);
}




void setRGB(bool r, bool g, bool b) {
  digitalWrite(PIN_LED_R, r ? HIGH : LOW);
  digitalWrite(PIN_LED_G, g ? HIGH : LOW);
  digitalWrite(PIN_LED_B, b ? HIGH : LOW);
}




void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;




  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);




  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(400);
  }
}




void connectMQTT() {
  if (mqttClient.connected()) return;




  mqttSecure.setInsecure();
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);




  while (!mqttClient.connected()) {
    mqttClient.connect("ESP32_NeuroPulse", MQTT_USER, MQTT_PASSWORD);
    if (!mqttClient.connected()) delay(1500);
  }
}




void fetchAQI() {
  if (WiFi.status() != WL_CONNECTED) return;




  aqiSecure.setInsecure();
  HTTPClient http;




  String url = "https://api.waqi.info/feed/geo:" +
               String(LATITUDE, 6) + ";" + String(LONGITUDE, 6) +
               "/?token=" + String(WAQI_TOKEN);




  if (!http.begin(aqiSecure, url)) return;




  int code = http.GET();
  if (code == 200) {
    StaticJsonDocument<4096> doc;
    if (deserializeJson(doc, http.getString()) == DeserializationError::Ok) {
      if (String((const char*)doc["status"]) == "ok") {
        aqiValue = doc["data"]["aqi"] | 0;
      }
    }
  }
  http.end();
}


void readEnvironment() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();




  if (!isnan(h)) humidityPct = h;
  if (!isnan(t)) temperatureC = t;
}




bool setupMAX30102() {
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) return false;




  particleSensor.setup(60, 4, 2, 100, 411, 4096);
  particleSensor.setPulseAmplitudeRed(0x24);
  particleSensor.setPulseAmplitudeIR(0x24);
  particleSensor.setPulseAmplitudeGreen(0);
  return true;
}




void sampleMAX30102() {
  if (millis() - lastPpgSampleMs < 25) return;
  lastPpgSampleMs = millis();




  uint32_t ir = particleSensor.getIR();
  uint32_t red = particleSensor.getRed();


  if (ir < 5000) return;




  irBuffer[ppgIndex] = ir;
  redBuffer[ppgIndex] = red;
  ppgIndex++;




  if (ppgIndex >= PPG_BUF_LEN) {
    ppgIndex = 0;




    int32_t spo2Int = 0;
    int8_t spo2ValidInt = 0;
    int32_t hrInt = 0;
    int8_t hrValidInt = 0;




    maxim_heart_rate_and_oxygen_saturation(
      irBuffer, PPG_BUF_LEN, redBuffer,
      &spo2Int, &spo2ValidInt, &hrInt, &hrValidInt
    );




    if (hrValidInt && hrInt > 40 && hrInt < 180) {
      bpmValue = 0.85f * bpmValue + 0.15f * (float)hrInt;
    }




    if (spo2ValidInt && spo2Int >= 80 && spo2Int <= 100) {
      spo2Value = 0.85f * spo2Value + 0.15f * (float)spo2Int;
    }
  }
}




void processFFT() {
  for (uint16_t i = 0; i < FFT_SAMPLES; i++) vImag[i] = 0.0;




  FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
  FFT.compute(FFTDirection::Forward);
  FFT.complexToMagnitude();




  double alphaSum = 0.0;
  double betaSum = 0.0;
  double binHz = EEG_SAMPLE_RATE / FFT_SAMPLES;




  for (uint16_t i = 1; i < FFT_SAMPLES / 2; i++) {
    double freq = i * binHz;
    double mag = vReal[i];




    if (freq >= 8.0 && freq <= 12.0) alphaSum += mag;
    if (freq >= 13.0 && freq <= 30.0) betaSum += mag;
  }




  alphaInternal = 0.8f * alphaInternal + 0.2f * (float)alphaSum;
  betaInternal  = 0.8f * betaInternal  + 0.2f * (float)betaSum;




  alphaRaw = 0.8f * alphaRaw + 0.2f * normalizeBandTo5_100(alphaInternal);
  betaRaw  = 0.8f * betaRaw  + 0.2f * normalizeBandTo5_100(betaInternal);




  alphaBetaRatio = safeDiv(alphaRaw, betaRaw + 0.001f, 1.0f);
  betaAlphaRatio = safeDiv(betaRaw, alphaRaw + 0.001f, 1.0f);
}




void sampleNeuroSignal() {
  unsigned long nowUs = micros();
  const unsigned long periodUs = (unsigned long)(1000000.0 / EEG_SAMPLE_RATE);
  if (nowUs - lastSampleUs < periodUs) return;
  lastSampleUs = nowUs;




  int16_t adc = ads.readADC_SingleEnded(0);
  float x = dcBlock((float)adc);




  vReal[fftIndex] = x;
  vImag[fftIndex] = 0.0;
  fftIndex++;




  if (fftIndex >= FFT_SAMPLES) {
    fftIndex = 0;
    processFFT();
  }
}




void handleEncoder() {
  int clkState = digitalRead(PIN_ENC_CLK);




  if (clkState != lastClkState && millis() - lastEncoderMs > 120) {
    lastEncoderMs = millis();
    int dtState = digitalRead(PIN_ENC_DT);




    if (clkState == HIGH) {
      if (dtState != clkState) currentPage = (currentPage + 1) % 3;
      else currentPage = (currentPage + 2) % 3;
    }
  }
  lastClkState = clkState;




  bool swState = digitalRead(PIN_ENC_SW);
  if (swState == LOW && lastSwState == HIGH && millis() - lastEncoderMs > 180) {
    lastEncoderMs = millis();
    currentPage = (currentPage + 1) % 3;
  }
  lastSwState = swState;
}




void computeMetrics() {
  float neuroStability = clamp100(
    100.0f - normalizeTo100(fabs(alphaInternal - lastAlphaInternal) + fabs(betaInternal - lastBetaInternal), 0.0f, 100.0f)
  );




  neuroStress = normalizeTo5_95(
    0.50f * normalizeTo100(betaAlphaRatio, 0.8f, 2.5f) +
    0.25f * inverseNormalizeTo100(neuroStability, 40.0f, 100.0f) +
    0.25f * normalizeTo100(fabs(betaRaw - alphaRaw), 0.0f, 100.0f),
    0.0f, 100.0f
  );




  neuroCalm = normalizeTo5_95(
    0.60f * normalizeTo100(alphaBetaRatio, 0.8f, 2.5f) +
    0.20f * neuroStability +
    0.20f * inverseNormalizeTo100(fabs(betaRaw - alphaRaw), 0.0f, 100.0f),
    0.0f, 100.0f
  );




  cardiacStrain = normalizeTo5_95(
    0.60f * normalizeTo100(bpmValue, 75.0f, 130.0f) +
    0.40f * inverseNormalizeTo100(spo2Value, 92.0f, 100.0f),
    0.0f, 100.0f
  );




  oxygenRisk = normalizeTo5_95(
    0.75f * inverseNormalizeTo100(spo2Value, 92.0f, 100.0f) +
    0.25f * normalizeTo100(bpmValue, 90.0f, 130.0f),
    0.0f, 100.0f
  );




  envStress = normalizeTo5_95(
    0.35f * normalizeTo100(temperatureC, 28.0f, 40.0f) +
    0.25f * normalizeTo100(humidityPct, 60.0f, 95.0f) +
    0.40f * normalizeTo100((float)aqiValue, 50.0f, 300.0f),
    0.0f, 100.0f
  );




  comfortScore = normalizeTo5_95(
    0.45f * inverseNormalizeTo100(temperatureC, 24.0f, 36.0f) +
    0.25f * inverseNormalizeTo100(humidityPct, 40.0f, 80.0f) +
    0.30f * inverseNormalizeTo100((float)aqiValue, 50.0f, 250.0f),
    0.0f, 100.0f
  );




  focusScore = normalizeTo5_95(
    0.40f * inverseNormalizeTo100(fabs(alphaRaw - betaRaw), 0.0f, 80.0f) +
    0.30f * neuroStability +
    0.20f * comfortScore +
    0.10f * inverseNormalizeTo100(bpmValue, 60.0f, 110.0f),
    0.0f, 100.0f
  );




  brainPotent = normalizeTo5_95(alphaRaw + betaRaw, 10.0f, 200.0f);
  mentalPressure = normalizeTo5_95((0.65f * neuroStress + 0.35f * cardiacStrain), 5.0f, 95.0f);
  neuralImbalance = normalizeTo5_95(fabs(betaRaw - alphaRaw), 0.0f, 95.0f);
  mentalFatigue = normalizeTo5_95(
    0.45f * (100.0f - focusScore) + 0.30f * neuroStress + 0.25f * envStress,
    0.0f, 100.0f
  );




  anxietyIndicator = normalizeTo5_95(
    0.50f * neuroStress + 0.25f * cardiacStrain + 0.25f * neuralImbalance,
    0.0f, 100.0f
  );




  exertionLevel = normalizeTo5_95(
    0.60f * cardiacStrain + 0.20f * oxygenRisk + 0.20f * envStress,
    0.0f, 100.0f
  );




  cognitiveImpactScore = normalizeTo5_95(
    0.50f * envStress + 0.50f * neuroStress,
    0.0f, 100.0f
  );




  cognitiveCardiacCorrelation = normalizeTo5_95(
    100.0f - fabs(neuroStress - cardiacStrain),
    0.0f, 100.0f
  );




  if (focusScore < lastFocus - 4.0f) {
    if (declineCounter < 10) declineCounter++;
  } else if (focusScore >= lastFocus) {
    if (declineCounter > 0) declineCounter--;
  }




  cognitiveDeclineDetection = normalizeTo5_95((float)declineCounter, 0.0f, 10.0f);




  personalizedHealthScore = normalizeTo5_95(
    0.22f * comfortScore +
    0.18f * (100.0f - neuroStress) +
    0.18f * (100.0f - cardiacStrain) +
    0.14f * (100.0f - oxygenRisk) +
    0.14f * focusScore +
    0.14f * neuroCalm,
    0.0f, 100.0f
  );




  lastAlphaInternal = alphaInternal;
  lastBetaInternal = betaInternal;
  lastFocus = focusScore;
}


void updateRGBState() {
  if (millis() - lastLedMs < 5000) return;
  lastLedMs = millis();




  const float margin = 3.0f;
  if (alphaRaw > betaRaw + margin) setRGB(false, true, false);
  else if (betaRaw > alphaRaw + margin) setRGB(true, false, false);
  else setRGB(true, true, false);
}


void drawPage0() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(8, 8);
  tft.println("ENVIRONMENT");
  tft.setCursor(8, 45);  tft.printf("Temp: %.1f C", temperatureC);
  tft.setCursor(8, 75);  tft.printf("Hum : %.1f %%", humidityPct);
  tft.setCursor(8, 105); tft.printf("AQI : %d", aqiValue);
}




void drawPage1() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(8, 8);
  tft.println("HEART / SPO2");
  tft.setCursor(8, 45); tft.printf("HR   : %.1f BPM", bpmValue);
  tft.setCursor(8, 75); tft.printf("SpO2 : %.1f %%", spo2Value);
}




void drawPage2() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(8, 8);
  tft.println("ALPHA / BETA");
  tft.setCursor(8, 45); tft.printf("Alpha: %.1f", alphaRaw);
  tft.setCursor(8, 75); tft.printf("Beta : %.1f", betaRaw);
}




void updateDisplay() {
  if (millis() - lastDisplayMs < 500) return;
  lastDisplayMs = millis();




  if (currentPage == 0) drawPage0();
  else if (currentPage == 1) drawPage1();
  else drawPage2();
}




template <size_t N>
void publishJson(const char* topic, StaticJsonDocument<N>& doc) {
  char buffer[1024];
  size_t n = serializeJson(doc, buffer, sizeof(buffer));
  if (n > 0) {
    mqttClient.publish(topic, (const uint8_t*)buffer, n, true);
  }
}




void publishAll() {
  if (millis() - lastPublishMs < 5000) return;
  lastPublishMs = millis();




  StaticJsonDocument<256> envDoc;
  envDoc["temp"] = temperatureC;
  envDoc["hum"] = humidityPct;
  envDoc["aqi"] = aqiValue;
  publishJson("NeuroPulse/environment", envDoc);




  StaticJsonDocument<256> ppgDoc;
  ppgDoc["bpm"] = bpmValue;
  ppgDoc["spo2"] = spo2Value;
  publishJson("NeuroPulse/max30102", ppgDoc);




  StaticJsonDocument<256> rawDoc;
  rawDoc["alpha_raw"] = alphaRaw;          
  rawDoc["beta_raw"] = betaRaw;            
  rawDoc["alpha_internal"] = alphaInternal;
  rawDoc["beta_internal"] = betaInternal;  
  rawDoc["alpha_beta_ratio"] = alphaBetaRatio;
  rawDoc["beta_alpha_ratio"] = betaAlphaRatio;
  publishJson("NeuroPulse/neuro/raw", rawDoc);




  StaticJsonDocument<768> metricDoc;
  metricDoc["brain_potent"] = brainPotent;
  metricDoc["mental_pressure"] = mentalPressure;
  metricDoc["neural_imbalance"] = neuralImbalance;
  metricDoc["mental_fatigue"] = mentalFatigue;
  metricDoc["anxiety_indicator"] = anxietyIndicator;
  metricDoc["exertion_level"] = exertionLevel;
  metricDoc["cognitive_impact_score"] = cognitiveImpactScore;
  metricDoc["cognitive_cardiac_correlation"] = cognitiveCardiacCorrelation;
  metricDoc["cognitive_decline_detection"] = cognitiveDeclineDetection;
  metricDoc["personalized_health_score"] = personalizedHealthScore;
  publishJson("NeuroPulse/metrics", metricDoc);




  StaticJsonDocument<64> pageDoc;
  pageDoc["page"] = currentPage;
  publishJson("NeuroPulse/device/page", pageDoc);
}




void setup() {
  Serial.begin(115200);
  delay(500);




  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  setRGB(true, true, false);




  pinMode(PIN_ENC_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC_DT, INPUT);
  pinMode(PIN_ENC_SW, INPUT);




  dht.begin();
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);




  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 20);
  tft.println("Booting...");
  delay(1000);




  if (!ads.begin()) {
    tft.fillScreen(TFT_RED);
    tft.setCursor(10, 20);
    tft.println("ADS1115 fail");
    while (true) delay(100);
  }




  ads.setGain(GAIN_ONE);
  ads.setDataRate(RATE_ADS1115_475SPS);




  setupMAX30102();




  mqttClient.setBufferSize(1024);  
  connectWiFi();
  connectMQTT();




  fetchAQI();
  readEnvironment();
  drawPage0();
}




void loop() {
  connectWiFi();
  connectMQTT();
  mqttClient.loop();




  sampleNeuroSignal();
  sampleMAX30102();
  handleEncoder();




  if (millis() - lastEnvReadMs >= 5000) {
    lastEnvReadMs = millis();
    readEnvironment();
  }




  if (millis() - lastAqiMs >= 300000UL || lastAqiMs == 0) {
    lastAqiMs = millis();
    fetchAQI();
  }




  if (millis() - lastMetricsMs >= 1000) {
    lastMetricsMs = millis();
    computeMetrics();
  }




  updateRGBState();
  updateDisplay();
  publishAll();
}







