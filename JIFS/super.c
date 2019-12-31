// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019 Jaeyoun Nam (siisee111@gmail.com)
 *
 */


#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/dcache.h>
#include <linux/dax.h>

#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/parser.h>
#include <linux/vfs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/seq_file.h>
#include <linux/mount.h>
#include <linux/mm.h>
#include <linux/ctype.h>
#include <linux/bitops.h>
#include <linux/magic.h>
#include <linux/exportfs.h>
#include <linux/random.h>
#include <linux/cred.h>
#include <linux/backing-dev.h>
#include <linux/list.h>

#include "jifs.h"


static DEFINE_MUTEX(ji_mutex);

static const struct inode_operations jifs_inode_ops;
static const struct super_operations jifs_sops;
static const struct export_operations jifs_export_ops;

unsigned int jifs_dbgmask = 0;


static loff_t jifs_max_size(int bits)
{
	loff_t res;

	res = (1ULL << 63) - 1;

	if (res > MAX_LFS_FILESIZE)
		res = MAX_LFS_FILESIZE;

	jifs_dbg("max file size %llu bytes\n",  res);
	return res;
}


static phys_addr_t get_phys_addr(void **data)
{
	phys_addr_t phys_addr;
	char *options = (char *)*data;

	if (!options || strncmp(options, "physaddr=", 9) != 0)
		return (phys_addr_t)ULLONG_MAX;
	options += 9;
	phys_addr = (phys_addr_t)simple_strtoull(options, &options, 0);
	if (*options && *options != ',') {
		printk(KERN_ERR "Invalid phys addr specification: %s\n",
				(char *)*data);
		return (phys_addr_t)ULLONG_MAX;
	}
	if (phys_addr & (PAGE_SIZE - 1)) {
		printk(KERN_ERR "physical address 0x%16llx for jifs isn't "
				"aligned to a page boundary\n", (u64)phys_addr);
		return (phys_addr_t)ULLONG_MAX;
	}
	if (*options == ',')
		options++;
	*data = (void *)options;
	return phys_addr;
}

/*
 * file이나 directory를 만들 때, 내부적으로 그 것에 해당하는 inode를 정의해야된다.
 * mode 변수가 이것이 directory인지 file인지 permission에 대한 정보를 준다.
 * 이 변수를 통해 directory와 file에 맞추어 설정가능하다.
 */

static struct inode *jifs_make_inode(struct super_block *sb, int mode,
		const struct file_operations *fops)
{
	struct inode* inode;
	inode = new_inode(sb);
	if (unlikely(!inode))
		return NULL;

	inode->i_mode = mode;
	inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
	inode->i_fop = fops;
	inode->i_ino = get_next_ino();
	return inode;
}



/*
 * Create a file mapping a name to a counter.
 */

static const struct inode_operations jifs_inode_ops = {
	.setattr			= simple_setattr,
	.getattr			= simple_getattr,
};

static struct dentry *jifs_create_file (struct super_block *sb,
		struct dentry *dir, const char *name,
		atomic_t *counter)
{
	struct dentry *dentry;
	struct inode *inode;

	jifs_dbg_verbose("%s: file %s create....\n", __func__, name);

	/* dentry 만듬 */
	dentry = d_alloc_name(dir, name);
	if (unlikely(! dentry))
		goto out;
	inode = jifs_make_inode(sb, S_IFREG | 0644, &jifs_file_ops);
	if (unlikely(! inode))
		goto out_dput;
	inode->i_private = counter;

	/* put is all into the dentry cache */
	d_add(dentry, inode);
	return dentry;

out_dput:
	dput(dentry);
out:
	return 0;
}

/*
 * directory 만드는 것은 jifs_create_file과 유사.
 * mode가 달라지는 것을 확인.
 */
static struct dentry *jifs_create_dir (struct super_block *sb,
		struct dentry *parent, const char *name)
{
	struct dentry *dentry;
	struct inode *inode;

	jifs_dbg_verbose("%s: dir %s create....\n", __func__, name);

	/* dentry 만듬 */
	dentry = d_alloc_name(parent, name);
	if (unlikely(! dentry))
		goto out;

	inode = jifs_make_inode(sb, S_IFDIR | 0755, &simple_dir_operations);

	if (unlikely(! inode))
		goto out_dput;
	inode->i_op = &simple_dir_inode_operations;

	/* put is all into the dentry cache */
	d_add(dentry, inode);
	return dentry;

out_dput:
	dput(dentry);
out:
	return 0;
}


/*
 * File들을 만들어 보자.
 */

static atomic_t counter, subcounter;

static void jifs_create_files (struct super_block *sb, struct dentry * root)
{
	struct dentry *subdir;

	jifs_dbg_verbose("%s: files and dir create....\n", __func__);

	/* top level directory에 카운터 파일 하나 추가. */
	atomic_set(&counter, 0);
	jifs_create_file(sb, root, "counter", &counter);
	
	/* subdirectory 를 만들고 counter 파일 하나 추가. */
	atomic_set(&subcounter, 0);
	subdir = jifs_create_dir(sb, root, "subdir");
	if (likely(subdir))
		jifs_create_file(sb, subdir, "subcounter", &subcounter);
}





