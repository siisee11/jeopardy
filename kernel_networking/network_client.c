#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include "client.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aby Sam Ross");


static int __init network_client_init(void)
{
	pr_info(" *** mtp | network client init | network_client_init *** \n");
	tcp_client_connect();

	int len = 4096;
	char response[len+1];
	char reply[len+1];
	int ret = -1;

	/* Send page to server */
	memset(&reply, 0, len+1);
	strcat(reply, "GETPAGE"); 
	send(reply, strlen(reply), MSG_DONTWAIT);
	wait_event_timeout_wrapper();
	receive(response, MSG_DONTWAIT);

	return 0;
}

static void __exit network_client_exit(void)
{
	int len = 49;
	char response[len+1];
	char reply[len+1];

	//DECLARE_WAITQUEUE(exit_wait, current);
	DECLARE_WAIT_QUEUE_HEAD(exit_wait);

	memset(&reply, 0, len+1);
	strcat(reply, "ADIOS"); 
	//tcp_client_send(conn_socket, reply);
	tcp_client_send(conn_socket, reply, strlen(reply), MSG_DONTWAIT);

	//while(1)
	//{
	/*
	   tcp_client_receive(conn_socket, response);
	   add_wait_queue(&conn_socket->sk->sk_wq->wait, &exit_wait)
	   */
	wait_event_timeout(exit_wait,\
			!skb_queue_empty(&conn_socket->sk->sk_receive_queue),\
			5*HZ);
	if(!skb_queue_empty(&conn_socket->sk->sk_receive_queue))
	{
		memset(&response, 0, len+1);
		tcp_client_receive(conn_socket, response, MSG_DONTWAIT);
		//remove_wait_queue(&conn_socket->sk->sk_wq->wait, &exit_wait);
	}

	//}

	if(conn_socket != NULL)
	{
		sock_release(conn_socket);
	}
	pr_info(" *** mtp | network client exiting | network_client_exit *** \n");
}

	module_init(network_client_init)
module_exit(network_client_exit)
