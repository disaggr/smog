/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#pragma once

#include "./smog.h"
#include "./kernel.h"

class Dirty_Pages : public Smog_Kernel {
 public:
  explicit Dirty_Pages(bool initialize) : Smog_Kernel(initialize, false) {}

 protected:
  void Execute_Kernel() override;
};
