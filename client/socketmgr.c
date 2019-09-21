#include "socketmgr.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

struct socket_host * socks = NULL;
struct socket_host * socket_exist(int pid);


void response_to_host( struct socket_host *  sock,void * buff, size_t size){
     pthread_mutex_lock(&sock->clients_mutex);
     write(sock->confd,buff,size);
     pthread_mutex_unlock(&sock->clients_mutex);

}
void *handle_socket_host(void *arg){

    struct socket_host *  sock = ( struct socket_host * )arg;
    printf("connect to port %d\n",sock->host_port);
    
       char buff[1024];
    int conn = sock->clientfd ;
    while(1){
       
       size_t bytes = read(conn,buff,1024);
       if (bytes<=0){
              printf("read end\n");
           break;
       }

        char *packet=malloc(13+bytes);
       //socketId
       
       
       
       *(packet) = (char)(101);
       memcpy(packet+1,&bytes,8);
      
       memcpy(packet+9,&(sock->pid),4);
       memcpy(packet+13,&buff,bytes);
       printf("send to server with pid %d : size %ld\n", sock->pid,bytes);
      // write(sock->confd,packet,bytes+13);
       response_to_host(sock,packet,bytes+13);
       free(packet);
    }
    close(sock->clientfd);
    printf("send message close socket\n");
    char *packet=malloc(13);
    memset(packet,0,13);
    
    *(packet) = (char)(102);
    *(packet+1) = (size_t)(0);
    *(packet+9) = (int)(sock->pid);
  //  write(sock->confd,packet,13);
  sock->pid = 0;
    response_to_host(sock,packet,13);
    free(packet);
}

int forward_packet(struct socket_host * sock,void *buffer, size_t size){
     printf("forward_packet to %d\n",sock->pid);
       if (sock->clientfd<=0){
             sock->clientfd = act_connect("0.0.0.0",sock->host_port);
       }
        
    return write(sock->clientfd,buffer,size);
        
}
 struct socket_host *  socket_add(int confd,int pid,int port){

   
    struct socket_host * exist  = socket_exist(pid);
    if (exist != NULL){

        return exist;
    }
    struct socket_host * newsock = malloc(sizeof( struct socket_host));
    newsock->confd = confd;

   
    
    int * pidx = malloc(4);
    memcpy(pidx,&pid,4);
    
    newsock->pid = *pidx;


    newsock->host_port = port;
    newsock->clientfd = act_connect("0.0.0.0",newsock->host_port);
    free(pidx);
    if (pthread_mutex_init(&newsock->clients_mutex, NULL) != 0)
    {
        return NULL;
    }

    pthread_create(&newsock->tid, NULL, &handle_socket_host, newsock);
    if (socks == NULL){
        socks = newsock;
    }else{
        socks->list.next = newsock;
        newsock->list.prev = socks;
    }
    return newsock;
}

struct socket_host * socket_exist(int pid)
{
    struct socket_host * sock = socks;
    while (sock!=NULL){
        if (sock->pid == pid){
            return sock;
        }
        sock = sock->list.next;
    }
    return NULL;
}