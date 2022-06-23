#include "smog.h"
#include "kernel.h"
#include <cstdint>

struct __attribute__((packed)) node {
	struct node *next;
	struct node *prev;
  	char padding[CACHE_LINE_SIZE - 2 * sizeof(struct node*)];
};

class Pointer_Chase : public Smog_Kernel {
	public:
		Pointer_Chase() {}
	protected:
		void Initialize() override;
		void Execute_Kernel() override;
    private:
		void Delete_Node( struct node *index);
		void Insert_Node( struct node *index, struct node *insertee);
        struct node *m_list;
        uint64_t m_elements;
};