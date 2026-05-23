#pragma once

#include <Arduino.h>

#include "models/SensorData.h"

class ApiClient {
 public:
  explicit ApiClient(const String& endpoint);

  String buildPayload(const SensorSnapshot& snapshot) const;
  bool postPayload(const String& payload) const;
  const String& endpoint() const;

 private:
  String endpoint_;
};
