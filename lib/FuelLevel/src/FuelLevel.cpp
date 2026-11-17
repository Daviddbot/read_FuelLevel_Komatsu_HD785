#include "FuelLevel.h"

namespace fuel {

const char* statusName(Status s) {
  switch (s) {
    case Status::Ok:
      return "OK";
    case Status::FaultShort:
      return "FAULT_SHORT";
    case Status::FaultOpen:
      return "FAULT_OPEN";
  }
  return "UNKNOWN";
}

float interpolate(float x, const float* xTable, const float* yTable, uint8_t n) {
  if (x <= xTable[0]) return yTable[0];
  if (x >= xTable[n - 1]) return yTable[n - 1];
  for (uint8_t i = 0; i < n - 1; i++) {
    if (x <= xTable[i + 1]) {
      return yTable[i] + (x - xTable[i]) * (yTable[i + 1] - yTable[i]) /
                             (xTable[i + 1] - xTable[i]);
    }
  }
  return yTable[n - 1];
}

Status classify(float voltage, float vFaultLow, float vFaultHigh) {
  if (voltage < vFaultLow) return Status::FaultShort;
  if (voltage > vFaultHigh) return Status::FaultOpen;
  return Status::Ok;
}

Oversampler::Oversampler(uint8_t n) : n_(n) {}

bool Oversampler::add(uint16_t sample) {
  sum_ += sample;
  count_++;
  return count_ >= n_;
}

float Oversampler::take() {
  float avg = count_ ? static_cast<float>(sum_) / count_ : NAN;
  sum_ = 0;
  count_ = 0;
  return avg;
}

FuelLevel::FuelLevel(const Settings& settings)
    : s_(settings), alpha_(settings.periodS / (settings.tauS + settings.periodS)) {}

Reading FuelLevel::process(float adcAvg) {
  Reading r;
  r.adcAvg = adcAvg;
  r.voltage = adcAvg * s_.vref / kAdcCounts;
  r.status = classify(r.voltage, s_.vFaultLow, s_.vFaultHigh);
  r.fuelRaw = NAN;

  if (r.status == Status::Ok) {
    r.fuelRaw = interpolate(r.voltage, s_.voltTable, s_.fuelTable, s_.tableSize);
    // Pembacaan valid pertama langsung mengisi filter (tidak mulai dari 0).
    if (isnan(filtered_)) {
      filtered_ = r.fuelRaw;
    } else {
      filtered_ += alpha_ * (r.fuelRaw - filtered_);
    }
  }
  // Saat fault, filter tidak diubah: r.fuel = nilai valid terakhir.
  r.fuel = filtered_;
  return r;
}

}  // namespace fuel
