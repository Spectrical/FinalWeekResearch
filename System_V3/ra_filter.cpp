#include "ra_filter.h"
#include "stdlib.h"

void ra_filter_init(ra_filter_t *filter, int size) {
    filter->values = (int *)calloc(size, sizeof(int));
    filter->size = size;
    filter->index = 0;
    filter->count = 0;
    filter->sum = 0;
}

int ra_filter_run(ra_filter_t *filter, int value) {
    if (!filter->values) return value;

    filter->sum -= filter->values[filter->index];
    filter->values[filter->index] = value;
    filter->sum += value;

    filter->index = (filter->index + 1) % filter->size;
    if (filter->count < filter->size) filter->count++;

    return filter->sum / filter->count;
}