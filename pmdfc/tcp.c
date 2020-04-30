#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/idr.h>
#include <linux/kref.h>
#include <linux/net.h>
#include <linux/export.h>
#include <net/tcp.h>

#include <asm/uaccess.h>

#include "tcp.h"
#include "tcp_internal.h"

#if 0
#define SC_NODEF_FMT "node %s (num %u) at %pI4:%u"
#define SC_NODEF_ARGS(sc) sc->sc_node->nd_name, sc->sc_node->nd_num,	\
	&sc->sc_node->nd_ipv4_address,		\
	ntohs(sc->sc_node->nd_ipv4_port)
#endif

/* struct workqueue */
static struct workqueue_struct *pmnet_wq;

/* PMNET nodes */
static struct pmnet_node pmnet_nodes[1];

static struct pmnet_handshake *pmnet_hand;
static struct pmnet_msg *pmnet_keep_req, *pmnet_keep_resp;



/* get pmnet_node by number */
static struct pmnet_node * pmnet_nn_from_num(u8 node_num)
{
	BUG_ON(node_num >= ARRAY_SIZE(pmnet_nodes));
	return &pmnet_nodes[node_num];
}


static int pmnet_prep_nsw(struct pmnet_node *nn, struct pmnet_status_wait *nsw)
{
	int ret;

	spin_lock(&nn->nn_lock);
	ret = idr_alloc(&nn->nn_status_idr, nsw, 0, 0, GFP_ATOMIC);
	if (ret >= 0) {
		nsw->ns_id = ret;
		list_add_tail(&nsw->ns_node_item, &nn->nn_status_list);
	}
	spin_unlock(&nn->nn_lock);
	if (ret < 0)
		return ret;

	init_waitqueue_head(&nsw->ns_wq);
	nsw->ns_sys_status = PMNET_ERR_NONE;
	nsw->ns_status = 0;
	return 0;
}


static void sc_kref_release(struct kref *kref)
{
	struct pmnet_sock_container *sc = container_of(kref,
			struct pmnet_sock_container, sc_kref);

	if (sc->sc_sock) {
		sock_release(sc->sc_sock);
		sc->sc_sock = NULL;
	}

	if (sc->sc_page)
		__free_page(sc->sc_page);
	kfree(sc);
}

static void sc_put(struct pmnet_sock_container *sc)
{
	kref_put(&sc->sc_kref, sc_kref_release);
}
static void sc_get(struct pmnet_sock_container *sc)
{
	kref_get(&sc->sc_kref);
}


static struct pmnet_sock_container *sc_alloc(void)
{
	struct pmnet_sock_container *sc, *ret = NULL;
	struct page *page = NULL;
	int status = 0;

	page = alloc_page(GFP_NOFS);
	sc = kzalloc(sizeof(*sc), GFP_NOFS);
	if (sc == NULL || page == NULL)
		goto out;

	kref_init(&sc->sc_kref);

#if 0
	INIT_WORK(&sc->sc_connect_work, o2net_sc_connect_completed);
	INIT_WORK(&sc->sc_rx_work, o2net_rx_until_empty);
	INIT_WORK(&sc->sc_shutdown_work, o2net_shutdown_sc);
	INIT_DELAYED_WORK(&sc->sc_keepalive_work, o2net_sc_send_keep_req);
#endif

//	timer_setup(&sc->sc_idle_timeout, pmnet_idle_timer, 0);

	ret = sc;
	sc->sc_page = page;
	sc = NULL;
	page = NULL;

out:
	if (page)
		__free_page(page);
	kfree(sc);

	return ret;
}


/* ------------------------------------------------------------ */

static void pmnet_sc_queue_work(struct pmnet_sock_container *sc,
				struct work_struct *work)
{
	sc_get(sc);
	if (!queue_work(pmnet_wq, work))
		sc_put(sc);
}
static void pmnet_sc_queue_delayed_work(struct pmnet_sock_container *sc,
					struct delayed_work *work,
					int delay)
{
	sc_get(sc);
	if (!queue_delayed_work(pmnet_wq, work, delay))
		sc_put(sc);
}
static void pmnet_sc_cancel_delayed_work(struct pmnet_sock_container *sc,
					 struct delayed_work *work)
{
	if (cancel_delayed_work(work))
		sc_put(sc);
}

