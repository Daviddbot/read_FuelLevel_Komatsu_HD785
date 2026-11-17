/*
  Read Fuel Level - Komatsu HD785  (v2)
  Board : Arduino Uno (ATmega328P, ADC 10-bit)

  Output Serial (1x per detik), format key=value agar mudah dibaca & di-parse:
    adc=215.3,v=1.051,fuel_raw=49.9,fuel=50.0,status=OK
  fuel_raw = hasil per detik (tanpa filter lambat) -> untuk dibandingkan dengan display saat unit diam
  fuel     = hasil terfilter (redam goyangan solar saat unit jalan)
*/

// ---------------- Konfigurasi ----------------
const uint8_t PIN_FUEL = A0;   // A4/A5 = jalur I2C (SDA/SCL) di Uno; pakai A0 supaya bebas tambah LCD/RTC I2C

// Tegangan pin 5V Arduino (referensi ADC). Ukur dengan multimeter saat Arduino
// dinyalakan dari sumber daya yang SAMA dengan yang dipakai di unit.
// Catatan: kalau tabel kalibrasi di bawah diambil dari angka Serial Monitor (bukan multimeter),
// biarkan 5.00 agar kalibrasi tidak bergeser.
const float VREF = 5.00;

// Tabel kalibrasi (voltTable HARUS urut naik)
const uint8_t N = 5;
const float voltTable[N] = {0.100, 0.575, 1.050, 1.390, 1.785};  // Volt
const float fuelTable[N] = {100.0, 75.0,  50.0,  25.0,  0.0};    // Persen

// Batas deteksi fault -> VERIFIKASI di unit:
//   ukur tegangan saat konektor sensor dicabut (open) dan saat sinyal di-short ke ground.
const float V_FAULT_LOW  = 0.03;  // di bawah ini: short ke ground (jangan ditampilkan 100%)
const float V_FAULT_HIGH = 2.20;  // di atas ini: kabel putus / open (jangan ditampilkan 0%)

// Sampling & filter
const unsigned long SAMPLE_MS = 50;           // 20 sampel/detik -> redam noise listrik
const uint8_t SAMPLES_PER_REPORT = 20;        // 20 x 50 ms = laporan tiap 1 detik
const float TAU_S = 30.0;                     // konstanta waktu filter (detik) -> redam sloshing
// ---------------------------------------------

unsigned long prevSample = 0;
uint32_t adcSum = 0;
uint8_t adcCount = 0;
float fuelFiltered = NAN;   // NAN = belum ada pembacaan valid

float interpolate(float x, const float xT[], const float yT[], uint8_t n) {
  if (x <= xT[0]) return yT[0];
  if (x >= xT[n - 1]) return yT[n - 1];
  for (uint8_t i = 0; i < n - 1; i++) {
    if (x <= xT[i + 1]) {
      return yT[i] + (x - xT[i]) * (yT[i + 1] - yT[i]) / (xT[i + 1] - xT[i]);
    }
  }
  return yT[n - 1];
}

void setup() {
  Serial.begin(115200);
  analogRead(PIN_FUEL);  // buang konversi pertama setelah power-on
}

void loop() {
  unsigned long now = millis();
  if (now - prevSample < SAMPLE_MS) return;
  prevSample = now;

  // --- Oversampling: kumpulkan 20 sampel, rata-rata dalam float (tanpa pembulatan) ---
  adcSum += analogRead(PIN_FUEL);
  if (++adcCount < SAMPLES_PER_REPORT) return;

  float adcAvg = (float)adcSum / adcCount;
  adcSum = 0;
  adcCount = 0;

  float voltage = adcAvg * VREF / 1024.0;   // ATmega328P: V = ADC * Vref / 1024

  const char* status = "OK";
  float fuelRaw = NAN;

  if (voltage < V_FAULT_LOW) {
    status = "FAULT_SHORT";
  } else if (voltage > V_FAULT_HIGH) {
    status = "FAULT_OPEN";
  } else {
    fuelRaw = interpolate(voltage, voltTable, fuelTable, N);

    // --- Filter EMA: langsung diisi nilai pertama (tidak start dari 0) ---
    if (isnan(fuelFiltered)) {
      fuelFiltered = fuelRaw;
    } else {
      const float dt = SAMPLE_MS * SAMPLES_PER_REPORT / 1000.0;
      const float alpha = dt / (TAU_S + dt);
      fuelFiltered += alpha * (fuelRaw - fuelFiltered);
    }
  }

  Serial.print(F("adc="));        Serial.print(adcAvg, 1);
  Serial.print(F(",v="));         Serial.print(voltage, 3);
  Serial.print(F(",fuel_raw="));  Serial.print(fuelRaw, 1);       // "nan" jika fault
  Serial.print(F(",fuel="));      Serial.print(fuelFiltered, 1);  // nilai valid terakhir
  Serial.print(F(",status="));    Serial.println(status);
}
