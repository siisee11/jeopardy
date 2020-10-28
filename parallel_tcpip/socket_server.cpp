//#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <iostream>
#include <thread>
#include <deque>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <vector>

#define  BUFF_SIZE   1024

using std::deque;
std::mutex cout_mu;
std::condition_variable cond;

char client_message[2000];
char buffer[1024];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

struct pmnetMsg
{
	unsigned int datalen;
	char *buffer;
};

class Buffer
{
public:
    void add(pmnetMsg msg) {
        while (true) {
            std::unique_lock<std::mutex> locker(mu);
            cond.wait(locker, [this](){return buffer_.size() < size_;});
            buffer_.push_back(msg);
            locker.unlock();
            cond.notify_all();
            return;
        }
    }
    pmnetMsg remove() {
        while (true)
        {
            std::unique_lock<std::mutex> locker(mu);
            cond.wait(locker, [this](){return buffer_.size() > 0;});
            pmnetMsg back = buffer_.back();
            buffer_.pop_back();
            locker.unlock();
            cond.notify_all();
            return back;
        }
    }
    Buffer() {}
private:
   // Add them as member variables here
    std::mutex mu;
    std::condition_variable cond;

   // Your normal variables here
    deque<pmnetMsg> buffer_;
    const unsigned int size_ = 10;
};

class Producer
{
private:
    Buffer &buffer_;
	int client_socket;

public:
    Producer(Buffer& buffer, int sockfd)
		: buffer_(buffer), client_socket(sockfd)
    {}

	void run() {
		char   buff_rcv[BUFF_SIZE+5];
		char   buff_snd[BUFF_SIZE+5];

		read ( client_socket, buff_rcv, BUFF_SIZE);
		printf( "receive: %s\n", buff_rcv);

		pmnetMsg msg1;
		msg1.datalen = strlen( buff_rcv);
		msg1.buffer = buff_rcv;

		buffer_.add(msg1);
		{
			std::unique_lock<std::mutex> lock(cout_mu);
			std::cout << "Produced: " << msg1.datalen << std::endl;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(50));

		sprintf( buff_snd, "%d : client says %s", strlen( buff_rcv), buff_rcv);
		write( client_socket, buff_snd, strlen( buff_snd)+1);          // +1: NULL까지 포함해서 전송

		printf("Exit socketThread \n");
		close( client_socket);

    }
};

class Consumer
{
    Buffer &buffer_;

public:
    Consumer(Buffer& buffer)
		: buffer_(buffer)
	{}
    void run() {
        while (true) {
            pmnetMsg msg2 = buffer_.remove();
			{
				std::unique_lock<std::mutex> lock(cout_mu);
				std::cout << "Consumed: " << msg2.datalen << std::endl;
			}
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
};



int   main( void)
{
    Buffer b;

	int   server_socket;
	int   client_socket;
	socklen_t client_addr_size;

	struct sockaddr_in   server_addr;
	struct sockaddr_in   client_addr;

	char   buff_rcv[BUFF_SIZE+5];
	char   buff_snd[BUFF_SIZE+5];

	/* Consumer start */
	Consumer c(b);
	std::thread consumer_thread(&Consumer::run, &c);

	server_socket  = socket( PF_INET, SOCK_STREAM, 0);
	if( -1 == server_socket)
	{
		printf( "server socket 생성 실패\n");
		exit( 1);
	}

	memset( &server_addr, 0, sizeof( server_addr));
	server_addr.sin_family     = AF_INET;
	server_addr.sin_port       = htons( 4000);
	server_addr.sin_addr.s_addr= htonl( INADDR_ANY);

	if( -1 == bind( server_socket, (struct sockaddr*)&server_addr, sizeof( server_addr) ) )
	{
		printf( "bind() 실행 에러\n");
		exit( 1);
	}

	if( -1 == listen(server_socket, 5))
	{
		printf( "listen() 실행 실패\n");
		exit( 1);
	}

	std::vector<std::thread> threads;

	int i = 0;
	while( 1)
	{
		client_addr_size  = sizeof( client_addr);
		client_socket     = accept( server_socket, (struct sockaddr*)&client_addr, &client_addr_size);

		if ( -1 == client_socket)
		{
			printf( "클라이언트 연결 수락 실패\n");
			exit( 1);
		}


		Producer p(b, client_socket);
		//Accept call creates a new socket for the incoming connection
		threads.push_back(std::thread(&Producer::run, &p));
		std::cout << "Started thread n'" << i << "\n";


		if( i >= 1)
		{
			i = 0;
			for (i = 0; i < 2; i++) {
				threads[i].join();
				std::cout << "Joined thread n'" << i << "\n";
			}
		}
		i++;
	}

	consumer_thread.join();
}
