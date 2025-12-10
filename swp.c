#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include "proxy_parse.c"
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <semaphore.h>
#include "Cache.c"
#include "stats.c"
#include "Pool.c"

#define MAX_CLIENTS 10
#define MAX_BYTES 4096

pthread_mutex_t lock;
sem_t semaphore;
pthread_t Thread_id[MAX_CLIENTS];
pthread_t Logger_Thread;
pthread_t Pool_thread;

int port_number_proxy = 8080;
int proxy_socketId;

cache proxy_cache;
parameters P;
Pool pool;



void* logger_thread()
{
    while (1) { 
        writethelog(&P);
        sleep(10);
    }

    return NULL;
}


int connectRemoteServer(const char *host_addr, int port_number)
{
    int remotesocketId = socket(AF_INET, SOCK_STREAM, 0);
    if (remotesocketId < 0)
    {
        perror("Error creating socket");
        exit(1);
    }

    struct hostent *host = gethostbyname(host_addr);
    if (host == NULL)
    {
        herror("Error resolving host");
        exit(1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    memcpy(&server_addr.sin_addr.s_addr, host->h_addr, host->h_length);

    if (connect(remotesocketId, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error connecting to web server");
        close(remotesocketId);
        exit(1);
    }

    return remotesocketId;
}

int handle_request( char *host_address, const char *request , cache *c)
{
    char buffer[MAX_BYTES];
    
    cache_element* p = findelement(c , host_address);
    
    if (p != NULL)
    {
        update_hit(&P);
        printf("Cache HITTT\n");
        //printf("Cache length: %d\n",proxy_cache.length);
        return 0;
    }

   else
   {
     update_miss(&P);
    int remotesocketID = connectRemoteServer(host_address, 80);

    if (send(remotesocketID, request, strlen(request), 0) < 0)
    {
        perror("send failed");
        close(remotesocketID);
        return -1;
    }

    int bytes_recv;
    struct timeval timeout;
    timeout.tv_sec = 3;  
    timeout.tv_usec = 0; 

   setsockopt(remotesocketID, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    while ((bytes_recv = recv(remotesocketID, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[bytes_recv] = '\0';
       // printf("Reponse received\n");
    }

    if (proxy_cache.length > 5)
    {
        printf("Cache length exceeded.\n");
        c->eviction_policy(c);
        printf("Cache length: %d\n",proxy_cache.length);
    }
    //printf("Cache length: %d\n",proxy_cache.length);
    addelement(c, buffer , host_address);
   // printf("lllllll\n");
    close(remotesocketID);

   }
    return 0;
}


void* request_thread_function(void* clfd)
{
    sem_wait(&semaphore);
    int p;
    sem_getvalue(&semaphore,&p);
    printf("Semaphore value is : %d\n" , p);

    int persistent_flag = 0;
    
    int socket = *(int *)clfd;
    free(clfd);


    int bytes_incoming_from_client;
    int buffer_len;

    char * buffer = (char*)calloc(MAX_BYTES,sizeof(char));

    struct timeval timeout;
    timeout.tv_sec = 3;  
   timeout.tv_usec = 0; 

   setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));    

    memset(buffer ,0 , MAX_BYTES);
    bytes_incoming_from_client = recv(socket , buffer , MAX_BYTES,0);

    while (bytes_incoming_from_client > 0)
    {
        buffer_len = strlen(buffer);
         if(strstr(buffer,"\r\n\r\n"))
         {
            break;
         }
         else
         {
            bytes_incoming_from_client = recv(socket , buffer + buffer_len , MAX_BYTES - buffer_len , 0);
         }
    }

  if (strlen(buffer) > 0)
    {    
        struct ParsedRequest *request = ParsedRequest_create(); 
        buffer_len = strlen(buffer);
    if (ParsedRequest_parse(request , buffer , buffer_len) < 0)
    {
        perror("Parsing failed.\n");

        return NULL;
    }
    
    printf("Host is : %s \n ", request->host);

    struct ParsedHeader *r = ParsedHeader_get(request , "Connection");
    if (r != NULL && (findsocket(&pool, socket) != 1) && strcmp(r->value , "keep-alive") == 0)
    {
        add_pool_element(&pool , socket);
    }

    if (handle_request(request->host,buffer , &proxy_cache) < 0)
    {
        perror("Request not sent.\n");
        return NULL;
    }  

    }   
    //printf("Request sent......\n");    
    if (findsocket(&pool, socket) != 1)
    {  
      shutdown(socket, SHUT_RDWR);
      close(socket);
    }
    free(buffer);
    printf("Semaphore Posted\n");
    sem_post(&semaphore);

    return NULL;
}


void* persistent_connections_thread()
{
    int epfd = epoll_create1(0);
    //printf("inside thread\n");
    while (1)
    {
        remove_expired_elements(&pool  , epfd);
       // printf("hjsf\n");
        addto_epoll(&pool, epfd);

        struct epoll_event events[64];
        int n = epoll_wait(epfd, events, 64, 15000);

        for (int i = 0;i<n; i++)
        {
            int sock = events[i].data.fd;

            int *sockptr = malloc(sizeof(int));
            *sockptr = sock;

            pthread_t tid;
            pthread_create(&tid, NULL, request_thread_function, sockptr);
            pthread_detach(tid);
        }
        
        //sleep(15);
    }
    return NULL;
}


int main(int argc, char *argv[])
{
    sem_init(&semaphore, 0, MAX_CLIENTS);
    pthread_mutex_init(&lock, NULL);


    if (argc == 2)
        port_number_proxy = atoi(argv[1]);

    int proxy_socketId = socket(AF_INET , SOCK_STREAM, 0);
    int client_socketId;

   pthread_mutex_init(&P.lock, NULL);
   initialize(&P);
   proxy_cache.head = NULL;
   proxy_cache.eviction_policy = remove_element_lfu;
   proxy_cache.length = 0;
   pthread_mutex_init(&proxy_cache.lock,NULL);

    
    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY; 

    int opt = 1;
if (setsockopt(proxy_socketId, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt failed");
    exit(1);
}


    if (bind(proxy_socketId, (struct sockaddr *)&server_addr , sizeof(server_addr)) < 0)
    {
        perror("Binding of the socket Failed!");
        exit(1);
    }    


    int listen_status = listen(proxy_socketId , MAX_CLIENTS);
    if(listen_status < 0)
    {
        perror("Listening function Failed!");
        exit(1);
    }

    int i =0;
    int connected_clients[MAX_CLIENTS];

 
        if (pthread_create(&Logger_Thread , NULL , logger_thread , NULL) < 0 )
        {
            perror("Logger Thread formation failed.\n");
        }

        if (pthread_create(&Pool_thread , NULL , persistent_connections_thread , NULL) < 0 )
        {
            perror("Pool Thread formation failed.\n");
        }

   printf("WEB PROXY STARTED.....\n");
   printf("Binded on the port 8080...\n");
    while(1)
    {
        memset(&client_addr , 0 , sizeof(client_addr));
        socklen_t client_len = sizeof(client_addr);

        client_socketId = accept(proxy_socketId , (struct sockaddr *)&client_addr ,&client_len);
        //printf("%d \n", client_socketId);
        if (client_socketId < 0)
      {
       perror("Client socket opening failed!");
       exit(1);
       }
    struct sockaddr_in* client_pt = (struct sockaddr_in *)&client_addr;
    struct in_addr ip_addr = client_pt->sin_addr;
    char str[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &ip_addr, str, INET_ADDRSTRLEN );
    printf("Client is connected with port number: %d and ipaddress: %s\n",ntohs(client_addr.sin_port),str);

    int *sockptr = malloc(sizeof(int));
    *sockptr = client_socketId;

    if(pthread_create(&Thread_id[i],NULL,request_thread_function, (void*)sockptr)  <  0)
    {
        perror("Thread formation failed.\n");
        exit(1);
    }  
        i = (i+1) % MAX_CLIENTS;

  }

    close(proxy_socketId);
    return 0;
}
