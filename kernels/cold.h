/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#pragma once

#include "./smog.h"
#include "./kernel.h"

class Cold : public Smog_Kernel {
 public:
  explicit Cold(bool initialize) : Smog_Kernel(initialize, false) {}

 protected:
  void Execute_Kernel() override;
};
