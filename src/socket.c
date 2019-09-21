#include <string.h>
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
#include "socket.h"


struct socket_conn  conns[MAX_CONN];

void *handle_conn_client(void *arg);


void clean_sockets()
{
    memset(conns,0,MAX_CONN*sizeof(struct socket_conn));
}
/*
[header]=[command-1byte][size-8byte][client-4byte]
[body]=buffer
*/

int forward_packet_host(unsigned char command,client_t *client,void *buff,size_t size){
   
    printf("send packet to host pid %d command %d \n",client->connfd,command);
    unsigned char * packet = malloc(HEADER_SIZE+size);
    //creat packet;
    if (buff==NULL){
        size =0;
    }
    memcpy(packet,&command,1);
    memcpy(packet+1,&size,8);
    memcpy(packet+9,&(client->connfd),4);
 
    if (buff!=NULL)
    memcpy(packet+HEADER_SIZE,buff,size);
    //send
    pthread_mutex_lock(&client->parent->clients_mutex);
    int error = write(client->parent->connfd, packet, size+HEADER_SIZE);
    printf("send error code : %d\n",error);
    pthread_mutex_unlock(&client->parent->clients_mutex);

    free(packet);
    return error;
}


int forward_packet_client(unsigned char command,int client_fd,void *buff,size_t size){

    if (command==102){
        close(client_fd);
        return 1;
    }else{
        int error = write(client_fd,buff,size);
        free(buff);
        printf("send packt from host to client %d \n",client_fd);
        return error;
    }

}

void *wait_client(void *arg);

int server_register(unsigned int port,int fd)
{
    unsigned int ava_conn =find_available_conn(); 
    if (ava_conn<0){
       return -1;
    }
    port = ava_conn+PORT_START;
    struct socket_conn * conn = &conns[ava_conn];
    
    int listenfd = 0, connfd = 0;

    struct sockaddr_in serv_addr;
   
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    

    while (1){
        serv_addr.sin_port = htons(port);
        if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("\t  error* socket binding failed");
            port++;
        }else{
            break;
        }
    }

    if (listen(listenfd, 10) < 0) {
        perror("\t  error* socket listening failed");
        return EXIT_FAILURE;
    }

    printf("New conn - port: %d \n",port);

    conn->listenfd = listenfd;
    conn->conn_id = ava_conn;
    conn->port = port;
    conn->connfd = fd;
     
    if (pthread_mutex_init(&conn->clients_mutex, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }
    int tid;
    void *mapval = malloc(4);
    memcpy(mapval,&ava_conn,4);
    pthread_create(&tid, NULL, &wait_client, mapval);
    conn->tid = tid;
    return  conn->conn_id;
}

void *wait_client(void *arg){
    int id = *((int *)arg);
    struct socket_conn *conn = &conns[id];
    struct sockaddr_in cli_addr;
    pthread_t tid;
    int connfd;

     while (1) {

        socklen_t clilen = sizeof(cli_addr);
        connfd = accept(conn->listenfd, (struct sockaddr*)&cli_addr, &clilen);
        if (connfd < 0 ){
           continue;
        }
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->addr = cli_addr;
        cli->connfd = connfd;
        cli->parent = conn;
        printf("client connect\n");
        pthread_create(&tid, NULL, &handle_conn_client, (void*)cli);
        sleep(1);
    }
}
void conn_disconnect(struct socket_conn * conn){
       pthread_mutex_destroy(&conn->clients_mutex);
}


void *handle_conn_client(void *arg){
    char buff_out[BUFFER_SZ];
    char buff_in[BUFFER_SZ / 2];
    int rlen;

    client_t *cli = (client_t *)arg;
 
    while ((rlen = read(cli->connfd, buff_in, sizeof(buff_in) )) > 0) {
         printf("rev packet from client %d - size :%ld \n",cli->connfd,rlen);
         forward_packet_host(1,cli,buff_in,rlen);
        
    }
    forward_packet_host(0,cli,NULL,0);
    close(cli->connfd);
    free(cli);
    pthread_detach(pthread_self());
    return NULL;
}


int find_available_conn()
{
    for (unsigned int i=0 ;i< MAX_CONN;i++){
        if (conns[i].conn_id<=0){
            return i;
        }
    }
    return -1;
}

