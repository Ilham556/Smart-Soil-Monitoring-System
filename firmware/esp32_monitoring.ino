#include <WiFiManager.h> // Library untuk mengatur WiFi via HP
#include <ThingSpeak.h>  // Library untuk kirim data ke ThingSpeak

// ===== KONFIGURASI SENSOR =====
#define PH_PIN    34
#define PH_EN     13
#define SOIL_PIN  35

// ===== KONFIGURASI THINGSPEAK =====
unsigned long myChannelNumber = 3216180;      // GANTI dengan Channel ID Anda
const char * myWriteAPIKey = "NHH4EEV0NC5UCWK7"; // GANTI dengan Write API Key Anda

WiFiClient  client;

void setup() {
  Serial.begin(115200);
  
  // Setup Sensor (Sesuai kode asli Anda)
  analogReadResolution(10);        // Resolusi 10-bit (0-1023)
  analogSetAttenuation(ADC_11db);  // Baca tegangan hingga ~3.3V

  pinMode(PH_EN, OUTPUT);
  digitalWrite(PH_EN, HIGH);

  Serial.println("\n=== MULAI SISTEM MONITORING ===");

  // ===== SETUP WIFI MANAGER =====
  // 1. Buat object WiFiManager
  WiFiManager wm;

  // 2. (Opsional) Reset pengaturan wifi tersimpan (hapus komen // di bawah untuk testing)
  // wm.resetSettings();

  // 3. Buat Portal Wifi.
  // Jika ESP32 belum connect, dia akan memancarkan WiFi bernama "Alat_Tani_IoT"
  bool res;
  res = wm.autoConnect("Alat_Tani_IoT", "12345678"); // Nama WiFi AP & Password

  if(!res) {
      Serial.println("Gagal terhubung ke WiFi");
      // ESP.restart(); // Restart jika gagal
  } else {
      Serial.println("Berhasil terhubung ke WiFi!");
  }

  // ===== SETUP THINGSPEAK =====
  ThingSpeak.begin(client);
}

void loop() {
  // Cek Koneksi WiFi, jika putus coba reconnect
  if (WiFi.status() != WL_CONNECTED) {
    WiFiManager wm;
    wm.autoConnect("Alat_Tani_IoT", "12345678");
  }

  /* ===== BACA SENSOR SOIL MOISTURE ===== */
  long soilSum = 0;
  for (int i = 0; i < 10; i++) {
    soilSum += analogRead(SOIL_PIN);
    delay(50);
  }
  float soilRaw = soilSum / 10.0;

  // Mapping nilai 10-bit (0-1023) ke persen
  float soilPercent = map(soilRaw, 1023, 0, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);

  /* ===== BACA SENSOR pH TANAH ===== */
  long sum = 0;
  for (int i = 0; i < 20; i++) {
    sum += analogRead(PH_PIN);
    delay(100);
  }
  float adcAvg = sum / 20.0;

  // Logika Saturasi
  if (adcAvg >= 1015) {
    Serial.println("ADC SATURASI - pH TIDAK VALID");
    delay(2000);
    return;
  }

  adcAvg = constrain(adcAvg, 20, 300);
  float ph = (-0.016 * adcAvg) + 10.6;
  ph = constrain(ph, 3.5, 9.5);

  /* ===== STATUS TANAH ===== */
  String soilStatus;
  if (soilPercent < 10) soilStatus = "SANGAT KERING";
  else if (soilPercent < 25) soilStatus = "KERING";
  else soilStatus = "LEMBAP";

  /* ===== PRINT KE SERIAL MONITOR ===== */
  Serial.print("Soil: "); Serial.print(soilPercent, 1); Serial.print("% (" + soilStatus + ")");
  Serial.print(" | pH: "); Serial.println(ph, 1);

  /* ===== KIRIM KE THINGSPEAK ===== */
  // Set Field 1 untuk Soil Moisture
  ThingSpeak.setField(1, soilPercent);
  
  // Set Field 2 untuk pH Tanah
  ThingSpeak.setField(2, ph);

  // Kirim data
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  if(x == 200){
    Serial.println(">> Data berhasil dikirim ke ThingSpeak");
  } else {
    Serial.println(">> Gagal update. Kode Error: " + String(x));
  }

  // ThingSpeak versi gratis butuh jeda minimal 15 detik antar pengiriman
  Serial.println("Menunggu 20 detik sebelum update berikutnya...");
  delay(20000); 
}
