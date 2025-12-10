#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "Cache.h"

long start_time = 1764806400;

cache_element* findelement(cache *c, char* url)
{
    pthread_mutex_lock(&c->lock);
    cache_element* p = c->head;

    while (p != NULL)
    {
        if (strcmp(p->url, url) == 0)
        {
            pthread_mutex_unlock(&c->lock);
            return p;
        }

        p = p->next;
    }
    pthread_mutex_unlock(&c->lock);
    return NULL;
}

void addelement(cache* c, char* data, char* url)
{
    pthread_mutex_lock(&c->lock);

    char f = url[0];
    c->host_frequency[(int)f - 97]++;
    
    cache_element *p = c->head;
    cache_element *q = NULL;

    while (p != NULL)
    {
        if (strcmp(p->url, url) == 0)
            break;

        q = p;
        p = p->next;
    }

    if (p != NULL)
    {
        if (q != NULL)
            q->next = p->next;
        else
            c->head = p->next;

        p->lru_timestamp = time(NULL) - start_time;
        p->frequency = c->host_frequency[(int)url[0] - 97];
        p->next = NULL;

        cache_element* tail = c->head;
        if (tail == NULL)
        {
            c->head = p;
        }
        else
        {
            while (tail->next != NULL)
                tail = tail->next;

            tail->next = p;
        }
        pthread_mutex_unlock(&c->lock);
        return;
    }

    cache_element* new = malloc(sizeof(cache_element));
    new->url = strdup(url);
    new->data = strdup(data);
    new->lru_timestamp = time(NULL) - start_time;
    new->next = NULL;
    new->frequency = c->host_frequency[(int)url[0] - 97];

    if (c->head == NULL)
    {
        c->head = new;
        c->length = 1;
        pthread_mutex_unlock(&c->lock);
        return;
    }

    cache_element* tail = c->head;
    while (tail->next != NULL)
        tail = tail->next;

    tail->next = new;
    c->length++;
    pthread_mutex_unlock(&c->lock);
}

void remove_element_lru(cache* c)
{
    pthread_mutex_lock(&c->lock);
    if (c->head == NULL)
    {
        pthread_mutex_unlock(&c->lock);
        return;
    }

    cache_element* p = c->head;
    c->head = p->next;

    //printf("Evicted entry for %s\n", p->url);
    free(p->url);
    free(p->data);
    free(p);

    c->length--;
    pthread_mutex_unlock(&c->lock);
}

void remove_element_lfu(cache *c)
{
    pthread_mutex_lock(&c->lock);

    if (c->head == NULL)
    {
        pthread_mutex_unlock(&c->lock);
        return;
    }
  
    int mn = 100000;
    int low = 0;
    for (int i = 0;i < 26; i++)
    {
        if (c->host_frequency[i] < mn)
        {
            mn = c->host_frequency[i];
            low = i;
        }
    }

    low = low + 97;
    char t = (char)low;

    cache_element* p = c->head;
    cache_element* q = NULL;

    while(p != NULL)
    {
        if (p->url[0] == t)
        {
            break;
        }
        q = p;
        p = p->next;
    }

    if(p != NULL)
    {
        if (q != NULL)
        {
            q->next = p->next;
            p->next = NULL;

            //printf("Evicted entry for %s\n", p->url);
            free(p->url);
            free(p->data);
            free(p);

            c->host_frequency[t]--;
            c->length--;
            pthread_mutex_unlock(&c->lock);
            return;
        }
        else
        {
            cache_element* p = c->head;
            c->head = p->next;

            //printf("Evicted entry for %s\n", p->url);
            free(p->url);
            free(p->data);
            free(p);

            c->length--;
            c->host_frequency[t]--;
            pthread_mutex_unlock(&c->lock);
            return;
        }

    }

    cache_element* pq = c->head;
    c->head = pq->next;

    //printf("Evicted entry for %s\n", p->url);
    free(pq->url);
    free(pq->data);
    free(pq);

    c->length--;
    pthread_mutex_unlock(&c->lock);     
    return;    
}


