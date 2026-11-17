// Logika pembacaan fuel level: konversi ADC -> volt -> persen, deteksi fault,
// dan filter. Tidak memanggil analogRead/millis supaya bisa di-unit-test.
#pragma once

#include <math.h>
#include <stdint.h>

namespace fuel {

enum class Status : uint8_t { Ok, FaultShort, FaultOpen };

const char* statusName(Status s);

// Interpolasi linear pada tabel. xTable harus urut naik.
// Nilai di luar rentang tabel dibatasi ke ujung tabel.
float interpolate(float x, const float* xTable, const float* yTable, uint8_t n);

// Short ke ground jika voltage < vFaultLow, kabel putus jika voltage > vFaultHigh.
Status classify(float voltage, float vFaultLow, float vFaultHigh);

struct Settings {
  const float* voltTable;  // urut naik
  const float* fuelTable;  // persen, pasangan voltTable
  uint8_t tableSize;
  float vref;              // tegangan referensi ADC (Volt)
  float vFaultLow;
  float vFaultHigh;
  float tauS;              // konstanta waktu filter EMA (detik)
  float periodS;           // jarak antar pemanggilan FuelLevel::process (detik)
};

struct Reading {
  float adcAvg;
  float voltage;
  float fuelRaw;  // persen tanpa filter lambat; NAN saat fault
  float fuel;     // persen terfilter; NAN sampai ada pembacaan valid pertama
  Status status;
};

// Mengumpulkan n sampel ADC lalu menghasilkan rata-ratanya (float, tanpa pembulatan).
class Oversampler {
 public:
  explicit Oversampler(uint8_t n);
  // Menambah satu sampel. Mengembalikan true jika n sampel sudah terkumpul.
  bool add(uint16_t sample);
  // Rata-rata sampel yang terkumpul, lalu mulai blok baru. NAN jika kosong.
  float take();

 private:
  uint32_t sum_ = 0;
  uint8_t count_ = 0;
  uint8_t n_;
};

class FuelLevel {
 public:
  static constexpr float kAdcCounts = 1024.0f;  // ADC 10-bit AVR

  explicit FuelLevel(const Settings& settings);
  // Mengolah satu rata-rata ADC (dipanggil tiap periodS detik).
  Reading process(float adcAvg);

 private:
  Settings s_;
  float alpha_;
  float filtered_ = NAN;
};

}  // namespace fuel