/* to inode.c */
static struct inode *jifs_iget(struct super_block *sb, ino_t ino)
{
	struct jifs_inode_info *si;
	struct inode *inode;
	int err;

	inode = iget_locked(sb, ino);

	si = JIFS_I(inode);

	jifs_dbg("%s: inode %lu\n", __func__, ino);

/*
	if (ino == JIFS_ROOT_INO) {
		pi_addr = JIFS_ROOT_INO_START;
	} else {
//		err = jifs_get_inode_address(sb, ino, &pi_addr, 0);
		goto fail;
	}
	if (pi_addr == 0) {
		err = -EACCES;
		goto fail;
	}
	*/


	/* TODO: rebuild and read */

	inode->i_ino = ino;

	unlock_new_inode(inode);
	return inode;

fail:
	iget_failed(inode);
	return ERR_PTR(err);
}

static struct kmem_cache *jifs_inode_cachep;


static void jifs_put_super(struct super_block *sb)
{
	/* unmount time */
	struct jifs_sb_info *sbi = JIFS_SB(sb);

	jifs_dbgmask = 0;
	kfree(sbi);
	sb->s_fs_info = NULL;
	jifs_dbg_verbose("jifs super block destroyed\n");
}


/*-------------------------parse option-----------------------------*/

enum {
	Opt_mode,
	Opt_err,
	Opt_dbgmask
};

static const match_table_t tokens = {
	{Opt_mode,			"mode=%o"			},
	{Opt_dbgmask,		"dbgmask=%u"		},
	{Opt_err,			NULL				}
};

static int jifs_parse_options(char *data, struct jifs_mount_opts *opts)
{
	substring_t args[MAX_OPT_ARGS];
	int option;
	int token;
	char *p;

	opts->mode = 0755;

	while ((p = strsep(&data, ",")) != NULL) {
		if (!*p)
			continue;

		token = match_token(p, tokens, args);
		switch (token) {
			case Opt_mode:
				if (match_octal(&args[0], &option))
					return -EINVAL;
				opts->mode = option & S_IALLUGO;
				break;

			/* dbgmask option */
			case Opt_dbgmask:
				if (match_int(&args[0], &option))
					return -EINVAL;
				jifs_dbgmask = option;
				break;
		}
	}

	return 0;
}





static int jifs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode *root_i = NULL;
	//struct jifs_inode *root_ji;
	struct jifs_sb_info *sbi = NULL;
	//struct jifs_super_block *js;
	u32 random = 0;


	int err;
	unsigned long blocksize;
	int retval = -ENOMEM;

	printk("%s: called.\n", __func__);


	sbi = kzalloc(sizeof(*sbi), GFP_KERNEL);
	if (!sbi)
		goto failed;

	sb->s_fs_info = sbi;
	sbi->sb = sb;

	err = jifs_parse_options(data, &sbi->mount_opts);
	if (err)
		goto failed;

	/*
	   sbi->phys_addr = get_phys_addr(&data);
	   if (sbi->phys_addr == (phys_addr_t)ULLONG_MAX)
	   goto out;
	 */

	/* generation set to random number */
	get_random_bytes(&random, sizeof(u32));
	atomic_set(&sbi->next_generation, random);

	/* init with default values */
	sbi->mode = (S_IRUGO | S_IXUGO | S_IWUSR);
	sbi->uid = current_fsuid();
	sbi->gid = current_fsgid();
	/*
	   set_opt(sbi->s_mount_opt, XIP);
	   clear_opt(sbi->s_mount_opt, PROTECT);
	   set_opt(sbi->s_mount_opt, HUGEIOREMAP);
	   sbi->inode_maps = kzalloc(sizeof(struct inode_map), GFP_KERNEL);
	   if(!sbi->inode_maps) 
	   goto failed;
	 */


	mutex_init(&sbi->s_lock);
/*
	sbi->zeroed_page = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!sbi->zeroed_page)
		goto failed;
*/

	//	js = jifs_get_super(sb);


setup_sb:
	/* fill the superblock */
	sb->s_magic = JIFS_MAGIC;
	sb->s_op = &jifs_sops;
	sb->s_maxbytes = jifs_max_size(sb->s_blocksize_bits);
	sb->s_time_gran = 1;
	sb->s_export_op = &jifs_export_ops;
	sb->s_xattr = NULL;
//	sb->s_flags |= MS_NOSEC;


//	root_i = new_inode(sb);
	root_i = jifs_make_inode(sb, S_IFDIR | 0755, &simple_dir_operations);
	if (!root_i) {
		pr_err("root inode allocation failed\n");
		return -ENOMEM;
	}
	inode_init_owner(root_i, NULL, S_IFDIR);

	/* fill root inode's information */
//	root_i->i_ino = JIFS_ROOT_INO;
	root_i->i_sb = sb;
