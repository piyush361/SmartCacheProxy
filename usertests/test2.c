#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>


const char *sites[][2] = {
    {"hulu.com",        "/"},
    {"imdb.com",        "/"},
    {"jetbrains.com",   "/"},
    {"kernel.org",      "/"},
    {"linkedin.com",    "/"},
    {"microsoft.com",   "/"},
    {"netflix.com",     "/"},
    {"opera.com",       "/"},
    {"python.org",      "/"},
    {"twitch.tv",       "/"}
};

int main() {

    srand(time(NULL));
    char buffer[4096];
    const char *method = "GET";
    const char *protocol = "http";
    // const char *host = "stackoverflow.com";
    // const char *path = "/questions";

   

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);                 
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 

    
    int v = 1;
    while(v)
    {

        int idx = rand() % 10;
        const char *host = sites[idx][0];
        const char *path = sites[idx][1];


        snprintf(buffer, sizeof(buffer),
        "%s %s://%s%s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: MyClient/1.0\r\n"
        "Accept: */*\r\n"
        "\r\n",
        method, protocol, host, path, host
    );


        int clientfd = socket(AF_INET , SOCK_STREAM , 0);
    if(clientfd < 0) { perror("socket failed"); return 1; }


    if(connect(clientfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        return 1;
    }
        send(clientfd, buffer, strlen(buffer), 0);
        close(clientfd);

        sleep(1);
       v++;
    }

    return 0;
}


