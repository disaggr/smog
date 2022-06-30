#pragma once
#include "smog.h"
#include <stdint.h>

#define PARALLEL_ACCESSES 4

struct __attribute__((packed)) element {
	uint64_t index;
	struct element *next;
	struct element *prev;
	uint64_t randoms[PARALLEL_ACCESSES];
	uint64_t scratch;
  	char padding[CACHE_LINE_SIZE - sizeof(uint64_t) - 2 * sizeof(struct node*) - PARALLEL_ACCESSES * sizeof(uint64_t) - sizeof(uint64_t)];
};

class Smog_Kernel {
	public:
		Smog_Kernel(bool initialize) : m_initialize(initialize) {}
		void Configure(Thread_Options t_opts) {
			m_id = t_opts.tid;
			m_page_count = t_opts.page_count;
			m_page_buffer = t_opts.page_buffer;
			
		}
		void Run() {
			if(m_initialize) {
				Initialize();
			}
			pthread_barrier_wait(&g_initalization_finished);
			Execute_Kernel();
		}

		void Initialize();
	protected:
		virtual void Execute_Kernel() = 0;
		int m_id;
		size_t m_page_count;
		void *m_page_buffer;
		struct element *m_buffer;
		uint64_t m_elements;
		bool m_initialize;
	private:
		void Delete_Node( struct element *index);
		void Insert_Node( struct element *index, struct element *insertee);
};
