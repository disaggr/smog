/*
 * Copyright (c) 2022 - 2023 OSM Group @ HPI, University of Potsdam
 */

#pragma once

#include <stdint.h>

#include "./smog.h"

#define PARALLEL_ACCESSES 4

struct element {
    union {
        struct {
            uint64_t index;
            struct element *next;
            struct element *prev;
            uint64_t randoms[PARALLEL_ACCESSES];
            uint64_t scratch;
        };
        char padding[CACHE_LINE_SIZE];
    };
};

class Smog_Kernel {
 public:
  Smog_Kernel(bool initialize, bool shuffle) : m_initialize(initialize), m_shuffle(shuffle) {}
  void Configure(thread_options t_opts) {
      m_id = t_opts.tid;
      m_slice_start = t_opts.slice_start;
      m_slice_length = t_opts.slice_length;
      m_buffer = (struct element*) m_slice_start;
      m_elements = m_slice_length / sizeof(struct element);
  }
  void Run() {
      if (m_initialize) {
          Initialize(m_shuffle);
      }
      pthread_barrier_wait(&g_initalization_finished);
      Execute_Kernel();
  }
  void Run_Unhinged() {
      if (m_initialize) {
          Initialize(m_shuffle);
      }
      pthread_barrier_wait(&g_initalization_finished);
      Execute_Kernel_Unhinged();
  }

  void Initialize(bool shuffle);

 protected:
  virtual void Execute_Kernel() = 0;
  virtual void Execute_Kernel_Unhinged() {
      Execute_Kernel();
  }
  int m_id;
  void *m_slice_start;
  size_t m_slice_length;
  struct element *m_buffer;
  uint64_t m_elements;
  bool m_initialize;
  bool m_shuffle;

 private:
  void Delete_Node(struct element *index);
  void Insert_Node(struct element *index, struct element *insertee);
};
