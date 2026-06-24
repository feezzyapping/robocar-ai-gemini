/* ROBOCAR AI Gemini
   Jun 19, 2025 11:26 PM
   By : Ya'afi Ft ChatGPT
*/
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "driver/i2s.h"

#define BUTTON_PIN 35
#define RCX_PIN 34
#define led 33

#define trg1 15
#define ech1 2
#define trg2 4
#define ech2 16
#define trg3 17
#define ech3 5
#define trg4 18
#define ech4 19

#define ena 13
#define in1 12
#define in2 14
#define in3 27
#define in4 26
#define enb 25
int distance1 = 0;
int distance2 = 0;
int distance3 = 0;
int distance4 = 0;

bool ka = false;          // locking button logic  // logika button pengunci
bool mic_state = false;   // start recording once the receiver got a signal  // mulai record saat receiver dapat sinyal
bool ask_gemini = false;  // ask gemini once the record has done  // tanya gemini saat record selesai

uint8_t motor_state = 0;            // 0 = stop, 1 = forward, 2 = reverse, 3 = spin
bool speed_state = false;           // false = normal (158), true = fast (255)
uint8_t rotate_state = 0;           // 0 = forward, 1 = right, 2 = left
int interval_read = 80;             // interval reading sensor
int interval_rotate = 500;          // interval for calibrating the motor to rotate 90 degrees
unsigned long previous_read = 0;    // sensor
unsigned long previous_rotate = 0;  // motor

// WiFi credentials
const char* ssid = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";  // fill with your ssid
const char* password = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";  // fill with your password

// API Key
const char* deepgramApiKey = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";   // Deepgram API
const char* Gemini_Token = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";     // Gemini API
const char* Gemini_Max_Tokens = "100";

// Answer parsing from API
String data_mic = "";

// I2S config
#define I2S_WS 21
#define I2S_SD 23
#define I2S_SCK 22
#define I2S_PORT I2S_NUM_0
#define SAMPLE_RATE 16000
#define SAMPLE_BITS I2S_BITS_PER_SAMPLE_16BIT
#define RECORD_TIME_SECONDS 2
#define BUFFER_SIZE (SAMPLE_RATE * RECORD_TIME_SECONDS)

int16_t audioBuffer[BUFFER_SIZE];

void setupWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
}

void setupI2SMic() {
  const i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = SAMPLE_BITS,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 512,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
}

void recordAudio() {
  size_t bytesRead;
  Serial.println("Recording...");
  i2s_read(I2S_PORT, (void*)audioBuffer, sizeof(audioBuffer), &bytesRead, portMAX_DELAY);
  for (int i = 0; i < BUFFER_SIZE; i++) {
    audioBuffer[i] = constrain(audioBuffer[i] * 4, -32768, 32767);  // Faktor 2x (bisa diuji-coba)
  }
  Serial.println("Recording done!");
  mic_state = false;
}

