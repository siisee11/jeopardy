#include <linux/cleancache.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/uuid.h>

#include "tmem.h"

/* Allocation flags */
#define pmdfc_GFP_MASK  (GFP_ATOMIC | __GFP_NORETRY | __GFP_NOWARN)

/* Initial page pool: 32 MB (2^13 * 4KB) in pages */
#define pmdfc_ORDER 13

/* The pool holding the compressed pages */
struct page* page_pool;

/* Currently handled oid */
struct tmem_oid coid = {.oid[0]=-1UL, .oid[1]=-1UL, .oid[2]=-1UL};

/*  Clean cache operations implementation */
static void pmdfc_cleancache_put_page(int pool_id,
					struct cleancache_filekey key,
					pgoff_t index, struct page *page)
{
	struct tmem_oid oid = *(struct tmem_oid *)&key;

	printk(KERN_INFO "pmdfc: PUT PAGE pool_id=%d key=%u index=%ld page=%p\n", pool_id, 
			*key.u.key, index, page);

	if (tmem_oid_valid(&coid)) {
		printk(KERN_INFO "pmdfc: PUT PAGE success\n");
		coid = oid;
		page_pool = page;
	}
}

static int pmdfc_cleancache_get_page(int pool_id,
					struct cleancache_filekey key,
					pgoff_t index, struct page *page)
{
	u32 ind = (u32) index;
	struct tmem_oid oid = *(struct tmem_oid *)&key;
	
	printk(KERN_INFO "pmdfc: GET PAGE pool_id=%d key=%llu,%llu,%llu index=%ld page=%p\n", pool_id, 
			(long long)oid.oid[0], (long long)oid.oid[1], (long long)oid.oid[2], index, page);

	if ( tmem_oid_compare(&coid, &oid) == 0) {
		page = page_pool;
		printk(KERN_INFO "pmdfc: GET PAGE success\n");
		return 1;
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
	page_pool = alloc_pages(pmdfc_GFP_MASK, pmdfc_ORDER);

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
	
	return 0;
}

static void pmdfc_exit(void)
{
	__free_pages(page_pool, pmdfc_ORDER);
}

module_init(pmdfc_init);
module_exit(pmdfc_exit);

MODULE_LICENSE("GPL");
