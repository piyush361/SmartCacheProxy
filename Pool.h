#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>

typedef struct Pool{
    pthread_mutex_t lock;
    int length;
    struct Connection* head; 
}Pool;

typedef struct Connection{
    int socket;
    long Time;
    int added_To_epoll;
    struct Connection* next;
}Connection;

void add_pool_element(Pool *p , int socket);
int chech_expiry(Pool *p);
void addto_epoll(Pool *p , int epfd);
void remove_expired_elements(Pool *p, int epfd);
int findsocket(Pool *p , int socket);