/* ------------------------------------------------------------ */

static int pmnet_set_usertimeout(struct socket *sock)
{
	int user_timeout = PMNET_TCP_USER_TIMEOUT;

	return kernel_setsockopt(sock, SOL_TCP, TCP_USER_TIMEOUT,
			(char *)&user_timeout, sizeof(user_timeout));
}

static int pmnet_set_nodelay(struct socket *sock)
{
	int ret, val = 1;
	mm_segment_t oldfs;

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	/*
	 * Dear unsuspecting programmer,
	 *
	 * Don't use sock_setsockopt() for SOL_TCP.  It doesn't check its level
	 * argument and assumes SOL_SOCKET so, say, your TCP_NODELAY will
	 * silently turn into SO_DEBUG.
	 *
	 * Yours,
	 * Keeper of hilariously fragile interfaces.
	 */
	ret = sock->ops->setsockopt(sock, SOL_TCP, TCP_NODELAY,
			(char __user *)&val, sizeof(val));

	set_fs(oldfs);
	return ret;
}


#if 0
static void pmnet_initialize_handshake(void)
{
	pmnet_hand->pmhb_heartbeat_timeout_ms = cpu_to_be32(
			PMHB_MAX_WRITE_TIMEOUT_MS);
	pmnet_hand->pmnet_idle_timeout_ms = cpu_to_be32(pmnet_idle_timeout());
	pmnet_hand->pmnet_keepalive_delay_ms = cpu_to_be32(
			pmnet_keepalive_delay());
	pmnet_hand->pmnet_reconnect_delay_ms = cpu_to_be32(
			pmnet_reconnect_delay());
}
#endif

/*
 * we register callbacks so we can queue work on events before calling
 * the original callbacks.  our callbacks our careful to test user_data
 * to discover when they've reaced with pmnet_unregister_callbacks().
 */
static void pmnet_register_callbacks(struct sock *sk,
		struct pmnet_sock_container *sc)
{
	write_lock_bh(&sk->sk_callback_lock);

#if 0
	/* accepted sockets inherit the old listen socket data ready */
	if (sk->sk_data_ready == pmnet_listen_data_ready) {
		sk->sk_data_ready = sk->sk_user_data;
		sk->sk_user_data = NULL;
	}
#endif

	BUG_ON(sk->sk_user_data != NULL);
	sk->sk_user_data = sc;
	sc_get(sc);

	sc->sc_data_ready = sk->sk_data_ready;
	sc->sc_state_change = sk->sk_state_change;
#if 0
	sk->sk_data_ready = pmnet_data_ready;
	sk->sk_state_change = pmnet_state_change;
#endif

	mutex_init(&sc->sc_send_lock);

	write_unlock_bh(&sk->sk_callback_lock);
}


#if 0
static void pmnet_ensure_shutdown(struct pmnet_node *nn,
		struct pmnet_sock_container *sc,
		int err)
{
	spin_lock(&nn->nn_lock);
	if (nn->nn_sc == sc)
		pmnet_set_nn_state(nn, NULL, 0, err);
	spin_unlock(&nn->nn_lock);
}
#endif

static void pmnet_init_msg(struct pmnet_msg *msg, u16 data_len, u16 msg_type, u32 key)
{
	memset(msg, 0, sizeof(struct pmnet_msg));
	msg->magic = cpu_to_be16(PMNET_MSG_MAGIC);
	msg->data_len = cpu_to_be16(data_len);
	msg->msg_type = cpu_to_be16(msg_type);
	msg->sys_status = cpu_to_be32(PMNET_ERR_NONE);
	msg->status = 0;
	msg->key = cpu_to_be32(key);
}


static int pmnet_tx_can_proceed(struct pmnet_node *nn,
		struct pmnet_sock_container **sc_ret,
		int *error)
{
	int ret = 0;

	spin_lock(&nn->nn_lock);
	if (nn->nn_persistent_error) {
		ret = 1;
		*sc_ret = NULL;
		*error = nn->nn_persistent_error;
	} else if (nn->nn_sc_valid) {
		kref_get(&nn->nn_sc->sc_kref);

		ret = 1;
		*sc_ret = nn->nn_sc;
		*error = 0;
	}
	spin_unlock(&nn->nn_lock);

	return ret;
}

