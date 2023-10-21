/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#pragma once

#include <cstdint>

#include "./smog.h"
#include "./kernel.h"

class Random_Write : public Smog_Kernel {
 public:
  explicit Random_Write(bool initialize) : Smog_Kernel(initialize, false) {}

 protected:
  void Execute_Kernel() override;
};
