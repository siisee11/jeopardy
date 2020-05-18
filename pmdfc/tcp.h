/*
 * tcp.h
 *
 * In kernel networking
 *
 * Copyright (c) 2019, Jaeyoun Nam, SKKU.
 */

#ifndef PMNET_TCP_H
#define PMNET_TPC_H

#include <linux/socket.h>
#ifdef __KERNEL__
#include <net/sock.h>
#include <linux/tcp.h>
#else
#include <sys/socket.h>
#endif
#include <linux/inet.h>
#include <linux/in.h>


#define PORT 			(2325)
#define CLIENT_PORT 	(2346)
#define DEST_ADDR		("115.145.173.67")
#define CLIENT_ADDR		("10.0.2.15")

#define PMNET_MAX_PAYLOAD_BYTES  (8192 - sizeof(struct pmnet_msg))

/* same as hb delay, we're waiting for another node to recognize our hb */
#define PMNET_RECONNECT_DELAY_MS_DEFAULT	2000
#define PMNET_KEEPALIVE_DELAY_MS_DEFAULT	2000
#define PMNET_IDLE_TIMEOUT_MS_DEFAULT		30000
#define PMNET_TCP_USER_TIMEOUT			0x7fffffff


struct pmnet_msg
{
	__be16 magic;
	__be16 data_len;
	__be16 msg_type;
	__be16 pad1;
	__be32 sys_status;
	__be32 status;
	__be32 key;
	__be32 msg_num;
	__u8  buf[0];
};

static unsigned int inet_addr(const char *str)
{
	int a,b,c,d;
	char arr[4];
	sscanf(str,"%d.%d.%d.%d",&a,&b,&c,&d);
	arr[0] = a; arr[1] = b; arr[2] = c; arr[3] = d;
	return *(unsigned int*)arr;
}


int pmnet_send_message(u32 msg_type, u32 key, void *data, u32 len,
		       u8 target_node, int *status);
int pmnet_send_message_vec(u32 msg_type, u32 key, struct kvec *vec,
			   size_t veclen, u8 target_node, int *status);
int pmnet_recv_message(u32 msg_type, u32 key, void *data, u32 len,
		       u8 target_node);

struct pmnm_node;
int pmnet_start_listening(struct pmnm_node *node);
void pmnet_stop_listening(struct pmnm_node *node);
void pmnet_disconnect_node(struct pmnm_node *node);


int pmnet_init(void);
void pmnet_exit(void);


#endif /* PMNET_TCP_H */