/* ------------------------------------------------------------ */

static int pmnet_recv_tcp_msg(struct socket *sock, void *data, size_t len)
{
	struct kvec vec = { .iov_len = len, .iov_base = data, };
	struct msghdr msg = { .msg_flags = MSG_DONTWAIT, };
	return kernel_recvmsg(sock, &msg, &vec, 1, len, msg.msg_flags);
}

static int pmnet_send_tcp_msg(struct socket *sock, struct kvec *vec,
		size_t veclen, size_t total)
{
	int ret;
	struct msghdr msg = {.msg_flags = 0,};

	if (sock == NULL) {
		ret = -EINVAL;
		goto out;
	}

	ret = kernel_sendmsg(sock, &msg, vec, veclen, total);
	if (likely(ret == total))
		return 0;
	pr_info("sendmsg returned %d instead of %zu\n", ret, total);
	if (ret >= 0)
		ret = -EPIPE; /* should be smarter, I bet */
out:
	pr_info("returning error: %d\n", ret);
	return ret;
}

int pmnet_send_message_vec(u32 msg_type, u32 key, struct kvec *caller_vec,
		size_t caller_veclen, u8 target_node, int *status)
{
	int ret = 0;
	struct pmnet_msg *msg = NULL;
	size_t veclen, caller_bytes = 0;
	struct kvec *vec = NULL;
	struct pmnet_sock_container *sc = NULL;
	struct pmnet_node *nn = pmnet_nn_from_num(target_node);
	struct pmnet_status_wait nsw = {
		.ns_node_item = LIST_HEAD_INIT(nsw.ns_node_item),
	};

	if (pmnet_wq == NULL) {
		pr_info("attempt to tx without o2netd running\n");
		ret = -ESRCH;
		goto out;
	}

	if (caller_veclen == 0) {
		ret = -EINVAL;
		goto out;
	}

	caller_bytes = iov_length((struct iovec *)caller_vec, caller_veclen);
	if (caller_bytes > PMNET_MAX_PAYLOAD_BYTES) {
		ret = -EINVAL;
		goto out;
	}

#if 0
	wait_event(nn->nn_sc_wq, pmnet_tx_can_proceed(nn, &sc, &ret));
	if (ret)
		goto out;
#endif

	sc = nn->nn_sc;

	veclen = caller_veclen + 1;
	vec = kmalloc(sizeof(struct kvec) * veclen, GFP_ATOMIC);
	if (vec == NULL) {
		pr_info("failed to %zu element kvec!\n", veclen);
		ret = -ENOMEM;
		goto out;
	}

	msg = kmalloc(sizeof(struct pmnet_msg), GFP_ATOMIC);
	if (!msg) {
		pr_info("failed to allocate a pmnet_msg!\n");
		ret = -ENOMEM;
		goto out;
	}

	pmnet_init_msg(msg, caller_bytes, msg_type, key);

	vec[0].iov_len = sizeof(struct pmnet_msg);
	vec[0].iov_base = msg;
	memcpy(&vec[1], caller_vec, caller_veclen * sizeof(struct kvec));

#if 0
	ret = pmnet_prep_nsw(nn, &nsw);
	if (ret)
		goto out;

	msg->msg_num = cpu_to_be32(nsw.ns_id);
	pmnet_set_nst_msg_id(&nst, nsw.ns_id);

	pmnet_set_nst_send_time(&nst);
#endif

	/* finally, convert the message header to network byte-order
	 * 	 * and send */
	mutex_lock(&sc->sc_send_lock);
	ret = pmnet_send_tcp_msg(sc->sc_sock, vec, veclen,
			sizeof(struct pmnet_msg) + caller_bytes);
	mutex_unlock(&sc->sc_send_lock);
	if (ret < 0) {
		pr_info("error returned from pmnet_send_tcp_msg=%d\n", ret);
		goto out;
	}

#if 0
	/* wait on other node's handler */
	pmnet_set_nst_status_time(&nst);
	wait_event(nsw.ns_wq, pmnet_nsw_completed(nn, &nsw));

	pmnet_update_send_stats(&nst, sc);


	/* Note that we avoid overwriting the callers status return
	 * 	 * variable if a system error was reported on the other
	 * 	 	 * side. Callers beware. */
	ret = pmnet_sys_err_to_errno(nsw.ns_sys_status);
	if (status && !ret)
		*status = nsw.ns_status;

	mlog(0, "woken, returning system status %d, user status %d\n",
			ret, nsw.ns_status);
#endif

out:
//	pmnet_debug_del_nst(&nst); /* must be before dropping sc and node */
	if (sc)
		sc_put(sc);
	kfree(vec);
	kfree(msg);
//	pmnet_complete_nsw(nn, &nsw, 0, 0, 0);
	return ret;
}
EXPORT_SYMBOL_GPL(pmnet_send_message_vec);

