#include "progress.h"
#include "constants.h" 
#include <stdio.h>
#include <stdint.h>

void format_time(double total_seconds, int *hours, int *minutes, int *seconds, int *milliseconds) {
    *hours = (int)(total_seconds / 3600);
    *minutes = (int)((total_seconds - (*hours * 3600)) / 60);
    *seconds = (int)total_seconds % 60;
    *milliseconds = (int)((total_seconds - (int)total_seconds) * 1000);
}

void display_progress(size_t completed, size_t total, double elapsed_time, double start_time) {
    int bar_width = 50;
    double progress = (double)completed / total;
    int filled = (int)(progress * bar_width);
    int remaining = bar_width - filled;

    double current_rate = completed / (elapsed_time - start_time);
    double eta = (current_rate > 0) ? (double)(total - completed) / current_rate : 0;

    int eta_hours, eta_minutes, eta_seconds, eta_milliseconds;
    format_time(eta, &eta_hours, &eta_minutes, &eta_seconds, &eta_milliseconds);

    printf("\r[");
    for (int i = 0; i < filled; i++) printf("=");
    for (int i = 0; i < remaining; i++) printf(" ");
    printf("] %zu/%zu ETA: %02d:%02d:%02d.%03d", completed, total, eta_hours, eta_minutes, eta_seconds, eta_milliseconds);
    fflush(stdout);
}

