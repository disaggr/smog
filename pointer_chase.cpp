#include <pointer_chase.h>
#include <stdint.h>
#include <cstdlib>

void Pointer_Chase::Delete_Node( struct node *index) {
	index->prev->next = index->next;
	index->next->prev = index->prev;
}

void Pointer_Chase::Insert_Node( struct node *index, struct node *insertee) {
	index->next->prev = insertee;
	insertee->next = index->next;
	insertee->prev = index;
	index->next = insertee;
}

void Pointer_Chase::Initialize(void *thread_buffer, size_t thread_pages)
{
	m_list = (struct node*) m_page_buffer;
	m_elements = m_page_count * g_page_size / CACHE_LINE_SIZE;
	for(uint64_t i = 1; i < m_elements - 1; i++){
		m_list[i].prev = &m_list[i - 1];
		m_list[i].next = &m_list[i + 1];
	}

	m_list[0].prev = &m_list[m_elements - 1];
	m_list[0].next = &m_list[1];
	m_list[m_elements - 1].prev= &m_list[m_elements - 2];
	m_list[m_elements - 1].next = &m_list[0];

	std::srand(std::time(0));

	uint64_t r = 0;
	for(uint64_t i = 0; i < m_elements - 1; i++) {
		r = std::rand() % (m_elements - 1);
		delete_node(&m_list[r]);
		insert_node(&m_list[i], &m_list[r]);
	}
}

void Pointer_Chase::Execute_Kernel() {
	struct node *tmp = &list[0];

	while (1) {
			// Here I am assuming the impact of skipping a few pages is not
			// going to be a big issue
			if (measuring) {
				continue;
			}

			tmp = tmp->next;
			tmp = tmp->next;
			tmp = tmp->next;
			tmp = tmp->next;
			g_thread_status[m_tid].count += 4;

			volatile uint64_t delay = 0;
			for(size_t j = 0; j < g_smog_delay; j++) {
				delay += 1;
			}
	}
}
