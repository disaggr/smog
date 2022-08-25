#include <pointer_chase.h>
#include <stdint.h>
#include <cstdlib>

void Pointer_Chase::Execute_Kernel() {
	struct element *tmp = &m_buffer[0];

	while (1) {
			// Here I am assuming the impact of skipping a few pages is not
			// going to be a big issue
			if (g_measuring) {
				mem_fence();
				continue;
			}

			//tmp = tmp->next;
			//tmp = tmp->next;
			//tmp = tmp->next;
			tmp = tmp->next;
			g_thread_status[m_id].count += 1;

			volatile uint64_t delay = 0;
			for(size_t j = 0; j < g_smog_delay; j++) {
				delay += 1;
			}
	}
}
