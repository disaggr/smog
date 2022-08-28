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
		Smog_Kernel(bool initialize, bool shuffle) : m_initialize(initialize), m_shuffle(shuffle) {}
		void Configure(Thread_Options t_opts) {
			m_id = t_opts.tid;
			m_page_count = t_opts.page_count;
			m_page_buffer = t_opts.page_buffer;
			m_buffer = (struct element*) m_page_buffer;
			m_elements = m_page_count * g_page_size / sizeof(struct element);
			
		}
		void Run() {
			if(m_initialize) {
				Initialize(m_shuffle);
			}
			pthread_barrier_wait(&g_initalization_finished);
			Execute_Kernel();
		}
		void Run_Unhinged() {
			if(m_initialize) {
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
		size_t m_page_count;
		void *m_page_buffer;
		struct element *m_buffer;
		uint64_t m_elements;
		bool m_initialize;
		bool m_shuffle;
	private:
		void Delete_Node( struct element *index);
		void Insert_Node( struct element *index, struct element *insertee);
};
