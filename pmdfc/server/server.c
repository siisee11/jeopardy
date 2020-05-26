#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <fcntl.h> 
#include <unistd.h>
#include <errno.h>

#include "tcp.h"
#include "tcp_internal.h"

#define MAX 4096 
#define SA struct sockaddr 

struct pmnet_msg_in {
	struct pmnet_msg *hdr;
	void *page;
	size_t page_off;
};

struct pmnet_msg_in *msg_in;
void *saved_page;

static void pmnet_init_msg(struct pmnet_msg *msg, uint16_t data_len, 
		uint16_t msg_type, uint32_t key)
{
	memset(msg, 0, sizeof(struct pmnet_msg));
	msg->magic = htons(PMNET_MSG_MAGIC);
	msg->data_len = htons(data_len);
	msg->msg_type = htons(msg_type);
//	msg->sys_status = htonl(PMNET_ERR_NONE);
	msg->sys_status = htonl(0);
	msg->status = 0;
	msg->key = htonl(key);
}

int pmnet_send_message(int sockfd, uint32_t msg_type, uint32_t key, 
		void *data, uint16_t datalen)
{
	int ret = 0;

	/* TODO: send message by sendmmsg() */
	struct msghdr msghdr1;
	struct pmnet_msg *msg = NULL;
	struct iovec iov_msg[2];

	msg = calloc(1, sizeof(struct pmnet_msg));
	if (!msg) {
		printf("failed to allocate a pmnet_msg!\n");
		ret = -ENOMEM;
		goto out;
	}

	pmnet_init_msg(msg, datalen, msg_type, key); 

	memset(iov_msg, 0, sizeof(iov_msg));
	iov_msg[0].iov_base = msg;
	iov_msg[0].iov_len = sizeof(struct pmnet_msg);
	iov_msg[1].iov_base = data;
	iov_msg[1].iov_len = datalen;

	memset(&msghdr1, 0, sizeof(msghdr1));
	msghdr1.msg_iov = iov_msg;
	msghdr1.msg_iovlen = 2;
	
	/* send message and data */
//	ret = write(sockfd, msg, sizeof(struct pmnet_msg)); 
//	ret = write(sockfd, data, datalen); 
	ret = sendmsg(sockfd, &msghdr1, MSG_DONTWAIT);

out:
	return ret;
}

/* this returns -errno if the header was unknown or too large, etc.
 * after this is called the buffer us reused for the next message */
static int pmnet_process_message(int sockfd, struct pmnet_msg *hdr)
{
	int ret = 0;
	int status;
	char reply[1024];
	void *data;
	size_t datalen;
	char *to_va, *from_va;

	printf("%s: processing message\n", __func__);

	switch(ntohs(hdr->magic)) {
		case PMNET_MSG_STATUS_MAGIC:
			printf("PMNET_MSG_STATUS_MAGIC\n");
			goto out; 
		case PMNET_MSG_KEEP_REQ_MAGIC:
			printf("PMNET_MSG_KEEP_REQ_MAGIC\n");
			goto out;
		case PMNET_MSG_KEEP_RESP_MAGIC:
			printf("PMNET_MSG_KEEP_RESP_MAGIC\n");
			goto out;
		case PMNET_MSG_MAGIC:
			printf("PMNET_MSG_MAGIC\n");
			break;
		default:
			printf("bad magic\n");
			ret = -EINVAL;
			goto out;
			break;
	}

	switch(ntohs(hdr->msg_type)) {
		case PMNET_MSG_HOLA:
			printf("CLIENT-->SERVER: PMNET_MSG_HOLA\n");

			/* send hello message */
			memset(&reply, 0, 1024);

			ret = pmnet_send_message(sockfd, PMNET_MSG_HOLASI, 0, 
				reply, 1024);

			printf("SERVER-->CLIENT: PMNET_MSG_HOLASI(%d)\n", ret);
			break;

		case PMNET_MSG_HOLASI:
			printf("SERVER-->CLIENT: PMNET_MSG_HOLASI\n");
			break;

		case PMNET_MSG_PUTPAGE:
			printf("CLIENT-->SERVER: PMNET_MSG_PUTPAGE\n");
			/* TODO: segmentation fault occur */
			from_va = msg_in->page;
			memcpy(saved_page, from_va, 4096);
			memset(&reply, 0, 1024);
			ret = pmnet_send_message(sockfd, PMNET_MSG_SUCCESS, 0, 
				reply, 1024);
			printf("SERVER-->CLIENT: PMNET_MSG_SUCCESS(%d)\n", ret);
			break;

		case PMNET_MSG_GETPAGE:
			printf("CLIENT-->SERVER: PMNET_MSG_GETPAGE\n");
			ret = pmnet_send_message(sockfd, PMNET_MSG_SENDPAGE, 0, 
				saved_page, 4096);

			printf("SERVER-->CLIENT: PMNET_MSG_SENDPAGE(%d)\n",ret);
			break;

		case PMNET_MSG_SENDPAGE:
			printf("SERVER-->CLIENT: PMNET_MSG_SENDPAGE\n");

			break;
	}

out:
	return ret;
}



