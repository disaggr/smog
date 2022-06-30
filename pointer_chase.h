#include "smog.h"
#include "kernel.h"
#include <cstdint>

class Pointer_Chase : public Smog_Kernel {
	public:
		Pointer_Chase(bool initialize) : Smog_Kernel(initialize) {}
	protected:
		void Execute_Kernel() override;
    private:
		void Delete_Node( struct node *index);
		void Insert_Node( struct node *index, struct node *insertee);
};