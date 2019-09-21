#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
struct list_header{
    struct list_header * next;
    struct list_header * prev;
};
struct socket_host{
    struct list_header list;
    int pid;
    int confd;
    int host_port;
    int clientfd;
    int tid;

 
     pthread_mutex_t clients_mutex ;  
};


int act_connect(char host[],unsigned int port) ;
struct socket_host *  socket_add(int confd,int pid,int port);
int forward_packet(struct socket_host * sock,void *buffer, size_t size);
struct socket_host * socket_exist(int pid);