int pmnet_send_message(u32 msg_type, u32 key, void *data, u32 len,
		u8 target_node, int *status)
{
	struct kvec vec = {
		.iov_base = data,
		.iov_len = len,
	};
	return pmnet_send_message_vec(msg_type, key, &vec, 1,
			target_node, status);
}
EXPORT_SYMBOL_GPL(pmnet_send_message);


/* 
 * this work func is kicked whenever a path sets the nn state which doesn't
 * have valid set.  This includes seeing hb come up, losing a connection,
 * having a connect attempt fail, etc. This centralizes the logic which decides
 * if a connect attempt should be made or if we should give up and all future
 * transmit attempts should fail 
 */
static void pmnet_start_connect(struct work_struct *work)
{
	struct pmnet_node *nn = 
		container_of(work, struct pmnet_node, nn_connect_work.work);
	struct pmnet_sock_container *sc = NULL;
	struct socket *sock = NULL;
	struct sockaddr_in myaddr = {0, }, remoteaddr = {0, };
	int ret = 0, stop;
	unsigned int timeout;

	char response[4097];
	char reply[4097];

	int status;

	pr_info("pmnet_start_connect::start");

	/*
	 * sock_create allocates the sock with GFP_KERNEL. We must set
	 * per-process flag PF_MEMALLOC_NOIO so that all allocations done
	 * by this process are done as if GFP_NOIO was specified. So we
	 * are not reentering filesystem while doing memory reclaim.
	 */

	sc = sc_alloc();
	if (sc == NULL) {
		pr_info("couldn't allocate sc\n");
		ret = -ENOMEM;
		goto out;
	}

	ret = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
	if (ret < 0) {
		pr_info("can't create socket: %d\n", ret);
		goto out;
	}
	sc->sc_sock = sock; /* freed by sc_kref_release */

	sock->sk->sk_allocation = GFP_ATOMIC;

	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = inet_addr (DEST_ADDR);
	myaddr.sin_port = htons(PORT); /* any port */

	ret = sock->ops->bind(sock, (struct sockaddr *)&myaddr,
			sizeof(myaddr));
	if (ret) {
		goto out;
	}

	ret = pmnet_set_nodelay(sc->sc_sock);
	if (ret) {
		pr_info("setting TCP_NODELAY failed with %d\n", ret);
		goto out;
	}

	ret = pmnet_set_usertimeout(sock);
	if (ret) {
		pr_info("set TCP_USER_TIMEOUT failed with %d\n", ret);
		goto out;
	}

	pmnet_register_callbacks(sc->sc_sock->sk, sc);

	remoteaddr.sin_family = AF_INET;
	remoteaddr.sin_addr.s_addr = inet_addr (DEST_ADDR);
	remoteaddr.sin_port = htons(PORT);

	ret = sc->sc_sock->ops->connect(sc->sc_sock,
			(struct sockaddr *)&remoteaddr,
			sizeof(remoteaddr),
			O_NONBLOCK);
	if (ret == -EINPROGRESS)
		ret = 0;


	memset(&reply, 0, 4097);
	strcat(reply, "HOLA"); 


	pmnet_send_message(0, 0, &reply, sizeof(reply),
		0, &status);

out:
#if 0
	if (ret && sc) {
		printk(KERN_NOTICE "pmnet: Connect attempt to " SC_NODEF_FMT
				" failed with errno %d\n", SC_NODEF_ARGS(sc), ret);
		/* 0 err so that another will be queued and attempted
		 * from set_nn_state 
		 */
//		pmnet_ensure_shutdown(nn, sc, 0);
	}
#endif
	if (sc)
		sc_put(sc);

	return;
}

