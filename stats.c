#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<string.h>
#include<time.h>
#include"stats.h"

void initialize(parameters* p)
{
    pthread_mutex_lock(&p->lock);
    
    p->cache_hits = 0;
    p->cache_misses = 0;
    p->total_requests = 0;

    p->rate = 0.00;
    pthread_mutex_unlock(&p->lock);

    return;
}


void update_hit(parameters* p)
{
    pthread_mutex_lock(&p->lock);

    p->total_requests++;
    p->cache_hits++;

    p->rate = (double)(p->cache_hits) / (p->total_requests);
 printf("RATE IS : %f\n", p->rate);

    pthread_mutex_unlock(&p->lock);
    return;
}

void update_miss(parameters* p)
{
    pthread_mutex_lock(&p->lock);

    p->total_requests++;
    p->cache_misses++;

    p->rate = (double)(p->cache_hits) / (p->total_requests);
 printf("RATE IS: %f\n", p->rate);
    pthread_mutex_unlock(&p->lock);
    return;
}




void get_timestamp(char* buffer, int size)
{
  time_t now = time(NULL);
  struct tm *local_time = localtime(&now);
  //char buffer[80];

  strftime(buffer, size, "%Y-%m-%d %H:%M:%S", local_time);
}

void writethelog(parameters *p)
{
   pthread_mutex_lock(&p->lock);
   FILE* fptr;
   fptr = fopen("Proxy_stats.txt", "a+");

   if(fptr == NULL)
   {
    pthread_mutex_unlock(&p->lock);
    perror("Error opening the file");
    return;
   }


   char buffer[4000];
   char ts[80];

   get_timestamp(ts , sizeof(ts));
   snprintf(buffer, sizeof(buffer),
     "Stats for time : %s\n\n 'Total Requests' : %d \n   'Cache Hits' : %d \n 'Cache Misses' : %d \n   'Hit  Rate' : %f \n\n",
     ts,p->total_requests , p->cache_hits , p->cache_misses , p->rate
   );

  // fprintf("The Stats for the time : %s \n\n\n ", ts);
   int x = fwrite(buffer, sizeof(char),strlen(buffer), fptr);

   if (x != strlen(buffer))
   {
     perror("Writing failed\n");
   }

   fclose(fptr);
   pthread_mutex_unlock(&p->lock);
   return;
}

