#include<pthread.h>

typedef struct parameters{
    int cache_hits;
    int cache_misses;
    int total_requests;
    double rate;
    pthread_mutex_t lock;
}parameters;

void update_hit(parameters* p);
void initialize(parameters* p);
void update_miss(parameters* p);