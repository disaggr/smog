#pragma once
#include "smog.h"
#include "kernel.h"

class Cold : public Smog_Kernel {
	public:
		Cold() {}
	protected:
		void Initialize() override;
		void Execute_Kernel() override;
};
