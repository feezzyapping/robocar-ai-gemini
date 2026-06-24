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

#define BUTTON_PIN 35                 // remote pin for switch direction of the motor
#define led 33                     // remote indicator
bool motor_state = false;          // false = reverse, true = forward
uint8_t rotate_state = 0;          // 0 = forward, 1 = right, 2 = left
bool ka = false;                   // button locking logic
int interval_read = 80;            // interval reading sensor
int interval_rotate = 400;         // interval for calibrating the motor to rotate 90 degrees
unsigned long previous_read = 0;   // sensor
unsigned long previous_rotate = 0; // motor

int distance1 = 0;
int distance2 = 0;
int distance3 = 0;
int distance4 = 0;


void setup() {
  // put your setup code here, to run once:
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

ledcAttach(ena, 1000, 8);
ledcAttach(enb, 1000, 8);

pinMode(BUTTON_PIN, INPUT);
pinMode(led, OUTPUT);

digitalWrite(trg1, LOW);
digitalWrite(trg2, LOW);
digitalWrite(trg3, LOW);
digitalWrite(trg4, LOW);

Serial.begin(115200);

delay(500);
}

void loop() {
  // put your main code here, to run repeatedly:
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

  if(duration1 != 0 && duration2 != 0 && duration3 != 0  && duration4 != 0) {
  distance1 = duration1 * 0.034 / 2;
  distance2 = duration2 * 0.034 / 2;
  distance3 = duration3 * 0.034 / 2;
  distance4 = duration4 * 0.034 / 2;
  }
}
if (digitalRead(BUTTON_PIN) == HIGH && !ka) {
  ka = true;
  //digitalWrite(led, HIGH);
  motor_state = !motor_state;
} else if (digitalRead(BUTTON_PIN) == LOW && ka) {
  ka = false;
  //digitalWrite(led, LOW);
}
// ===================== MAJU =====================
if (motor_state == 1 && rotate_state == 0) {
  if (distance1 >= 30) {  // maju
    ledcWrite(ena, 158);
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    ledcWrite(enb, 158);
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
  }
  else if (distance1 <= 30 && distance2 <= 20 && distance3 >= 20) {  // hadap kiri
    rotate_state = 2;
    previous_rotate = now;
  }
  else if (distance1 <= 30 && distance2 >= 20 && distance3 <= 20) {  // hadap kanan
    rotate_state = 1;
    previous_rotate = now;
  }
  else if (distance1 <= 30 && distance2 >= 20 && distance3 >= 20) {  // hadap kanan
    rotate_state = 1;
    previous_rotate = now;
  }
}
// ===================== MUNDUR =====================
else if (motor_state == 0 && rotate_state == 0) {
  if (distance4 >= 30) {  // mundur
    ledcWrite(ena, 158);
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    ledcWrite(enb, 158);
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
  }
  else if (distance4 <= 30 && distance2 <= 20 && distance3 >= 20) {  // hadap kanan
    previous_rotate = now;
    rotate_state = 1;
  }
  else if (distance4 <= 30 && distance2 >= 20 && distance3 <= 20) {  // hadap kiri
    previous_rotate = now;
    rotate_state = 2;
  }
  else if (distance4 <= 30 && distance2 >= 20 && distance3 >= 20) { // hadap kiri
    previous_rotate = now;
    rotate_state = 2;
  }
}

if (rotate_state == 1) {
  if(now - previous_rotate >= interval_rotate) rotate_state = 0;
  ledcWrite(ena, 255);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  ledcWrite(enb, 255);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
}
else if (rotate_state == 2) {
  if(now - previous_rotate >= interval_rotate) rotate_state = 0;
  ledcWrite(ena, 255);
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  ledcWrite(enb, 255);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
}

if(distance1 <= 30 && distance2 <= 30 && distance3 <= 30 && distance4 <= 30) digitalWrite(led, HIGH);
else digitalWrite(led, LOW);
Serial.print(distance1);
Serial.print("  ");
Serial.print(distance2);
Serial.print("  ");
Serial.print(distance3);
Serial.print("  ");
Serial.print(distance4);
Serial.print("  ");
Serial.println(motor_state);
}