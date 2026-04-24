// SEMANGAT KAKKKKKKK
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= WIFI =================
const char* ssid = "HUAWEI-2.4G-bUU5";
const char* password = "indonesia";
// const char* ssid = "Naura";
// const char* password = "bogel1922";

// ================= TELEGRAM =================
#define BOT_TOKEN "8484351035:AAHPK9O1YKlYjcjm39ZGG3iFW_Go9eYSqPc"
#define CHAT_ID "6027487871"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// ================= ESP32-CAM =================
const char* camURL = "http://192.168.100.224/capture";

// ================= PIN =================
#define PIR_PIN 13
#define TRIG_PIN 12
#define ECHO_PIN 14

#define SERVO_ATAS_1 27
#define SERVO_ATAS_2 26

Servo servoAtas1;
Servo servoAtas2;

int counter = 0;
const int batasJarak = 15;

// ================= FUNGSI =================
float bacaJarak() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long durasi = pulseIn(ECHO_PIN, HIGH);
  return durasi * 0.034 / 2;
}

bool triggerCamera() {
  HTTPClient http;
  http.begin(camURL);
  int code = http.GET();
  http.end();
  return (code == 200);
}

void kirimNotif(String pesan) {
  bot.sendMessage(CHAT_ID, pesan, "");
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("System Booting");

  pinMode(PIR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // 🔥 FIX PWM STABIL
  servoAtas1.setPeriodHertz(50);
  servoAtas1.attach(SERVO_ATAS_1, 500, 2400);

  servoAtas2.setPeriodHertz(50);
  servoAtas2.attach(SERVO_ATAS_2, 500, 2400);

  // posisi awal tutup
  servoAtas1.write(0);
  servoAtas2.write(180);

  delay(500);

  WiFi.begin(ssid, password);
  client.setInsecure();

  lcd.setCursor(0,1);
  lcd.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("WiFi Connected");

  delay(30000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("System Ready");
  lcd.setCursor(0,1);
  lcd.print("Menunggu...");
}

// ================= LOOP =================
void loop() {

  if (digitalRead(PIR_PIN) == HIGH) {
    counter++;
  } else {
    counter = 0;
  }

  if (counter < 3) {
    delay(200);
    return;
  }

  counter = 0;

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Ada Orang");

  // ===== CAMERA =====
  bool berhasil = false;
  int retry = 0;

  while (retry < 5) {
    lcd.setCursor(0,1);
    lcd.print("Kirim Kamera...");
    if (triggerCamera()) {
      berhasil = true;
      break;
    }
    retry++;
    delay(2000);
  }

  if (!berhasil) {
    lcd.clear();
    lcd.print("Gagal Kamera");
    return;
  }

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Foto Terkirim");

  kirimNotif("📦 Paket terdeteksi!");

  delay(1000);

  // ===== BUKA PINTU =====
  lcd.setCursor(0,1);
  lcd.print("Pintu Atas");

  servoAtas1.write(140);
  servoAtas2.write(40);

  delay(1500);

  // ===== DETEKSI TANGAN =====
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Tunggu Keluar");

  unsigned long start = millis();

  while (true) {
    float jarak = bacaJarak();

    if (jarak > batasJarak) {
      if (millis() - start > 3000) break;
    } else {
      start = millis();
    }

    delay(200);
  }

  // ===== TUTUP PINTU =====
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Tutup Pintu");

  servoAtas1.write(0);
  servoAtas2.write(180);

  delay(1500);

  kirimNotif("✅ Paket tersimpan");

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Selesai");

  delay(3000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("System Ready");
  lcd.setCursor(0,1);
  lcd.print("Menunggu...");
}