static void pmnet_connect_expired(struct work_struct *work)
{
	struct pmnet_node *nn =
		container_of(work, struct pmnet_node, nn_connect_expired.work);

	spin_lock(&nn->nn_lock);
#if 0
	if (!nn->nn_sc_valid) {
		printk(KERN_NOTICE "pmnet: No connection established with "
				"node %u after %u.%u seconds, check network and"
				" cluster configuration.\n",
				pmnet_num_from_nn(nn),
				pmnet_idle_timeout() / 1000,
				pmnet_idle_timeout() % 1000);
		pmnet_set_nn_state(nn, NULL, 0, 0);
	}
#endif
	spin_unlock(&nn->nn_lock);
}

static void pmnet_still_up(struct work_struct *work)
{
	struct pmnet_node *nn =
		container_of(work, struct pmnet_node, nn_still_up.work);

//	o2quo_hb_still_up(pmnet_num_from_nn(nn));
}

/* ------------------------------------------------------------ */

void pmnet_disconnect_node(struct o2nm_node *node)
{
	struct pmnet_node *nn = pmnet_nn_from_num(0);

	/* don't reconnect until it's heartbeating again */
	spin_lock(&nn->nn_lock);
	atomic_set(&nn->nn_timeout, 0);
//	pmnet_set_nn_state(nn, NULL, 0, -ENOTCONN);
	spin_unlock(&nn->nn_lock);

	if (pmnet_wq) {
		cancel_delayed_work(&nn->nn_connect_expired);
		cancel_delayed_work(&nn->nn_connect_work);
		cancel_delayed_work(&nn->nn_still_up);
		flush_workqueue(pmnet_wq);
	}
}


/* ----------------------------------------------------------------- */

#if 0
static int pmnet_open_listening_sock(__be32 addr, __be16 port)
{
	struct socket *sock = NULL;
	int ret;
	struct sockaddr_in sin = {
		.sin_family = PF_INET,
		.sin_addr = { .s_addr = addr },
		.sin_port = port,
	};

	ret = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
	if (ret < 0) {
		printk(KERN_ERR "pmnet: Error %d while creating socket\n", ret);
		goto out;
	}

	sock->sk->sk_allocation = GFP_ATOMIC;

	write_lock_bh(&sock->sk->sk_callback_lock);
	sock->sk->sk_user_data = sock->sk->sk_data_ready;
	sock->sk->sk_data_ready = pmnet_listen_data_ready;
	write_unlock_bh(&sock->sk->sk_callback_lock);

	pmnet_listen_sock = sock;
	INIT_WORK(&pmnet_listen_work, pmnet_accept_many);

	sock->sk->sk_reuse = SK_CAN_REUSE;
	ret = sock->ops->bind(sock, (struct sockaddr *)&sin, sizeof(sin));
	if (ret < 0) {
		printk(KERN_ERR "pmnet: Error %d while binding socket at "
				"%pI4:%u\n", ret, &addr, ntohs(port)); 
		goto out;
	}

	ret = sock->ops->listen(sock, 64);
	if (ret < 0)
		printk(KERN_ERR "pmnet: Error %d while listening on %pI4:%u\n",
				ret, &addr, ntohs(port));

out:
	if (ret) {
		pmnet_listen_sock = NULL;
		if (sock)
			sock_release(sock);
	}
	return ret;
}


/*
 * called from node manager when we should bring up our network listening
 * socket.  node manager handles all the serialization to only call this
 * once and to match it with pmnet_stop_listening().  note,
 * o2nm_this_node() doesn't work yet as we're being called while it
 * is being set up.
 */
