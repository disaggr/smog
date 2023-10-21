/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#pragma once

#include <cstdint>

#include "./smog.h"
#include "./kernel.h"

class Pointer_Chase : public Smog_Kernel {
 public:
  explicit Pointer_Chase(bool initialize) : Smog_Kernel(initialize, true) {}

 protected:
  void Execute_Kernel() override;
  void Execute_Kernel_Unhinged() override;

 private:
  void Delete_Node(struct node *index);
  void Insert_Node(struct node *index, struct node *insertee);
};
