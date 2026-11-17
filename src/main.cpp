/*
  Read Fuel Level - Komatsu HD785
  Board : Arduino Mega 2560 (ATmega2560, ADC 10-bit)

  Output Serial (1x per detik), format key=value:
    adc=215.3,v=1.051,fuel_raw=49.9,fuel=50.0,status=OK
  fuel_raw = hasil per detik (bandingkan dengan display saat unit diam)
  fuel     = hasil terfilter (redam goyangan solar saat unit jalan)

  Parameter lapangan (pin, kalibrasi, batas fault, filter): include/Config.h
*/
#include <Arduino.h>
#include <FuelLevel.h>

#include "Config.h"

namespace {

const fuel::Settings kSettings = {
    cfg::VOLT_TABLE,
    cfg::FUEL_TABLE,
    cfg::TABLE_SIZE,
    cfg::VREF,
    cfg::V_FAULT_LOW,
    cfg::V_FAULT_HIGH,
    cfg::TAU_S,
    cfg::SAMPLE_MS * cfg::SAMPLES_PER_REPORT / 1000.0f,
};

fuel::Oversampler sampler(cfg::SAMPLES_PER_REPORT);
fuel::FuelLevel fuelLevel(kSettings);
unsigned long prevSample = 0;

void printReading(const fuel::Reading& r) {
  Serial.print(F("adc="));
  Serial.print(r.adcAvg, 1);
  Serial.print(F(",v="));
  Serial.print(r.voltage, 3);
  Serial.print(F(",fuel_raw="));
  Serial.print(r.fuelRaw, 1);  // "nan" saat fault
  Serial.print(F(",fuel="));
  Serial.print(r.fuel, 1);     // nilai valid terakhir
  Serial.print(F(",status="));
  Serial.println(fuel::statusName(r.status));
}

}  // namespace

void setup() {
  Serial.begin(cfg::SERIAL_BAUD);
  analogReference(cfg::ADC_REFERENCE);
  analogRead(cfg::PIN_FUEL);  // buang konversi pertama setelah power-on / ganti referensi
}

void loop() {
  unsigned long now = millis();
  if (now - prevSample < cfg::SAMPLE_MS) return;
  prevSample = now;

  if (!sampler.add(analogRead(cfg::PIN_FUEL))) return;
  printReading(fuelLevel.process(sampler.take()));
}
