#include <pointer_chase.h>
#include <stdint.h>
#include <cstdlib>

void delete_node( struct node *index) {
	index->prev->next = index->next;
	index->next->prev = index->prev;
}

void insert_node( struct node *index, struct node *insertee) {
	index->next->prev = insertee;
	insertee->next = index->next;
	insertee->prev = index;
	index->next = insertee;
}

void pointer_chase_init(void *thread_buffer, size_t thread_pages)
{
	struct node* list = (struct node*) thread_buffer;
	uint64_t elements = thread_pages * page_size / CACHE_LINE_SIZE;
	for(uint64_t i = 1; i < elements - 1; i++){
		list[i].prev = &list[i - 1];
		list[i].next = &list[i + 1];
	}

	list[0].prev = &list[elements - 1];
	list[0].next = &list[1];
	list[elements - 1].prev= &list[elements - 2];
	list[elements - 1].next = &list[0];

	std::srand(std::time(0));

	uint64_t r = 0;
	for(uint64_t i = 0; i < elements - 1; i++) {
		r = std::rand() % (elements - 1);
		delete_node(&list[r]);
		insert_node(&list[i], &list[r]);
	}
}

void pointer_chase(Thread_Options t_opts) {
	struct node* list = (struct node*) t_opts.page_buffer;
	//uint64_t elements = t_opts.page_count * page_size / CACHE_LINE_SIZE;
	struct node *tmp = &list[0];

	while (1) {
                        // Here I am assuming the impact of skipping a few pages is not
                        // going to be a big issue
                        if (measuring) {
                                continue;
                        }

			tmp = tmp->next;
                        thread_status[t_opts.tid].count += 1;

                        volatile uint64_t delay = 0;
                        for(size_t j = 0; j < smog_delay; j++) {
                                 delay += 1;
                        }
        }
}
