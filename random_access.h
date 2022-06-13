#pragma once
#include "smog.h"

void random_access_init(void *thread_buffer, size_t thread_pages);
void random_access(Thread_Options t_opts);