void sendToDeepgram() {
  Serial.println("Sending to Deepgram...");

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  https.begin(client, "https://api.deepgram.com/v1/listen?model=nova-2&language=id&smart_format=true");
  https.addHeader("Content-Type", "audio/wav");
  https.addHeader("Authorization", "Token " + String(deepgramApiKey));

  // Gabungkan WAV header dan audio
  int totalSize = sizeof(audioBuffer) + 44;
  uint8_t* data = (uint8_t*)malloc(totalSize);
  if (!data) {
    Serial.println("Memory allocation failed!");
    return;
  }

  createWavHeader(data, sizeof(audioBuffer));
  memcpy(data + 44, audioBuffer, sizeof(audioBuffer));

  int httpCode = https.POST(data, totalSize);

  if (httpCode > 0) {
    String response = https.getString();
    Serial.println("Response:");
    Serial.println(response);

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response, DeserializationOption::NestingLimit(20));

    if (!error) {
      const char* transcript = doc["results"]["channels"][0]["alternatives"][0]["transcript"];
      Serial.print("Transcript : ");
      Serial.println(transcript);
      data_mic = transcript;
      if (data_mic != "") ask_gemini = true;
    } else {
      Serial.println("JSON parsing error!");
      Serial.print("Error detail : ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.printf("HTTP failed, error: %s\n", https.errorToString(httpCode).c_str());
  }

  free(data);
  https.end();
}

void sendToGemini() {
  WiFiClientSecure client;
  client.setInsecure();

  data_mic = "\"Pesan : " + data_mic + ". Balas pesan itu hanya dengan 1 balasan pada list : MAJU;MUNDUR;STOP;CEPAT;LAMBAT;SPIN sesuai dengan pesan. Jika pesan tidak cocok, balas NOTHING.\"";
  Serial.println("");
  Serial.print("Sending question : ");
  Serial.println(data_mic);

  HTTPClient https;
  String Answer = "";

  //Serial.print("[HTTPS] begin...\n");
  if (https.begin("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=" + (String)Gemini_Token)) {  // HTTPS

    https.addHeader("Content-Type", "application/json");
    String payload = String("{\"contents\": [{\"parts\":[{\"text\":" + data_mic + "}]}],\"generationConfig\": {\"maxOutputTokens\": " + (String)Gemini_Max_Tokens + "}}");

    //Serial.print("[HTTPS] GET...\n");

    // start connection and send HTTP header
    int httpCode = https.POST(payload);

    // httpCode will be negative on error
    // file found at server

    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      String payload = https.getString();
      //Serial.println(payload);

      DynamicJsonDocument doc(1024);


      deserializeJson(doc, payload);
      const char* teks = doc["candidates"][0]["content"]["parts"][0]["text"];
      Answer = teks;

      // For Filtering our Special Characters, WhiteSpaces and NewLine Characters
      Answer.trim();
      String filteredAnswer = "";
      for (size_t i = 0; i < Answer.length(); i++) {
        char c = Answer[i];
        if (isalnum(c) || isspace(c)) {
          filteredAnswer += c;
        } else {
          filteredAnswer += ' ';
        }
      }
      Answer = filteredAnswer;

      Serial.println("");
      Serial.println("Answer from Gemini AI : ");
      Serial.println("");
      Serial.println(Answer);
    } else {
      Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }

  if (Answer == "MAJU") motor_state = 1;
  else if (Answer == "MUNDUR") motor_state = 2;
  else if (Answer == "SPIN") motor_state = 3;
  else if (Answer == "STOP") motor_state = 4;
  else if (Answer == "CEPAT") speed_state = true;
  else if (Answer == "LAMBAT") speed_state = false;

  data_mic = "";
  ask_gemini = false;
}

void createWavHeader(uint8_t* header, int dataSize) {
  const int totalSize = dataSize + 36;

  memcpy(header, "RIFF", 4);
  *(int32_t*)(header + 4) = totalSize;
  memcpy(header + 8, "WAVEfmt ", 8);
  *(int32_t*)(header + 16) = 16;
  *(int16_t*)(header + 20) = 1;  // PCM
  *(int16_t*)(header + 22) = 1;  // mono
  *(int32_t*)(header + 24) = SAMPLE_RATE;
  *(int32_t*)(header + 28) = SAMPLE_RATE * 2;
  *(int16_t*)(header + 32) = 2;
  *(int16_t*)(header + 34) = 16;
  memcpy(header + 36, "data", 4);
  *(int32_t*)(header + 40) = dataSize;
}

// ===================== I2S Task (CORE 0) =====================

void i2sTask(void* param) {
  while (true) {
    if (mic_state) {
      recordAudio();
      sendToDeepgram();
    }

    if (ask_gemini) sendToGemini();

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// ======================= Arduino ============================

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(RCX_PIN, INPUT);
  pinMode(led, OUTPUT);

  pinMode(trg1, OUTPUT);
  pinMode(ech1, INPUT);
  pinMode(trg2, OUTPUT);
  pinMode(ech2, INPUT);
  pinMode(trg3, OUTPUT);
  pinMode(ech3, INPUT);
  pinMode(trg4, OUTPUT);
  pinMode(ech4, INPUT);

  pinMode(ena, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  pinMode(enb, OUTPUT);

  ledcAttach(ena, 1000, 8);  // pin, frequency, bit resolution
  ledcAttach(enb, 1000, 8);  // pin, frequency, bit resolution

  digitalWrite(trg1, LOW);
  digitalWrite(trg2, LOW);
  digitalWrite(trg3, LOW);
  digitalWrite(trg4, LOW);

  setupWiFi();
  setupI2SMic();

  // Jalankan I2S di core 0
  xTaskCreatePinnedToCore(i2sTask, "I2S Task", 8192, NULL, 2, NULL, 0);
  delay(500);
}

void loop() {
  unsigned long now = millis();

  if (now - previous_read >= interval_read) {
    digitalWrite(trg1, HIGH);
    delayMicroseconds(10);
    digitalWrite(trg1, LOW);
    long duration1 = pulseIn(ech1, HIGH, 20000);  // pin, state, timeout

    digitalWrite(trg2, HIGH);
    delayMicroseconds(10);
    digitalWrite(trg2, LOW);
    long duration2 = pulseIn(ech2, HIGH, 20000);  // pin, state, timeout

    digitalWrite(trg3, HIGH);
    delayMicroseconds(10);
    digitalWrite(trg3, LOW);
    long duration3 = pulseIn(ech3, HIGH, 20000);  // pin, state, timeout

    digitalWrite(trg4, HIGH);
    delayMicroseconds(10);
    digitalWrite(trg4, LOW);
    long duration4 = pulseIn(ech4, HIGH, 20000);  // pin, state, timeout

    if (duration1 != 0 && duration2 != 0 && duration3 != 0 && duration4 != 0) {
      distance1 = duration1 * 0.034 / 2;
      distance2 = duration2 * 0.034 / 2;
      distance3 = duration3 * 0.034 / 2;
      distance4 = duration4 * 0.034 / 2;
    }
  }
  // ===================== MAJU =====================
  if (motor_state == 1 && rotate_state == 0) {
    if (distance1 >= 30) {  // maju
      if (!speed_state) ledcWrite(ena, 158);
      else if (speed_state) ledcWrite(enb, 255);
      digitalWrite(in1, HIGH);
      digitalWrite(in2, LOW);
      if (!speed_state) ledcWrite(ena, 158);
      else if (speed_state) ledcWrite(enb, 255);
      digitalWrite(in3, HIGH);
      digitalWrite(in4, LOW);
    } else if (distance1 <= 30 && distance2 <= 20 && distance3 >= 20) {  // hadap kiri
      rotate_state = 2;
      previous_rotate = now;
    } else if (distance1 <= 30 && distance2 >= 20 && distance3 <= 20) {  // hadap kanan
      rotate_state = 1;
      previous_rotate = now;
    } else if (distance1 <= 30 && distance2 >= 20 && distance3 >= 20) {  // hadap kanan
      rotate_state = 1;
      previous_rotate = now;
    }
  }
  // ===================== MUNDUR =====================
  else if (motor_state == 2 && rotate_state == 0) {
    if (distance4 >= 30) {  // mundur
      if (!speed_state) ledcWrite(ena, 158);
      else if (speed_state) ledcWrite(enb, 255);
      digitalWrite(in1, LOW);
      digitalWrite(in2, HIGH);
      if (!speed_state) ledcWrite(ena, 158);
      else if (speed_state) ledcWrite(enb, 255);
      digitalWrite(in3, LOW);
      digitalWrite(in4, HIGH);
    } else if (distance4 <= 30 && distance2 <= 20 && distance3 >= 20) {  // hadap kanan
      rotate_state = 1;
      previous_rotate = now;
    } else if (distance4 <= 30 && distance2 >= 20 && distance3 <= 20) {  // hadap kiri
      rotate_state = 2;
      previous_rotate = now;
    } else if (distance4 <= 30 && distance2 >= 20 && distance3 >= 20) {  // hadap kiri
      rotate_state = 2;
      previous_rotate = now;
    }
  }
  // ===================== SPIN =====================
  else if (motor_state == 3) {
    ledcWrite(ena, 158);
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    ledcWrite(enb, 158);
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
  }
  // ===================== STOP =====================
  else if (motor_state == 0) {
    ledcWrite(ena, 0);
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    ledcWrite(enb, 0);
    digitalWrite(in3, LOW);
    digitalWrite(in4, LOW);
  }

  if (rotate_state == 1) {
    if (now - previous_rotate >= interval_rotate) rotate_state = 0;
    ledcWrite(ena, 255);
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    ledcWrite(enb, 255);
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
  } else if (rotate_state == 2) {
    if (now - previous_rotate >= interval_rotate) rotate_state = 0;
    ledcWrite(ena, 255);
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    ledcWrite(enb, 255);
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
  }

  if (digitalRead(RCX_PIN) == HIGH) mic_state = true;
  // Deteksi tombol
  if (digitalRead(BUTTON_PIN) == HIGH && !ka) {
    ka = true;
    mic_state = true;
  } else if (digitalRead(BUTTON_PIN) == LOW && ka) {
    ka = false;
  }

  // if (digitalRead(RCX_PIN) == HIGH) mic_state = true;
  digitalWrite(led, mic_state);
}
