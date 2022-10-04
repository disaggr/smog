#include <pointer_chase.h>
#include <stdint.h>
#include <cstdlib>
#include <iostream>

void Pointer_Chase::Execute_Kernel() {
	// mark this volatile so that gcc is strict about load and stores
	// (as if they had side effects) so usage is not optimized away below.
	volatile struct element *tmp = &m_buffer[0];

	while (1) {
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

#define REP0(X)
#define REP1(X) X
#define REP2(X) REP1(X) X
#define REP3(X) REP2(X) X
#define REP4(X) REP3(X) X
#define REP5(X) REP4(X) X
#define REP6(X) REP5(X) X
#define REP7(X) REP6(X) X
#define REP8(X) REP7(X) X
#define REP9(X) REP8(X) X
#define REP10(X) REP9(X) X

#define REP(HUNDREDS,TENS,ONES,X) \
  REP##HUNDREDS(REP10(REP10(X))) \
  REP##TENS(REP10(X)) \
  REP##ONES(X)

void Pointer_Chase::Execute_Kernel_Unhinged() {
	volatile struct element *tmp = &m_buffer[0];

	while (1) {
		REP(9,9,9,tmp = tmp->next;)
		g_thread_status[m_id].count += 999;
	}
}
