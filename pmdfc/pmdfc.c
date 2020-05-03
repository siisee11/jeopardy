#include <linux/cleancache.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/uuid.h>

#include "tmem.h"
#include "tcp.h"

/* socket for networking */
//struct socket *conn_socket = NULL;

/* Allocation flags */
#define PMDFC_GFP_MASK  (GFP_ATOMIC | __GFP_NORETRY | __GFP_NOWARN)

/* Initial page pool: 32 MB (2^13 * 4KB) in pages */
#define PMDFC_ORDER 13

/* The pool holding the compressed pages */
struct page* page_pool;

/* Currently handled oid */
struct tmem_oid coid = {.oid[0]=-1UL, .oid[1]=-1UL, .oid[2]=-1UL};

/* Global count */
atomic_t v = ATOMIC_INIT(0);


#if 0
int tcp_client_connect(void)
{
	struct sockaddr_in saddr;
	unsigned char destip[5] = {115,145,173,69,'\0'};
	int len = 4096;
	char response[len+1];
	char reply[len+1];
	int ret = -1;

	DECLARE_WAIT_QUEUE_HEAD(recv_wait);

	ret = sock_create(PF_INET, SOCK_STREAM, IPPROTO_TCP, &conn_socket);
	if(ret < 0)
	{
		pr_info(" *** mtp | Error: %d while creating first socket. | "
				"setup_connection *** \n", ret);
		goto err;
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(PORT);
	saddr.sin_addr.s_addr = htonl(create_address(destip));

	ret = conn_socket->ops->connect(conn_socket, (struct sockaddr *)&saddr\
			, sizeof(saddr), O_RDWR);
	if(ret && (ret != -EINPROGRESS))
	{
		pr_info(" *** mtp | Error: %d while connecting using conn "
				"socket. | setup_connection *** \n", ret);
		goto err;
	}

	memset(&reply, 0, len+1);
	strcat(reply, "HOLA"); 
	tcp_client_send(conn_socket, reply, strlen(reply), MSG_DONTWAIT);

	wait_event_timeout(recv_wait,\
			!skb_queue_empty(&conn_socket->sk->sk_receive_queue),\
			5*HZ);
	if(!skb_queue_empty(&conn_socket->sk->sk_receive_queue))
	{
		memset(&response, 0, len+1);
		tcp_client_receive(conn_socket, response, MSG_DONTWAIT);
	}

err:
	return -1;
}
#endif




/*  Clean cache operations implementation */
static void pmdfc_cleancache_put_page(int pool_id,
		struct cleancache_filekey key,
		pgoff_t index, struct page *page)
{
	struct tmem_oid oid = *(struct tmem_oid *)&key;
	void *pg_from;
	void *pg_to;

	int len = 4096;
	char response[len+1];
	char reply[len+1];
	int ret = -1;

	if (!tmem_oid_valid(&coid)) {
		printk(KERN_INFO "pmdfc: PUT PAGE pool_id=%d key=%llu,%llu,%llu index=%ld page=%p\n", pool_id, 
				(long long)oid.oid[0], (long long)oid.oid[1], (long long)oid.oid[2], index, page);
		printk(KERN_INFO "pmdfc: PUT PAGE success\n");
		coid = oid;
		tmem_oid_print(&coid);

		pg_from = kmap_atomic(page);
		pg_to = kmap_atomic(page_pool);
		memcpy(pg_to, pg_from, sizeof(struct page));

		/* Send page to server */
		memset(&reply, 0, len+1);
		strcat(reply, "PUTPAGE"); 

		/* unmap kmapped space */
		kunmap_atomic(pg_from);
		kunmap_atomic(pg_to);
	}
}

static int pmdfc_cleancache_get_page(int pool_id,
		struct cleancache_filekey key,
		pgoff_t index, struct page *page)
{
	u32 ind = (u32) index;
	struct tmem_oid oid = *(struct tmem_oid *)&key;
	char *from_va;
	char *to_va;

	char response[4097];
	char reply[4097];

//	printk(KERN_INFO "pmdfc: GET PAGE pool_id=%d key=%llu,%llu,%llu index=%ld page=%p\n", pool_id, 
//			(long long)oid.oid[0], (long long)oid.oid[1], (long long)oid.oid[2], index, page);


	if ( tmem_oid_compare(&coid, &oid) == 0 && atomic_read(&v) == 0 ) {
		atomic_inc(&v);
		printk(KERN_INFO "pmdfc: GET PAGE start\n");
		to_va = kmap_atomic(page);
		from_va = kmap_atomic(page_pool);

		memcpy(to_va, from_va, sizeof(struct page));

		kunmap_atomic(to_va);
		kunmap_atomic(from_va);


		/* Send page to server */
		memset(&reply, 0, 4097);
		strcat(reply, "GETPAGE"); 

		printk(KERN_INFO "pmdfc: GET PAGE success\n");

		return 0;
	}

	return -1;
}

static void pmdfc_cleancache_flush_page(int pool_id,
		struct cleancache_filekey key,
		pgoff_t index)
{
	//	printk(KERN_INFO "pmdfc: FLUSH PAGE: pool_id: %d\n", pool_id);
}

static void pmdfc_cleancache_flush_inode(int pool_id,
		struct cleancache_filekey key)
{
	//	printk(KERN_INFO "pmdfc: FLUSH INODE: pool_id: %d\n", pool_id);
}

static void pmdfc_cleancache_flush_fs(int pool_id)
{
	printk(KERN_INFO "pmdfc: FLUSH FS\n");
}

static int pmdfc_cleancache_init_fs(size_t pagesize)
{
	static atomic_t pool_id = ATOMIC_INIT(0);

	printk(KERN_INFO "pmdfc: INIT FS\n");
	atomic_inc(&pool_id);

	return atomic_read(&pool_id);
}

static int pmdfc_cleancache_init_shared_fs(uuid_t *uuid, size_t pagesize)
{
	printk(KERN_INFO "pmdfc: FLUSH INIT SHARED\n");
	return -1;
}

static const struct cleancache_ops pmdfc_cleancache_ops = {
	.put_page = pmdfc_cleancache_put_page,
	.get_page = pmdfc_cleancache_get_page,
	.invalidate_page = pmdfc_cleancache_flush_page,
	.invalidate_inode = pmdfc_cleancache_flush_inode,
	.invalidate_fs = pmdfc_cleancache_flush_fs,
	.init_shared_fs = pmdfc_cleancache_init_shared_fs,
	.init_fs = pmdfc_cleancache_init_fs
};

static int pmdfc_cleancache_register_ops(void)
{
	int ret;

	ret = cleancache_register_ops(&pmdfc_cleancache_ops);

	return ret;
}

static int __init pmdfc_init(void)
{
	int ret;

	printk(KERN_INFO ">> pmdfc INIT\n");
	//	page_pool = alloc_pages(PMDFC_GFP_MASK, PMDFC_ORDER);
	page_pool = alloc_page(PMDFC_GFP_MASK);

	ret = pmdfc_cleancache_register_ops();

	if (!ret) {
		printk(KERN_INFO ">> pmdfc: cleancache_register_ops success\n");
	} else {
		printk(KERN_INFO ">> pmdfc: cleancache_register_ops fail\n");
	}


	if (cleancache_enabled) {
		printk(KERN_INFO ">> pmdfc: cleancache_enabled\n");
	}
	else {
		printk(KERN_INFO ">> pmdfc: cleancache_disabled\n");
	}

	pmnet_init();
	pr_info(" *** mtp | network client init | network_client_init *** \n");

	return 0;
}

static void pmdfc_exit(void)
{
	pmnet_exit();

	//	__free_pages(page_pool, PMDFC_ORDER);
	__free_page(page_pool);
}

module_init(pmdfc_init);
module_exit(pmdfc_exit);

MODULE_LICENSE("GPL");
