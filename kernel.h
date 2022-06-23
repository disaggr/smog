#pragma once
#include "smog.h"

class Smog_Kernel {
	public:
		Smog_Kernel() {}
		void Configure(Thread_Options t_opts) {
			m_id = t_opts.tid;
			m_page_count = t_opts.page_count;
			m_page_buffer = t_opts.page_buffer;
			
		}
		void Run() {
			Initialize();
			pthread_barrier_wait(&g_initalization_finished);
			Execute_Kernel();
		}
	protected:
		virtual void Initialize() = 0;
		virtual void Execute_Kernel() = 0;
		int m_id;
		size_t m_page_count;
		void *m_page_buffer;
};
