#include <smog.h>
#define CACHE_LINE_SIZE 128

struct __attribute__((packed)) node {
	struct node *next;
	struct node *prev;
  	char padding[CACHE_LINE_SIZE - 2 * sizeof(struct node*)];
};

void pointer_chase_init(Thread_Options t_opts);
void pointer_chase(Thread_Options t_opts);
