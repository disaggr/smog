/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#pragma once

#include <cstdint>

#include "./smog.h"
#include "./kernel.h"

class Linear_Scan : public Smog_Kernel {
 public:
  explicit Linear_Scan(bool initialize) : Smog_Kernel(initialize, false) {}

 protected:
  void Execute_Kernel() override;
  void Execute_Kernel_Unhinged() override;
};
