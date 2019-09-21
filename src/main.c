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



static _Atomic unsigned int cli_count = 0;
static int uid = 10;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
 
char *_strdup(const char *s) {
    size_t size = strlen(s) + 1;
    char *p = malloc(size);
    if (p) {
        memcpy(p, s, size);
    }
    return p;
}

 
void queue_add(client_t *cl){
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!clients[i]) {
            clients[i] = cl;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

 
void queue_delete(int uid){
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {
            if (clients[i]->uid == uid) {
                clients[i] = NULL;
                break;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

  
void broadcast_send(void * data,size_t size,int sender){

pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i <MAX_CLIENTS; ++i){
        if (clients[i]&& sender != clients[i]->uid) {
            if (write(clients[i]->connfd, data, size) < 0) {
                perror("error * write to descriptor failed");
                break;
            }
        }
    }
pthread_mutex_unlock(&clients_mutex);

}
  
 
void print_client_addr(struct sockaddr_in addr){
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

/*
[head]=[command=1][size=8][socid=4]
*/
char cache_buff[BUFFER_SZ*3];
void *handle_client(void *arg){
    char buff_in[BUFFER_SZ ];

    int read_size = 0;
    int rlen;

    cli_count++;
    client_t *cli = (client_t *)arg;

    printf("\t->accept ");
    print_client_addr(cli->addr);
    printf(" referenced by %d\n", cli->uid);

    int index  = -1;
    struct socket_conn * conn = NULL;
    while ((rlen = read(cli->connfd, buff_in, sizeof(buff_in) - 1)) > 0) {
    
        memcpy(cache_buff+read_size,buff_in,rlen);
        read_size+=rlen;
        printf("reav buffer size : %ld\n",rlen);
        check_packet:
        if (read_size>=HEADER_SIZE){

            char command;
            size_t size;
            int sockid;

            memcpy(&command,cache_buff,1);
            memcpy(&size,cache_buff+1,8);
            memcpy(&sockid,cache_buff+9,4);
            
             printf("rev response  command:%d - size: %ld - readsize: %ld- sockid:%d\n",command,size,read_size,sockid);

            if (command == 100 && conn == NULL){
                
                index = server_register(0,cli->connfd);
                conn = &conns[index];
                read_size-=HEADER_SIZE;
               
                memcpy(cache_buff,cache_buff+HEADER_SIZE,read_size);
            }else if (command==102||command == 101){

                if (size  <= read_size - HEADER_SIZE){
                   
                   if (size>0){
                     char *packet= malloc(size);
                     memcpy(packet,cache_buff+HEADER_SIZE,size);
                     forward_packet_client(command,sockid,packet,size);
                   }
                 
                   read_size-=(HEADER_SIZE+size);
                   printf("Execute command %d\n",command);
                    if (read_size<0){
                        read_size = 0;
                    }
                   memcpy(cache_buff,cache_buff+HEADER_SIZE+size,read_size);
                   if (read_size>=HEADER_SIZE){
                       goto check_packet;
                   }
                }
            }
          

        }

        printf("=>>>>>>> store size: %d\n",read_size);
       
        
    }
    
  //  pthread_cancel(conn->tid);
    if (conn!= NULL){
        close(conn->listenfd);
    }
    close(cli->connfd);
    queue_delete(cli->uid);
    printf("\t->quit ");
    print_client_addr(cli->addr);
    printf(" referenced by %d %d\n", cli->uid);
    free(cli);
    cli_count--;
    pthread_detach(pthread_self());

    return NULL;
}

int main(int argc, char *argv[]){
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    pthread_t tid;

      if (argc <2 ){
         printf("error * no arg port\n");
         return -1;
      }
      printf("\nSetting:\n\t->port: %s\n",argv[1]);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    signal(SIGPIPE, SIG_IGN);
 
    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("\t  error* socket binding failed");
        return EXIT_FAILURE;
    }

    if (listen(listenfd, 10) < 0) {
        perror("\t  error* socket listening failed");
        return EXIT_FAILURE;
    }

    printf("SERVER STARTED: \n");

    while (1) {
        socklen_t clilen = sizeof(cli_addr);
        connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

        if ((cli_count + 1) == MAX_CLIENTS) {
            printf("\t-> max clients reached\n");
            printf("<< reject ");
            print_client_addr(cli_addr);
            printf("\n");
            close(connfd);
            continue;
        }
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->addr = cli_addr;
        cli->connfd = connfd;
        cli->uid = uid++;
        queue_add(cli);
        pthread_create(&tid, NULL, &handle_client, (void*)cli);
        sleep(1);
    }
    return EXIT_SUCCESS;
}