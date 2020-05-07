#include <linux/cleancache.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/uuid.h>

#include "tmem.h"
#include "tcp.h"
#include "bloom_filter.h"

/* Allocation flags */
#define PMDFC_GFP_MASK  (GFP_ATOMIC | __GFP_NORETRY | __GFP_NOWARN)

/* Initial page pool: 32 MB (2^13 * 4KB) in pages */
#define PMDFC_ORDER 13

/* bloom filter */
struct bloom_filter *bf;

/* The pool holding the compressed pages */
struct page* page_pool;

/* Currently handled oid */
struct tmem_oid coid = {.oid[0]=-1UL, .oid[1]=-1UL, .oid[2]=-1UL};

/* Global count */
atomic_t v = ATOMIC_INIT(0);


/*  Clean cache operations implementation */
static void pmdfc_cleancache_put_page(int pool_id,
		struct cleancache_filekey key,
		pgoff_t index, struct page *page)
{
	struct tmem_oid oid = *(struct tmem_oid *)&key;
	void *pg_from;
	void *pg_to;
	u8 *data = (u8 *)&key;

	int len = 4096;
	char response[10];
	char reply[10];
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
		memset(&reply, 0, 10);
		strcat(reply, "PUTPAGE"); 

#if 0
		/* add to bloom filter */
		ret = bloom_filter_add(bf, data);
		if ( ret < 0 )
			pr_info("bloom_filter add fail\n");
#endif

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
	u8 *data = (u8 *)&key;

	char response[4097];
	char reply[4097];

	bool isIn = false;

//	printk(KERN_INFO "pmdfc: GET PAGE pool_id=%d key=%llu,%llu,%llu index=%ld page=%p\n", pool_id, 
//			(long long)oid.oid[0], (long long)oid.oid[1], (long long)oid.oid[2], index, page);
	
//	bloom_filter_check(bf, data, 0, &isIn);

	if ( !isIn )
		goto out;

	printk(KERN_INFO "pmdfc: GET PAGE pool_id=%d key=%llu,%llu,%llu index=%ld page=%p\n", pool_id, 
			(long long)oid.oid[0], (long long)oid.oid[1], (long long)oid.oid[2], index, page);

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

out:
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

static int bloom_filter_init(void)
{
	unsigned char *data = "hihihellolowl";
	bool isIn;
	int ret = 0;

	bf = bloom_filter_new(1000);
	bloom_filter_add_hash_alg(bf, "md5");
	bloom_filter_add_hash_alg(bf, "sha1");
	bloom_filter_add_hash_alg(bf, "sha256");

	ret = bloom_filter_add(bf, data, 4);
	if ( ret < 0 )
		pr_info("bloom_filter add fail\n");

	bloom_filter_check(bf, data, 4, &isIn);

	pr_info("isIn=%d \n", isIn);

	return 0;
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

	/* init bloom filter */
	bloom_filter_init();
	pr_info(" *** bloom filter | init | bloom_filter_init *** \n");
	

	return 0;
}

static void pmdfc_exit(void)
{
	bloom_filter_unref(bf);
	pmnet_exit();

	//	__free_pages(page_pool, PMDFC_ORDER);
	__free_page(page_pool);
}

module_init(pmdfc_init);
module_exit(pmdfc_exit);

MODULE_LICENSE("GPL");
