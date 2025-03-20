#ifndef RA_FILTER_H
#define RA_FILTER_H

typedef struct {
    int *values;
    int size;
    int index;
    int count;
    int sum;
} ra_filter_t;

void ra_filter_init(ra_filter_t *filter, int size);
int ra_filter_run(ra_filter_t *filter, int value);

#endif // RA_FILTER_H