//	root_i->i_op = &jifs_dir_inode_ops;
	//root_i->i_fop = &jifs_dir_ops;
	root_i->i_op = &simple_dir_inode_operations;

	/* d_make_root returns dentry of root */
	set_nlink(root_i, 2);
	sb->s_root = d_make_root(root_i);
	if (!sb->s_root) {
		pr_err("root creation failed\n");
		return -ENOMEM;
	}

	/* file system을 시작할 때 있을 파일을 만듬. */
	jifs_create_files (sb, sb->s_root);

	return 0;

failed:
	jifs_dbg_verbose("super_fill failed.\n");
	return retval;
}

struct jifs_dentry *jifs_find_dentry(struct super_block *sb,
		struct jifs_inode *pi, struct inode *inode, const char *name,
		unsigned long name_len)
{
	/*
	struct jifs_inode_info *si = JIFS_I(inode);
	struct jifs_inode_info_header *sih = &si->header;
	unsigned long hash;
	*/
	struct jifs_dentry *direntry = NULL;

	return direntry;
}


static struct inode *jifs_nfs_get_inode(struct super_block *sb,
		u64 ino, u32 generation)
{
	struct inode *inode;

	if (ino < JIFS_ROOT_INO)
		return ERR_PTR(-ESTALE);

	if (ino < LONG_MAX)
		return ERR_PTR(-ESTALE);

	inode = jifs_iget(sb, ino);
	if (IS_ERR(inode))
		return ERR_CAST(inode);

	return inode;
}

static struct dentry *jifs_fh_to_dentry(struct super_block *sb,
		struct fid *fid, int fh_len, int fh_type)
{
	return generic_fh_to_dentry(sb, fid, fh_len, fh_type,
			jifs_nfs_get_inode);
}

static struct dentry *jifs_fh_to_parent(struct super_block *sb,
		struct fid *fid, int fh_len, int fh_type)
{
	return generic_fh_to_parent(sb, fid, fh_len, fh_type,
			jifs_nfs_get_inode);
}






static void jifs_i_callback(struct rcu_head *head)
{
	struct inode *inode = container_of(head, struct inode, i_rcu);
	kmem_cache_free(jifs_inode_cachep, JIFS_I(inode));
}


static void jifs_destroy_inode(struct inode *inode)
{
	call_rcu(&inode->i_rcu, jifs_i_callback);
}

/*--------------------inode cache----------------------*/

static void init_once(void *foo)
{
	struct jifs_inode_info *ji = foo;

	inode_init_once(&ji->vfs_inode);
}

static int __init init_inodecache(void)
{
	jifs_inode_cachep = kmem_cache_create("jifs_inode_cache",
			sizeof(struct jifs_inode_info), 0,
			(SLAB_RECLAIM_ACCOUNT|SLAB_MEM_SPREAD|
			 SLAB_ACCOUNT),
			init_once);
	if (jifs_inode_cachep == NULL)
		return -ENOMEM;
	return 0;
}

static void destroy_inodecache(void)
{
	/*
	 * Make sure all delayed rcu free inodes are flushed before we
	 * destroy cache.
	 */
	rcu_barrier();
	kmem_cache_destroy(jifs_inode_cachep);
}

/* ----------------operations definition--------------------*/



/* super block operations */
static const struct super_operations jifs_sops = {
	//	.alloc_inode	= jifs_alloc_inode,
	.destroy_inode	= jifs_destroy_inode,
	//	.write_inode	= jifs_write_inode,
	.drop_inode		= generic_delete_inode,
	.put_super		= jifs_put_super,
	.statfs			= simple_statfs,
	//	.remount_fs		= jifs_remount,
};

# if 0
static const struct export_operations jifs_export_ops = {
	.fh_to_dentry	= jifs_fh_to_dentry,
	.fh_to_parent	= jifs_fh_to_parent,
//	.get_parent		= jifs_get_parent,
};
# endif


/* -------------------module mounting----------------------- */

static struct dentry *jifs_mount(struct file_system_type *fs_type,
		int flags, const char *dev_name, void *data)
{
	return mount_bdev(fs_type, flags, dev_name, data, jifs_fill_super);
}


static struct file_system_type jifs_type = {
	.owner			= THIS_MODULE,
	.name			= "jifs",
	.mount			= jifs_mount,
	.kill_sb		= kill_block_super,
};
MODULE_ALIAS_FS("jifs");

static int __init init_jifs(void)
{
	int err;

# if 0
	err = init_inodecache();
	if (err)
		return err;
# endif

	err = register_filesystem(&jifs_type);
	if (err)
		goto out;

	jifs_dbg_verbose("jifs module loaded\n");
	return 0;
out:
	pr_err("cannot register filesystem\n");
	destroy_inodecache();
	return err;
}

static void __exit exit_jifs(void)
{
	int const ret = unregister_filesystem(&jifs_type);

	if (ret != 0)
		pr_err("cannot unregister filesystem\n");
	jifs_dbg_verbose("jifs module unloaded\n");

# if 0
	destroy_inodecache();
# endif
}

MODULE_AUTHOR("Jaeyoun");
MODULE_DESCRIPTION("jaeyoun ideal file system");
MODULE_LICENSE("GPL");
module_init(init_jifs);
module_exit(exit_jifs);
