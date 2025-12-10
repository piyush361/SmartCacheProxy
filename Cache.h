#include<time.h>
#include<pthread.h>

typedef struct cache_element {
    char* data;
    char* url;
    int frequency;
    time_t lru_timestamp;
    struct cache_element* next;
} cache_element;

typedef struct cache {
    int length;
    cache_element* head;
    void (*eviction_policy)(struct cache *c);
    int host_frequency[26];
    pthread_mutex_t lock;
} cache;

cache_element* findelement(cache* c, char* url);
void addelement(cache* c, char* data, char* url);
void remove_element_lru(cache* c);
void remove_element_lfu(cache *c);
