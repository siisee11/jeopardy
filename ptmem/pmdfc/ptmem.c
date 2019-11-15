/*
 * Xen implementation for transcendent memory (tmem)
 *
 * Copyright (C) 2009-2011 Oracle Corp.  All rights reserved.
 * Author: Dan Magenheimer
 */

#define pr_fmt(fmt) "ptmem:" KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/pagemap.h>
#include <linux/cleancache.h>
#include <linux/frontswap.h>


bool __read_mostly tmem_enabled = false;

static int __init enable_tmem(char *s)
{
	tmem_enabled = true;
	return 1;
}
__setup("tmem", enable_tmem);

static bool cleancache __read_mostly = true;
module_param(cleancache, bool, S_IRUGO);

#define TMEM_CONTROL               0
#define TMEM_NEW_POOL              1
#define TMEM_DESTROY_POOL          2
#define TMEM_NEW_PAGE              3
#define TMEM_PUT_PAGE              4
#define TMEM_GET_PAGE              5
#define TMEM_FLUSH_PAGE            6
#define TMEM_FLUSH_OBJECT          7
#define TMEM_READ                  8
#define TMEM_WRITE                 9
#define TMEM_XCHG                 10

/* Bits for HYPERVISOR_tmem_op(TMEM_NEW_POOL) */
#define TMEM_POOL_PERSIST          1
#define TMEM_POOL_SHARED           2
#define TMEM_POOL_PAGESIZE_SHIFT   4
#define TMEM_VERSION_SHIFT        24

struct tmem_pool_uuid {
	u64 uuid_lo;
	u64 uuid_hi;
};

struct tmem_oid {
	u64 oid[3];
};

#define TMEM_POOL_PRIVATE_UUID	{ 0, 0 }

/* flags for tmem_ops.new_pool */
#define TMEM_POOL_PERSIST          1
#define TMEM_POOL_SHARED           2


/* CleanCache operations */

static void tmem_cleancache_put_page(int pool, struct cleancache_filekey key,
				     pgoff_t index, struct page *page)
{
	u32 ind = (u32) index;
	struct tmem_oid oid = *(struct tmem_oid *)&key;

	printk("tmem_cleancache_put_page called");

	if (pool < 0)
		return;
	if (ind != index)
		return;
	mb(); /* ensure page is quiescent; tmem may address it with an alias */
}

static int tmem_cleancache_get_page(int pool, struct cleancache_filekey key,
				    pgoff_t index, struct page *page)
{
	u32 ind = (u32) index;
	struct tmem_oid oid = *(struct tmem_oid *)&key;
	int ret;

	printk("tmem_cleancache_get_page called");

	/* translate return values to linux semantics */
	if (pool < 0)
		return -1;
	if (ind != index)
		return -1;
	ret = 1;
	if (ret == 1)
		return 0;
	else
		return -1;
}

static void tmem_cleancache_flush_page(int pool, struct cleancache_filekey key,
				       pgoff_t index)
{
	u32 ind = (u32) index;
	struct tmem_oid oid = *(struct tmem_oid *)&key;

	if (pool < 0)
		return;
	if (ind != index)
		return;
}

static void tmem_cleancache_flush_inode(int pool, struct cleancache_filekey key)
{
	struct tmem_oid oid = *(struct tmem_oid *)&key;

	if (pool < 0)
		return;
}

static void tmem_cleancache_flush_fs(int pool)
{
	if (pool < 0)
		return;
}

static int tmem_cleancache_init_fs(size_t pagesize)
{
	struct tmem_pool_uuid uuid_private = TMEM_POOL_PRIVATE_UUID;
	printk("tmem_cleancache_get_page called");
	/* return poolid, for test it is 0 */
	return 0;
}

static int tmem_cleancache_init_shared_fs(uuid_t *uuid, size_t pagesize)
{
	struct tmem_pool_uuid shared_uuid;

	shared_uuid.uuid_lo = *(u64 *)&uuid->b[0];
	shared_uuid.uuid_hi = *(u64 *)&uuid->b[8];
	/* return poolid, for test it is 0 */
	return 0;
}

static const struct cleancache_ops tmem_cleancache_ops = {
	.put_page = tmem_cleancache_put_page,
	.get_page = tmem_cleancache_get_page,
	.invalidate_page = tmem_cleancache_flush_page,
	.invalidate_inode = tmem_cleancache_flush_inode,
	.invalidate_fs = tmem_cleancache_flush_fs,
	.init_shared_fs = tmem_cleancache_init_shared_fs,
	.init_fs = tmem_cleancache_init_fs
};


static int __init ptmem_init(void)
{
	BUILD_BUG_ON(sizeof(struct cleancache_filekey) != sizeof(struct tmem_oid));
	if (tmem_enabled && cleancache) {
		int err;

		err = cleancache_register_ops(&tmem_cleancache_ops);
		if (err)
			pr_warn("ptmem: failed to enable cleancache: %d\n",
				err);
		else
			pr_info("cleancache enabled, RAM provided by "
				"Persistent Transcendent Memory\n");
	}

	return 0;
}

module_init(ptmem_init)
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nam Jaeyoun <siisee111@gmail.com>");
MODULE_DESCRIPTION("Shim to persistent transcendent memory");
