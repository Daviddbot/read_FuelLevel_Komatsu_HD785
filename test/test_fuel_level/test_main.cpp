#include <Arduino.h>
#include <FuelLevel.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <unity.h>

using fuel::FuelLevel;
using fuel::Oversampler;
using fuel::Reading;
using fuel::Settings;
using fuel::Status;

namespace {

const float kVolt[] = {0.100f, 0.575f, 1.050f, 1.390f, 1.785f};
const float kFuel[] = {100.0f, 75.0f, 50.0f, 25.0f, 0.0f};
const uint8_t kN = 5;

const Settings kSettings = {kVolt, kFuel, kN, 5.0f, 0.03f, 2.20f, 30.0f, 1.0f};

float adcFor(float volt) { return volt * FuelLevel::kAdcCounts / kSettings.vref; }

}  // namespace

void setUp() {}
void tearDown() {}

// ---------- interpolate ----------

void test_interpolate_hits_calibration_points() {
  for (uint8_t i = 0; i < kN; i++) {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, kFuel[i], fuel::interpolate(kVolt[i], kVolt, kFuel, kN));
  }
}

void test_interpolate_between_points() {
  // tengah 0.100..0.575 V -> tengah 100..75 %
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 87.5f, fuel::interpolate(0.3375f, kVolt, kFuel, kN));
  // tengah 1.390..1.785 V -> tengah 25..0 %
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.5f, fuel::interpolate(1.5875f, kVolt, kFuel, kN));
}

void test_interpolate_clamps_outside_table() {
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, fuel::interpolate(0.05f, kVolt, kFuel, kN));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, fuel::interpolate(2.0f, kVolt, kFuel, kN));
}

// ---------- classify ----------

void test_classify_status() {
  TEST_ASSERT_EQUAL(Status::FaultShort, fuel::classify(0.0f, 0.03f, 2.20f));
  TEST_ASSERT_EQUAL(Status::Ok, fuel::classify(0.03f, 0.03f, 2.20f));
  TEST_ASSERT_EQUAL(Status::Ok, fuel::classify(1.0f, 0.03f, 2.20f));
  TEST_ASSERT_EQUAL(Status::Ok, fuel::classify(2.20f, 0.03f, 2.20f));
  TEST_ASSERT_EQUAL(Status::FaultOpen, fuel::classify(2.5f, 0.03f, 2.20f));
}

// ---------- Oversampler ----------

void test_oversampler_float_average_and_reset() {
  Oversampler s(4);
  TEST_ASSERT_FALSE(s.add(215));
  TEST_ASSERT_FALSE(s.add(215));
  TEST_ASSERT_FALSE(s.add(216));
  TEST_ASSERT_TRUE(s.add(216));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 215.5f, s.take());  // tidak dibulatkan ke 215

  TEST_ASSERT_FALSE(s.add(100));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, s.take());  // blok baru, sisa lama tidak terbawa
}

void test_oversampler_empty_take_is_nan() {
  Oversampler s(4);
  TEST_ASSERT_TRUE(isnan(s.take()));
}

// ---------- FuelLevel ----------

void test_first_reading_seeds_filter() {
  // Regresi bug versi awal: filter mulai dari 0 -> 50% terbaca ~94% di detik pertama.
  FuelLevel f(kSettings);
  Reading r = f.process(adcFor(1.050f));
  TEST_ASSERT_EQUAL(Status::Ok, r.status);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 50.0f, r.fuelRaw);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 50.0f, r.fuel);
}

void test_short_to_ground_is_fault_not_full() {
  FuelLevel f(kSettings);
  Reading r = f.process(0.0f);
  TEST_ASSERT_EQUAL(Status::FaultShort, r.status);
  TEST_ASSERT_TRUE(isnan(r.fuelRaw));
  TEST_ASSERT_TRUE(isnan(r.fuel));  // belum pernah ada pembacaan valid
}

void test_open_wire_is_fault_and_keeps_last_value() {
  FuelLevel f(kSettings);
  f.process(adcFor(1.050f));
  Reading r = f.process(1023.0f);  // ADC mentok = kabel putus
  TEST_ASSERT_EQUAL(Status::FaultOpen, r.status);
  TEST_ASSERT_TRUE(isnan(r.fuelRaw));
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 50.0f, r.fuel);
}

void test_filter_follows_step_with_time_constant() {
  // Step 50% -> 25%: setelah TAU (30 x 1 detik) filter sudah ~63% jalan ke target.
  FuelLevel f(kSettings);
  f.process(adcFor(1.050f));
  Reading r;
  for (int i = 0; i < 30; i++) r = f.process(adcFor(1.390f));
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 25.0f, r.fuelRaw);
  TEST_ASSERT_FLOAT_WITHIN(1.0f, 50.0f - 0.63f * 25.0f, r.fuel);
}

void test_filter_dampens_slosh() {
  // Goyangan +-10% di sekitar 50% -> hasil filter tetap dekat 50%.
  FuelLevel f(kSettings);
  f.process(adcFor(1.050f));
  Reading r;
  for (int i = 0; i < 60; i++) {
    r = f.process(adcFor((i % 2) ? 1.186f : 0.860f));  // 1.186 V = 40%, 0.860 V = 60%
  }
  TEST_ASSERT_FLOAT_WITHIN(2.0f, 50.0f, r.fuel);
}

// Hentikan CPU setelah test selesai. simavr mendeteksi "sleep dengan interrupt mati"
// lalu keluar normal (output ter-flush ke PlatformIO); board fisik cukup diam.
void haltCpu() {
  Serial.flush();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  cli();
  sleep_cpu();
}

void setup() {
#ifndef SIMULATOR
  delay(2000);  // beri waktu Serial siap saat test di board fisik
#endif
  UNITY_BEGIN();
  RUN_TEST(test_interpolate_hits_calibration_points);
  RUN_TEST(test_interpolate_between_points);
  RUN_TEST(test_interpolate_clamps_outside_table);
  RUN_TEST(test_classify_status);
  RUN_TEST(test_oversampler_float_average_and_reset);
  RUN_TEST(test_oversampler_empty_take_is_nan);
  RUN_TEST(test_first_reading_seeds_filter);
  RUN_TEST(test_short_to_ground_is_fault_not_full);
  RUN_TEST(test_open_wire_is_fault_and_keeps_last_value);
  RUN_TEST(test_filter_follows_step_with_time_constant);
  RUN_TEST(test_filter_dampens_slosh);
  UNITY_END();
  haltCpu();
}

void loop() {}