int pmnet_start_listening(struct o2nm_node *node)
{
	int ret = 0;

	BUG_ON(pmnet_wq != NULL);
	BUG_ON(pmnet_listen_sock != NULL);

	mlog(ML_KTHREAD, "starting pmnet thread...\n");
	pmnet_wq = create_singlethread_workqueue("pmnet");
	if (pmnet_wq == NULL) {
		mlog(ML_ERROR, "unable to launch pmnet thread\n");
		return -ENOMEM; /* ? */
	}

	ret = pmnet_open_listening_sock(node->nd_ipv4_address,
			node->nd_ipv4_port);
	if (ret) {
		destroy_workqueue(pmnet_wq);
		pmnet_wq = NULL;
	} else
		o2quo_conn_up(node->nd_num);

	return ret;
}

void pmnet_stop_listening()
{
	struct socket *sock = pmnet_listen_sock;
	size_t i;

	BUG_ON(pmnet_wq == NULL);
	BUG_ON(pmnet_listen_sock == NULL);

	/* stop the listening socket from generating work */
	write_lock_bh(&sock->sk->sk_callback_lock);
	sock->sk->sk_data_ready = sock->sk->sk_user_data;
	sock->sk->sk_user_data = NULL;
	write_unlock_bh(&sock->sk->sk_callback_lock);

	for (i = 0; i < ARRAY_SIZE(pmnet_nodes); i++) {
		struct o2nm_node *node = o2nm_get_node_by_num(i);
		if (node) {
			pmnet_disconnect_node(node);
			o2nm_node_put(node);
		}
	}

	/* finish all work and tear down the work queue */
	mlog(ML_KTHREAD, "waiting for pmnet thread to exit....\n");
	destroy_workqueue(pmnet_wq);
	pmnet_wq = NULL;

	sock_release(pmnet_listen_sock);
	pmnet_listen_sock = NULL;

	o2quo_conn_err(node->nd_num);
}
#endif

int pmnet_init(void)
{
	unsigned long i;

	pmnet_hand = kzalloc(sizeof(struct pmnet_handshake), GFP_KERNEL);
	pmnet_keep_req = kzalloc(sizeof(struct pmnet_msg), GFP_KERNEL);
	pmnet_keep_resp = kzalloc(sizeof(struct pmnet_msg), GFP_KERNEL);
	if (!pmnet_hand || !pmnet_keep_req || !pmnet_keep_resp)
		goto out;

	pmnet_hand->connector_id = cpu_to_be64(1);

	pmnet_keep_req->magic = cpu_to_be16(PMNET_MSG_KEEP_REQ_MAGIC);
	pmnet_keep_resp->magic = cpu_to_be16(PMNET_MSG_KEEP_RESP_MAGIC);

	pmnet_wq = alloc_ordered_workqueue("pmnet", WQ_MEM_RECLAIM);

	for (i = 0; i < ARRAY_SIZE(pmnet_nodes); i++) {
		struct pmnet_node *nn = pmnet_nn_from_num(i);
		
		pr_info("pmnet_init::set pmnet_node\n");
		atomic_set(&nn->nn_timeout, 0);
		spin_lock_init(&nn->nn_lock);

		INIT_DELAYED_WORK(&nn->nn_connect_work, pmnet_start_connect);
		queue_delayed_work(pmnet_wq, &nn->nn_connect_work, 0);

		INIT_DELAYED_WORK(&nn->nn_connect_expired,
				pmnet_connect_expired);

		INIT_DELAYED_WORK(&nn->nn_still_up, pmnet_still_up);
		/* until we see hb from a node we'll return einval */
		nn->nn_persistent_error = -ENOTCONN;
		init_waitqueue_head(&nn->nn_sc_wq);
		idr_init(&nn->nn_status_idr);
		INIT_LIST_HEAD(&nn->nn_status_list);
	}

	return 0;

out:
	kfree(pmnet_hand);
	kfree(pmnet_keep_req);
	kfree(pmnet_keep_resp);
	return -ENOMEM;
}

void pmnet_exit(void)
{
	kfree(pmnet_hand);
	kfree(pmnet_keep_req);
	kfree(pmnet_keep_resp);
}
