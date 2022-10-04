#include "kernel.h"
#include <cstdlib>
#include <iostream>

void Smog_Kernel::Delete_Node( struct element *index) {
	index->prev->next = index->next;
	index->next->prev = index->prev;
}

void Smog_Kernel::Insert_Node( struct element *index, struct element *insertee) {
	index->next->prev = insertee;
	insertee->next = index->next;
	insertee->prev = index;
	index->next = insertee;
}

void Smog_Kernel::Initialize(bool shuffle) {
	std::srand(std::time(0));

	for(uint64_t i = 0; i < m_elements; i++) {
		m_buffer[i].index = i;

		//initialize for dirty pages
		m_buffer[i].scratch = 0;

		//initialize for random access kernel
		for(uint64_t j = 0; j < PARALLEL_ACCESSES; j++)
			m_buffer[i].randoms[j] = std::rand() % (m_elements - 1);
				
	}

	for(uint64_t i = 1; i < m_elements - 1; i++) {
		//initialize for pointer chase kernel part 1
		m_buffer[i].prev = &m_buffer[i - 1];
		m_buffer[i].next = &m_buffer[i + 1];
	}

	//initialize for pointer chase kernel part 2
	m_buffer[0].prev = &m_buffer[m_elements - 1];
	m_buffer[0].next = &m_buffer[1];
	m_buffer[m_elements - 1].prev = &m_buffer[m_elements - 2];
	m_buffer[m_elements - 1].next = &m_buffer[0];

	if (shuffle) {
		for (uint64_t i = 0; i < m_elements - 2; ++i) {
			uint64_t j = i + std::rand() % (m_elements - i);
			struct element *a = &m_buffer[i];
			struct element *b = &m_buffer[j];
			a->prev->next = b;
			a->next->prev = b;
			b->prev->next = a;
			b->next->prev = a;
			struct element *t = a->next;
			a->next = b->next;
			b->next = t;
			t = a->prev;
			a->prev = b->prev;
			b->prev = t;
		}

	}

	//if(shuffle) {
	//	uint64_t r = 0;
	//	for(uint64_t i = 0; i < m_elements - 1; i++) {
	//		r = std::rand() % (m_elements - 1);
	//		Delete_Node(&m_buffer[r]);
	//		Insert_Node(&m_buffer[i], &m_buffer[r]);
	//	}
	//}

	//struct element *s = &m_buffer[0];
	//struct element *p = s;
	//do {
	//	std::cout << p->index << std::endl;
	//	p = p->next;
	//} while (p != s);
}
