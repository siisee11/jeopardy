#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include<pthread.h>

#define  BUFF_SIZE   1024

void * clientThread(void *arg)
{
	int   client_socket;
	struct sockaddr_in   server_addr;
	char   buff[BUFF_SIZE+5];

	client_socket  = socket( PF_INET, SOCK_STREAM, 0);
	if( -1 == client_socket)
	{
		printf( "socket 생성 실패\n");
		exit( 1);
	}

	memset( &server_addr, 0, sizeof( server_addr));
	server_addr.sin_family     = AF_INET;
	server_addr.sin_port       = htons( 4000);
	server_addr.sin_addr.s_addr= inet_addr( "127.0.0.1");

	if( -1 == connect( client_socket, (struct sockaddr*)&server_addr, sizeof( server_addr) ) )
	{
		printf( "접속 실패\n");
		exit( 1);
	}

	printf( "[Client] send msg\n");
	write( client_socket, "hello", 6);      // +1: NULL까지 포함해서 전송
	read ( client_socket, buff, BUFF_SIZE);
	printf( "[Server says] : %s\n", buff);
	close( client_socket);
	pthread_exit(NULL);
}


int main(){
	pthread_t tid;

	if( pthread_create(&tid, NULL, clientThread, NULL) != 0 )
		printf("Failed to create thread\n");

	sleep(1);

	pthread_join(tid,NULL);
	printf("[Client] Joined \n");

	return 0;
}
