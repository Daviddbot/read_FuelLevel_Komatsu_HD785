// Semua parameter yang perlu disetel di lapangan ada di file ini.
#pragma once

#include <Arduino.h>

namespace cfg {

// ---------------- Pin & ADC ----------------
// Mega 2560: I2C ada di pin 20/21, jadi A0..A15 bebas dipakai.
constexpr uint8_t PIN_FUEL = A0;

// Referensi ADC.
//   DEFAULT      -> 5V board (sama seperti versi Uno; tabel kalibrasi dibuat dengan ini)
//   INTERNAL2V56 -> referensi internal 2.56 V Mega: resolusi 2x lebih halus dan tidak
//                   terpengaruh naik-turun 5V. Jika dipakai, ukur tegangan pin AREF
//                   dengan multimeter, isi ke VREF, lalu verifikasi ulang kalibrasi.
constexpr uint8_t ADC_REFERENCE = DEFAULT;

// Tegangan referensi ADC (Volt).
// Biarkan 5.00 jika tabel kalibrasi diambil dari angka Serial Monitor.
// Isi dengan hasil multimeter jika tabel diukur dengan multimeter.
constexpr float VREF = 5.00f;

// ---------------- Kalibrasi ----------------
// Tegangan naik = solar turun. VOLT_TABLE HARUS urut naik (dicek saat compile).
constexpr float VOLT_TABLE[] = {0.100f, 0.575f, 1.050f, 1.390f, 1.785f};  // Volt
constexpr float FUEL_TABLE[] = {100.0f, 75.0f,  50.0f,  25.0f,  0.0f};    // Persen
constexpr uint8_t TABLE_SIZE = sizeof(VOLT_TABLE) / sizeof(VOLT_TABLE[0]);

// ---------------- Deteksi fault ----------------
// VERIFIKASI di unit: ukur tegangan saat konektor sensor dicabut (open)
// dan saat sinyal di-short ke ground.
constexpr float V_FAULT_LOW = 0.03f;   // di bawah ini: short ke ground
constexpr float V_FAULT_HIGH = 2.20f;  // di atas ini: kabel putus / open

// ---------------- Sampling & filter ----------------
constexpr unsigned long SAMPLE_MS = 50;      // 20 sampel/detik
constexpr uint8_t SAMPLES_PER_REPORT = 20;   // 20 x 50 ms = laporan tiap 1 detik
constexpr float TAU_S = 30.0f;               // konstanta waktu filter EMA (detik)

// ---------------- Output ----------------
constexpr unsigned long SERIAL_BAUD = 115200;

// ---------------- Pengecekan saat compile ----------------
constexpr bool isAscending(const float* t, uint8_t n) {
  return n < 2 || (t[0] < t[1] && isAscending(t + 1, n - 1));
}
static_assert(sizeof(FUEL_TABLE) == sizeof(VOLT_TABLE),
              "VOLT_TABLE dan FUEL_TABLE harus sama panjang");
static_assert(TABLE_SIZE >= 2, "Tabel kalibrasi minimal 2 titik");
static_assert(isAscending(VOLT_TABLE, TABLE_SIZE), "VOLT_TABLE harus urut naik");
static_assert(V_FAULT_LOW < VOLT_TABLE[0] && V_FAULT_HIGH > VOLT_TABLE[TABLE_SIZE - 1],
              "Batas fault harus di luar rentang tabel kalibrasi");
static_assert(SAMPLES_PER_REPORT > 0, "SAMPLES_PER_REPORT minimal 1");

}  // namespace cfg
