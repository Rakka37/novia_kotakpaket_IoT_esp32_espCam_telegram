#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>

// ================= WIFI =================
const char* ssid = "HUAWEI-2.4G-bUU5";
const char* password = "indonesia";

// ================= TELEGRAM =================
#define BOT_TOKEN "8484351035:AAHPK9O1YKlYjcjm39ZGG3iFW_Go9eYSqPc"
#define CHAT_ID "6027487871"

// ================= SERVER =================
WebServer server(80);

// 🔒 LOCK SYSTEM
bool sedangProses = false;

// ================= PIN CAMERA AI THINKER =================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ================= INIT CAMERA =================
void startCamera() {
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync= VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;

  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn  = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // 🔥 kecil biar stabil
  config.frame_size   = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count     = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("❌ Kamera gagal init");
    return;
  }

  Serial.println("✅ Kamera siap");
}

// ================= KIRIM FOTO =================
bool kirimFotoTelegram() {

  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("❌ Capture gagal");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  if (!client.connect("api.telegram.org", 443)) {
    Serial.println("❌ Gagal konek Telegram");
    esp_camera_fb_return(fb);
    return false;
  }

  Serial.println("📤 Mengirim foto...");

  String boundary = "----123456789";

  String data = "";
  data += "--" + boundary + "\r\n";
  data += "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n";
  data += CHAT_ID;
  data += "\r\n";

  data += "--" + boundary + "\r\n";
  data += "Content-Disposition: form-data; name=\"photo\"; filename=\"image.jpg\"\r\n";
  data += "Content-Type: image/jpeg\r\n\r\n";

  String endData = "\r\n--" + boundary + "--\r\n";

  int contentLength = data.length() + fb->len + endData.length();

  client.println("POST /bot" + String(BOT_TOKEN) + "/sendPhoto HTTP/1.1");
  client.println("Host: api.telegram.org");
  client.println("Content-Type: multipart/form-data; boundary=" + boundary);
  client.println("Content-Length: " + String(contentLength));
  client.println();

  client.print(data);
  client.write(fb->buf, fb->len);
  client.print(endData);

  esp_camera_fb_return(fb);

  // ===== VALIDASI RESPONSE =====
  bool sukses = false;

  Serial.println("📡 Response:");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    Serial.println(line);

    if (line.indexOf("200 OK") >= 0) {
      sukses = true;
    }

    if (line == "\r") break;
  }

  return sukses;
}

// ================= ENDPOINT =================
void handleCapture() {

  if (sedangProses) {
    Serial.println("⛔ BUSY");
    server.send(429, "text/plain", "BUSY");
    return;
  }

  sedangProses = true;

  Serial.println("📷 Trigger diterima");

  bool sukses = false;

  for (int i = 0; i < 3; i++) {
    if (kirimFotoTelegram()) {
      sukses = true;
      break;
    }
    delay(2000);
  }

  sedangProses = false;

  if (sukses) {
    Serial.println("✅ Kirim sukses");
    server.send(200, "text/plain", "OK");
  } else {
    Serial.println("❌ Kirim gagal");
    server.send(500, "text/plain", "FAIL");
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi Connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  startCamera();

  server.on("/capture", handleCapture);
  server.begin();
}

// ================= LOOP =================
void loop() {
  server.handleClient();
}