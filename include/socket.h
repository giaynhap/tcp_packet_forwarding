
#include <pthread.h>

struct socket_conn{
    int  conn_id;
    int  connfd;
    unsigned int  port;
    int tid;
    pthread_mutex_t clients_mutex ;  
    int listenfd;  
     struct sockaddr_in addr;      
};

typedef struct {
    struct sockaddr_in addr; 
    int connfd;             
    int uid;    
   struct socket_conn * parent;
    
} client_t;



int find_available_conn();
int server_register(unsigned int port,int fd);
int forward_packet_client(unsigned char command,int client_fd,void *buff,size_t size);

#define MAX_CONN 200
#define PORT_START 8000
#define MAX_CLIENTS 100
#define BUFFER_SZ 2048


extern struct socket_conn  conns[MAX_CONN];
#define  HEADER_SIZE 13 
