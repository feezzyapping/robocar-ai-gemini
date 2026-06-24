/* INMP441 Speech To Text
   Jun 19, 2025 2:20 AM
   By : Ya'afi Ft ChatGPT
*/
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "FS.h"
#include <SPIFFS.h>
#include "driver/i2s.h"

#define BUTTON_PIN 4
#define ledd 2
#define led 23
bool ka = false;
bool mic_state = false;
bool ask_gemini = false;  // ask gemini once the record has done  // tanya gemini saat record selesai

// WiFi credentials
const char* ssid = "Infinix NOTE 30";
const char* password = "12345678";

// API Declaration
const char* elevenlabsApiKey = "sk_3f90e924b4e9788f4e2d8991ee15bdaa4335d4593ad1bc8a"; // ElevenLabs API
const char* Gemini_Token = "AIzaSyCBqDtD6TVQm8CNz5-lsUg1C7bfCTex9NM";    // Gemini API
const char* Gemini_Max_Tokens = "100";

// Transcript ElevenLabs
String data_mic = "";

// I2S config
#define I2S_WS 5
#define I2S_SD 18
#define I2S_SCK 19
#define I2S_PORT I2S_NUM_0
#define SAMPLE_RATE 15000
#define SAMPLE_BITS I2S_BITS_PER_SAMPLE_16BIT
#define RECORD_TIME_SECONDS 2
#define BUFFER_SIZE (SAMPLE_RATE * RECORD_TIME_SECONDS)

byte header[44];
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

void recordAudio(const char* filename) {
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Gagal buka file!");
    mic_state = false;
    return;
  }
  // simpan pointer ke posisi data
  file.write(header, 44);

  size_t bytesRead;
  int totalBytes = 0;
  int16_t buffer[512];

  unsigned long startTime = millis();
  Serial.println("Recording...");

  while (millis() - startTime < RECORD_TIME_SECONDS * 1000) {
    i2s_read(I2S_PORT, (void*)buffer, sizeof(buffer), &bytesRead, portMAX_DELAY);
    Serial.print("."); Serial.println(bytesRead); // tambahkan ini
    file.write((uint8_t*)buffer, bytesRead);
    totalBytes += bytesRead;
  }
  Serial.printf("\nTotal bytes recorded: %d\n", totalBytes);

  Serial.println("Recording done!");

  // Update header dengan ukuran data sebenarnya
  file.seek(0);
  createWavHeader(header, totalBytes);
  file.write(header, 44);
  file.close();
}

void sendToElevenLabs(const char* filename) {
  File file = SPIFFS.open(filename, FILE_READ);
  if (!file) {
    Serial.println("❌ File tidak ditemukan!");
    return;
  }

  const String boundary = "----ESP32FormBoundary7MA4YWxkTrZu0gW";

  // =========== Bagian awal multipart =============
  String body_start =
    "--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"model_id\"\r\n\r\n"
    "scribe_v1\r\n"
    "--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"language_code\"\r\n\r\n"
    "ind\r\n"
    "--" + boundary + "\r\n"
    "Content-Disposition: form-data; name=\"file\"; filename=\"yapping.wav\"\r\n"
    "Content-Type: audio/wav\r\n\r\n";

  // =========== Bagian akhir multipart =============
  String body_end = "\r\n--" + boundary + "--\r\n";

  // Hitung total panjang upload
  size_t content_length =
    body_start.length() +
    file.size() +
    body_end.length();

  WiFiClientSecure client;
  client.setInsecure();

  if (!client.connect("api.elevenlabs.io", 443)) {
    Serial.println("❌ Gagal connect ke server");
    file.close();
    return;
  }

  // ====================== HEADER ======================
  client.printf(
    "POST /v1/speech-to-text HTTP/1.1\r\n"
    "Host: api.elevenlabs.io\r\n"
    "xi-api-key: %s\r\n"
    "Content-Type: multipart/form-data; boundary=%s\r\n"
    "Content-Length: %d\r\n"
    "Connection: close\r\n\r\n",
    elevenlabsApiKey,
    boundary.c_str(),
    content_length
  );

  // ===================== KIRIM BODY START =============
  client.print(body_start);

  // ===================== KIRIM FILE ====================
  uint8_t buffer[512];
  while (file.available()) {
    size_t r = file.read(buffer, sizeof(buffer));
    client.write(buffer, r);
  }
  file.close();

  // ====================== KIRIM BODY END ===============
  client.print(body_end);

  Serial.println("📤 Data terkirim! Menunggu respons...");

  // ====================== BACA RESPON ==================
  String response = "";
  while (client.connected() || client.available()) {
    if (client.available()) {
      response += client.readString();
    }
  }

  Serial.println("🔽 Response:");
  Serial.println(response);

  // Pisahkan header & body
  int idx = response.indexOf("\r\n\r\n");
  if (idx < 0) {
    Serial.println("❌ Body tidak ditemukan!");
    return;
  }

  String body = response.substring(idx + 4);

  DynamicJsonDocument doc(4096);
  auto err = deserializeJson(doc, body);
  if (err) {
    Serial.println("❌ JSON parsing error:");
    Serial.println(err.c_str());
    return;
  }

  // Ambil teks hasil STT
  data_mic = doc["text"] | "";
  // hapus balasan dalam kurung : (berdeham), (tertawa), (suara hewan)
  while (true) {
    int start = data_mic.indexOf('(');
    int end   = data_mic.indexOf(')', start);

    if (start == -1 || end == -1) break;

    data_mic.remove(start, end - start + 1);
  }
  // Hilangkan spasi ganda
  data_mic.replace("  ", " ");
  data_mic.trim();
  // ========================================================
  Serial.print("🟢 Transkripsi: ");
  Serial.println(data_mic);
  if(data_mic != "") ask_gemini = true;

  file.close();
  mic_state = false;
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

  /*
  if(Answer == "MAJU") motor_state = 1;
  else if(Answer == "MUNDUR") motor_state = 2;
  else if(Answer == "SPIN") motor_state = 3;
  else if(Answer == "STOP") motor_state = 4;
  else if(Answer == "CEPAT") speed_state = true;
  else if(Answer == "LAMBAT") speed_state = false; */

  data_mic = "";
  ask_gemini = false;
}

