# ROBOCAR AI Gemini

Robot mobil berbasis ESP32 yang dapat menerima perintah suara dalam Bahasa Indonesia, mengubah suara menjadi teks menggunakan Deepgram Speech-to-Text, kemudian memanfaatkan Google Gemini AI untuk memahami perintah dan mengendalikan pergerakan robot secara otomatis.

## Features

* 🎤 Voice Command Recognition
* 🤖 AI Command Understanding menggunakan Google Gemini
* 📡 WiFi Connectivity
* 🚗 Kendali Motor DC (Maju, Mundur, Spin, Stop)
* ⚡ Pengaturan Kecepatan (Normal dan Cepat)
* 📏 Obstacle Detection menggunakan 4 sensor ultrasonik HC-SR04
* 🔄 Automatic Obstacle Avoidance
* 🔘 Aktivasi melalui tombol atau sinyal eksternal receiver
* 🎙️ Perekaman audio menggunakan mikrofon I2S

---

## System Architecture

```text
Voice Input
     │
     ▼
I2S Microphone
     │
     ▼
ESP32
     │
     ▼
Deepgram Speech-to-Text
     │
     ▼
Transcribed Text
     │
     ▼
Google Gemini AI
     │
     ▼
Command Parsing
     │
     ▼
Motor Controller
     │
     ▼
Robot Movement
```

---

## Supported Commands

Gemini akan mengubah perintah suara menjadi salah satu command berikut:

| Voice Command | Robot Action             |
| ------------- | ------------------------ |
| MAJU          | Robot bergerak maju      |
| MUNDUR        | Robot bergerak mundur    |
| STOP          | Robot berhenti           |
| SPIN          | Robot berputar di tempat |
| CEPAT         | Kecepatan maksimum       |
| LAMBAT        | Kecepatan normal         |

Jika perintah tidak dikenali, robot akan mengabaikannya.

---

## Hardware Requirements

### Main Controller

* ESP32 Dev Board

### Sensors

* 4x HC-SR04 Ultrasonic Sensor
* 1x I2S Microphone (INMP441 atau kompatibel)

### Actuators

* Dual DC Motor
* Motor Driver

### Others

* Push Button
* LED Indicator
* WiFi Connection
* Battery Pack

---

## Pin Configuration

### Control

| Function       | GPIO |
| -------------- | ---- |
| Button         | 35   |
| Receiver Input | 34   |
| LED            | 33   |

### Ultrasonic Sensors

| Sensor | Trigger | Echo |
| ------ | ------- | ---- |
| Front  | 15      | 2    |
| Left   | 4       | 16   |
| Right  | 17      | 5    |
| Rear   | 18      | 19   |

### Motor Driver

| Function | GPIO |
| -------- | ---- |
| ENA      | 13   |
| IN1      | 12   |
| IN2      | 14   |
| IN3      | 27   |
| IN4      | 26   |
| ENB      | 25   |

### I2S Microphone

| Function | GPIO |
| -------- | ---- |
| WS       | 21   |
| SD       | 23   |
| SCK      | 22   |

---

## Software Requirements

### Arduino IDE

Install:

* ESP32 Board Package
* ArduinoJson Library

### Required Accounts

#### Deepgram API

Create an account:

https://deepgram.com

Get your API Key and replace:

```cpp
const char* deepgramApiKey = "YOUR_DEEPGRAM_API_KEY";
```

#### Google Gemini API

Create API Key:

https://ai.google.dev

Replace:

```cpp
const char* Gemini_Token = "YOUR_GEMINI_API_KEY";
```

---

## WiFi Setup

Replace:

```cpp
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
```

with your own WiFi credentials.

---

## Installation

### Clone Repository

```bash
git clone https://github.com/yourusername/robocar-ai-gemini.git
```

### Open Project

Open:

```text
ROBOCAR_AI_Gemini.ino
```

using Arduino IDE.

### Configure

Fill:

* WiFi SSID
* WiFi Password
* Deepgram API Key
* Gemini API Key

### Upload

1. Select ESP32 Board
2. Select COM Port
3. Upload Sketch

---

## Robot Workflow

1. User presses button or receiver sends trigger signal.
2. ESP32 records audio for approximately 2 seconds.
3. Audio is sent to Deepgram.
4. Deepgram returns speech transcription.
5. Text is sent to Gemini AI.
6. Gemini determines intended command.
7. ESP32 executes command.
8. Ultrasonic sensors continuously monitor obstacles.
9. Robot automatically adjusts direction if an obstacle is detected.

---

## Obstacle Avoidance Logic

### Forward Mode

* Front clear → Move forward
* Front blocked + left clear → Turn left
* Front blocked + right clear → Turn right

### Reverse Mode

* Rear clear → Move backward
* Rear blocked → Search for safer direction

---

## Future Improvements

* Camera-based object detection
* Face recognition
* Autonomous navigation
* GPS integration
* Local speech recognition (offline)
* Mobile application control
* Battery monitoring system

---

## Author

Feezzyapping

Built with:

* ESP32
* Deepgram Speech-to-Text
* Google Gemini AI
* Arduino IDE

---

## License

This project is intended for educational and research purposes.
Feel free to modify and improve it.

```
```
