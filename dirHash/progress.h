#ifndef PROGRESS_H
#define PROGRESS_H

#include <stddef.h>

void format_time(double total_seconds, int *hours, int *minutes, int *seconds, int *milliseconds);
void display_progress(size_t completed, size_t total, double elapsed_time, double start_time);

#endif