static int pmnet_advance_rx(int sockfd)
{
	struct pmnet_msg *hdr;
	int ret;
	void *data;
	size_t datalen;

	printf("pmnet_advance_rx: start\n");

	if (msg_in->page_off < sizeof(struct pmnet_msg)) {
		data = msg_in->hdr + msg_in->page_off;
		datalen = sizeof(struct pmnet_msg) - msg_in->page_off;
		printf("first read datalen=%zu\n", datalen);
		ret = read(sockfd, data, datalen);

		/* return value 0 means socket disconnected */
		printf("read ret=%d\n", ret);

		if (ret > 0) {
			msg_in->page_off += ret;
			if (msg_in->page_off == sizeof(struct pmnet_msg)) {
				hdr = msg_in->hdr;
				if (ntohs(hdr->data_len) > PMNET_MAX_PAYLOAD_BYTES)
					ret = -EOVERFLOW;
			}
		}
		if (ret <= 0)
			goto out;
	}

	if (msg_in->page_off < sizeof(struct pmnet_msg)) {
		/* oof, still don't have a header */
		goto out;
	}

	/* this was swabbed above when we first read it */
	hdr = msg_in->hdr;

	printf("at page_off %zu, datalen=%u\n", msg_in->page_off, ntohs(hdr->data_len));

	/* 
	 * do we need more payload? 
	 * Store payload to sc->sc_clean_page
	 */
	if (msg_in->page_off - sizeof(struct pmnet_msg) < ntohs(hdr->data_len)) {
		/* need more payload */
		data = msg_in->page + msg_in->page_off - sizeof(struct pmnet_msg);
		datalen = (sizeof(struct pmnet_msg) + ntohs(hdr->data_len) -
			  msg_in->page_off);
		ret = read(sockfd, data, datalen);
		if (ret > 0)
			msg_in->page_off += ret;
		if (ret <= 0)
			goto out;
	}

	if (msg_in->page_off - sizeof(struct pmnet_msg) == ntohs(hdr->data_len)) {
		/* we can only get here once, the first time we read
		 * the payload.. so set ret to progress if the handler
		 * works out. after calling this the message is toast */
		ret = pmnet_process_message(sockfd, hdr);
		if (ret == 0)
			ret = 1;
		msg_in->page_off = 0;
	}

out:
	printf("pmnet_advance_rx: end\n");
	return ret;
}

static void pmnet_rx_until_empty(int sockfd)
{
	int ret;

	do {
		ret = pmnet_advance_rx(sockfd);
	} while (ret > 0);

	if (ret <= 0 && ret != -EAGAIN) {
		printf("pmnet_rx_until_empty: saw error %d, closing\n", ret);
		/* not permanent so read failed handshake can retry */
	}
}

void init_msg()
{
	void *page;
	struct pmnet_msg *msg;

    page = calloc(1, 4096);
	msg = calloc(1, sizeof(struct pmnet_msg));
	msg_in = calloc(1, sizeof(struct pmnet_msg_in*));

	msg_in->page_off = 0;
	msg_in->page = page;
	msg_in->hdr= msg;

	return;
}

// Driver function 
int main() 
{ 
	int sockfd, connfd, len; 
	struct sockaddr_in servaddr, cli; 

	init_msg();

	saved_page = calloc(1, 4096);

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		printf("socket creation failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully created..\n"); 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(PORT); 

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
		printf("socket bind failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully binded..\n"); 

	// Now server is ready to listen and verification 
	if ((listen(sockfd, 5)) != 0) { 
		printf("Listen failed...\n"); 
		exit(0); 
	} 
	else
		printf("Server listening..\n"); 
	len = sizeof(cli); 

	// Accept the data packet from client and verification 
	connfd = accept(sockfd, (SA*)&cli, &len); 
	if (connfd < 0) { 
		printf("server acccept failed...\n"); 
		exit(0); 
	} 
	else
		printf("server acccept the client...\n"); 

	// Function for chatting between client and server 
	pmnet_rx_until_empty(connfd); 

	// After chatting close the socket 
	close(sockfd); 
} 

