#include <smog.h>

struct __attribute__((packed)) node {
	struct node *next;
	struct node *prev;
  	char padding[CACHE_LINE_SIZE - 2 * sizeof(struct node*)];
};

void pointer_chase_init(void *thread_buffer, size_t thread_pages);
void pointer_chase(Thread_Options t_opts);
