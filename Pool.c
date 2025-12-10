#include<stdio.h>
#include<stdlib.h>
#include<sys/epoll.h>
#include<string.h>
#include<time.h>
#include<errno.h>
#include "Pool.h"

long threshold = 22;

void addto_epoll(Pool *p , int epfd)
{
    pthread_mutex_lock(&p->lock);

    Connection* q = p->head;
    if(q == NULL)
    {
        pthread_mutex_unlock(&p->lock);
        return;
    }

    while(q != NULL)
    {
        if (q->added_To_epoll == 0)
        {
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = q->socket;

        printf("%d\n", q->socket);
        if (fcntl(q->socket, F_GETFD) != -1) {
    //printf("Invalid FD before adding: %d\n", q->socket);


        if (epoll_ctl(epfd, EPOLL_CTL_ADD, q->socket, &ev) == -1) 
        {
            if (errno != EEXIST) {
                perror("epoll_ctl add");
            }
        }
        else
        {
         q->added_To_epoll = 1;
        }
        } 
      else
      {
        printf("Invalid fd..\n");
      }  
    } 
        q = q->next;
    }

    pthread_mutex_unlock(&p->lock);
    return;
}


void add_pool_element(Pool *p , int socket)
{
    pthread_mutex_lock(&p->lock);

    Connection* q = p->head;
    if (q == NULL)
    {
       Connection* new = (Connection *)malloc(sizeof(Connection));
       new->socket = socket;
       new->next = NULL;
       new->added_To_epoll = 0;
       new->Time =   time(NULL);

       p->head = new;
       p->length++;
       pthread_mutex_unlock(&p->lock);
       return; 
    }

    while(q->next != NULL)
    {
        q = q->next;
    }

    Connection* new = (Connection *)malloc(sizeof(Connection));
    new->socket = socket;
    new->added_To_epoll = 0;
    new->next = NULL;
    new->Time =   time(NULL);

    q->next = new;

    p->length++;
    pthread_mutex_unlock(&p->lock);
    return;
}

void remove_expired_elements(Pool *p, int epfd)
{
    pthread_mutex_lock(&p->lock);

    Connection *prev = NULL;
    Connection *cur  = p->head;
   // printf("pool length before: %d\n" , p->length);
    long current = time(NULL);

    while (cur != NULL)
    {
        if ((current - cur->Time) > threshold)
        {
            Connection *expired = cur;

            if (prev == NULL)
            {
                p->head = cur->next;
            }
            else
            {
                prev->next = cur->next;
            }

            cur = cur->next;

            epoll_ctl(epfd, EPOLL_CTL_DEL, expired->socket, NULL);

            close(expired->socket);
            free(expired);

            p->length--;
            continue;
        }
        prev = cur;
        cur  = cur->next;
    }
    //printf("pool length after: %d\n" , p->length);
    pthread_mutex_unlock(&p->lock);
    return;
}


int findsocket(Pool *p, int socket)
{
    pthread_mutex_lock(&p->lock);

    Connection* q = p->head;
    if (q == NULL) 
    {
        pthread_mutex_unlock(&p->lock);
        return -1;
    }
    while (q != NULL)
    {
        if (q->socket == socket)
        {
            pthread_mutex_unlock(&p->lock);
            return 1;
        }
        q = q->next;
    }

    pthread_mutex_unlock(&p->lock);
    return 0;
}