void createWavHeader(uint8_t* header, int dataSize) {
  int fileSize = dataSize + 36;
  int sampleRate = 16000;  // atau 44100 tergantung setting I2S kamu
  int byteRate = sampleRate * 2; // mono 16-bit

  header[0] = 'R'; header[1] = 'I'; header[2] = 'F'; header[3] = 'F';
  header[4] = fileSize & 0xff;
  header[5] = (fileSize >> 8) & 0xff;
  header[6] = (fileSize >> 16) & 0xff;
  header[7] = (fileSize >> 24) & 0xff;
  header[8] = 'W'; header[9] = 'A'; header[10] = 'V'; header[11] = 'E';
  header[12] = 'f'; header[13] = 'm'; header[14] = 't'; header[15] = ' ';
  header[16] = 16; header[17] = 0; header[18] = 0; header[19] = 0;
  header[20] = 1; header[21] = 0;
  header[22] = 1; header[23] = 0;
  header[24] = sampleRate & 0xff;
  header[25] = (sampleRate >> 8) & 0xff;
  header[26] = (sampleRate >> 16) & 0xff;
  header[27] = (sampleRate >> 24) & 0xff;
  header[28] = byteRate & 0xff;
  header[29] = (byteRate >> 8) & 0xff;
  header[30] = (byteRate >> 16) & 0xff;
  header[31] = (byteRate >> 24) & 0xff;
  header[32] = 2; header[33] = 0;
  header[34] = 16; header[35] = 0;
  header[36] = 'd'; header[37] = 'a'; header[38] = 't'; header[39] = 'a';
  header[40] = dataSize & 0xff;
  header[41] = (dataSize >> 8) & 0xff;
  header[42] = (dataSize >> 16) & 0xff;
  header[43] = (dataSize >> 24) & 0xff;
}

// ===================== I2S Task (CORE 0) =====================

void i2sTask(void* param) {
  while (true) {
    if (mic_state) {
      recordAudio("/yapping.wav");
      sendToElevenLabs("/yapping.wav");
    }
    if (ask_gemini) sendToGemini();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// ======================= Arduino ============================

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(led, OUTPUT);
  pinMode(ledd, OUTPUT);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed!");
    return;
  }

  setupWiFi();
  setupI2SMic();

  // Jalankan I2S di core 0
  xTaskCreatePinnedToCore(i2sTask, "I2S Task", 8192, NULL, 2, NULL, 0);
}

void loop() {
  // Deteksi tombol
  if (digitalRead(BUTTON_PIN) == HIGH && !ka) {
    ka = true;
    mic_state = true;
  } else if (digitalRead(BUTTON_PIN) == LOW && ka) {
    ka = false;
  }
  digitalWrite(ledd, mic_state);
  if (data_mic == "Selamat pagi") digitalWrite(led, HIGH);
  else if (data_mic == "Selamat malam") digitalWrite(led, LOW);
}
