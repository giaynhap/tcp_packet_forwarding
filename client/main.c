#if defined WIN32
#include <winsock.h>
#else
#define closesocket close
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

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
#include "socketmgr.h"

#define BUFFERSIZE 512
#define PROTOPORT 5193
#pragma comment(lib,"ws2_32.lib")
size_t count_time;

void clear_winsock() {
    #if defined WIN32
        WSACleanup();
    #endif
}


int act_connect(char host[],unsigned int port) {
    #if defined WIN32
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2 ,2), &wsaData);
        if (iResult != 0) {
            printf("error at WSASturtup\n");
            return -1;
        }
    #endif

 
	int socketfd;
	socketfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketfd < 0) {
		printf("socket creation failed.\n");
		closesocket(socketfd);
		clear_winsock();
		return -1;
	}
 
	struct sockaddr_in sad;
	memset(&sad, 0, sizeof(sad));
	sad.sin_family = AF_INET;
	sad.sin_addr.s_addr = inet_addr(host);  
	sad.sin_port = htons(port);  
 
	if (connect(socketfd, (struct sockaddr *) &sad, sizeof(sad)) < 0) {
		printf("Failed to connect.\n");
		closesocket(socketfd);
		clear_winsock();
		return -1;
	}
    return socketfd;
}

int act_wait_rev_buff(int socketfd,size_t max_size, int (onrev)(int ,void*,size_t )){

    void * buf =malloc(max_size);
    size_t bytes;
	int err = 1;
    while (1) {

        if ((bytes= recv(socketfd, buf,max_size, 0)) <= 0) {
			printf("recv() failed or connection closed prematurely");
			closesocket(socketfd);
			clear_winsock();
			return 0;
		}
		err = onrev(socketfd,buf,bytes);
        if (err != 0){
            break;
        }
    }
    free(buf);
    return err;
}

int act_send_buff(int socketfd,void *buff,size_t len){
    if (send(socketfd, buff, len, 0) != len) {
		printf("send() sent a different number of bytes than expected");
		closesocket(socketfd);
		clear_winsock();
		return 0;
	}
    return 1;
}
int act_close(int socketfd){
    closesocket(socketfd);
	clear_winsock();
} 
 
 
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int conn;
int port ;
void *handle_client(void *arg){

    conn = act_connect("0.0.0.0",port);
    char packet[13];
    char command = 100;
    size_t size = 0; 
    memcpy(packet,&command,1);
    memcpy(packet+1,&size,8);
    act_send_buff(conn,packet,13);

    char buff[2048];
    char cache_buff[2048*2];
    int read_size = 0;
     while(1){
       size_t bytes = read(conn,buff,2048);
       if (bytes<=0){
           break;
       }

        memcpy(cache_buff+read_size,buff,bytes);
        read_size+=bytes;
        check_packet:
        if(read_size>=13){
            char command;
            size_t size;
            int sockid;

            memcpy(&command,cache_buff,1);
            memcpy(&size,cache_buff+1,8);
            memcpy(&sockid,cache_buff+9,4);
 
            printf("rev buffer from server:\n\tcommand: %d\n\tsize:%ld\n\tsockid: %d\n\tread_size: %ld\n\tbytes: %ld\n",command,size,sockid,read_size,bytes);
            if (size  <= read_size - 13 && read_size >= 13){
                
                char *packet= malloc(size);
                
                if (size > 0){
                    memcpy(packet,cache_buff+13,size);
                    forward_packet( socket_add(conn,sockid,2402),packet,size); 
                }
                else{
                    struct socket_host * sock = socket_exist(sockid);
                    if (sock!=NULL){
                        if (sock->clientfd > 0){
                            closesocket(sock->clientfd);
                        }
                        sock->pid = 0;
                        socket_delete(sock->pid);
                    }
                    free(packet);
                }
                read_size-=13+size;
                if (read_size<0){
                    read_size = 0;
                }
                
                memcpy(cache_buff,cache_buff+13+size,read_size);
                if (read_size>13){
                    goto check_packet;
                }
            }
        }
     }
}
 
 int main(int argc,char * argv[]){
   int tid;
   int test = 1;
   port =atoi(argv[1]);
   handle_client(NULL);
 }

 