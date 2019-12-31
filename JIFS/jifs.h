/*
 * BRIEF DESCRIPTION
 *
 * Definitions for the JIFS filesystem.
 *
 * Copyright 2019 SungKyunKwan university,
 * dicl lab, Jaeyoun Nam <siisee111@gmail.com>
 * Copyright 2012-2013 Intel Corporation
 * Copyright 2009-2011 Marco Stornelli <marco.stornelli@gmail.com>
 * Copyright 2003 Sony Corporation
 * Copyright 2003 Matsushita Electric Industrial Co., Ltd.
 * 2003-2004 (c) MontaVista Software, Inc. , Steve Longerbeam
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/fs.h>
#include <linux/dax.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/crc16.h>
#include <linux/mutex.h>
#include <linux/pagemap.h>
#include <linux/backing-dev.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/rcupdate.h>
#include <linux/types.h>
#include <linux/rbtree.h>
#include <linux/radix-tree.h>
#include <linux/version.h>
#include <linux/kthread.h>
#include <linux/buffer_head.h>
#include <linux/uio.h>
#include <asm/tlbflush.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
#include <linux/pfn_t.h>
#endif
#include "jifs_def.h"

#define PAGE_SHIFT_2M 21
#define PAGE_SHIFT_1G 30

#define JIFS_ASSERT(x)                                     	\
	if (!(x)) {                                             \
		printk(KERN_WARNING "assertion failed %s:%d: %s\n", \
				__FILE__, __LINE__, #x); 				\
	}


#ifdef pr_fmt
#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#endif

#define jifs_dbg(s, args...)			pr_info(s, ## args)

extern unsigned int jifs_dbgmask;

#define JIFS_DBGMASK_MMAPHUGE          (0x00000001)
#define JIFS_DBGMASK_MMAP4K            (0x00000002)
#define JIFS_DBGMASK_MMAPVERBOSE       (0x00000004)
#define JIFS_DBGMASK_MMAPVVERBOSE      (0x00000008)
#define JIFS_DBGMASK_VERBOSE           (0x00000010)
#define JIFS_DBGMASK_TRANSACTION       (0x00000020)

#define jifs_dbg_mmaphuge(s, args ...)		 \
		((jifs_dbgmask & JIFS_DBGMASK_MMAPHUGE) ? jifs_dbg(s, args) : 0)
#define jifs_dbg_mmap4k(s, args ...)		 \
		((jifs_dbgmask & JIFS_DBGMASK_MMAP4K) ? jifs_dbg(s, args) : 0)
#define jifs_dbg_mmapv(s, args ...)		 \
		((jifs_dbgmask & JIFS_DBGMASK_MMAPVERBOSE) ? jifs_dbg(s, args) : 0)
#define jifs_dbg_mmapvv(s, args ...)		 \
		((jifs_dbgmask & JIFS_DBGMASK_MMAPVVERBOSE) ? jifs_dbg(s, args) : 0)

#define jifs_dbg_verbose(s, args ...)		 \
		((jifs_dbgmask & JIFS_DBGMASK_VERBOSE) ? jifs_dbg(s, ##args) : 0)
#define jifs_dbg_trans(s, args ...)		 \
		((jifs_dbgmask & JIFS_DBGMASK_TRANSACTION) ? jifs_dbg(s, ##args) : 0)


#define set_opt(o, opt)					(o |= JIFS_MOUNT_ ## opt)
#define clear_opt(o, opt)				(o &= ~JIFS_MOUNT_ ## opt)
#define test_opt(o, opt)				(JIFS_SB(sb)->s_mount_opt & JIFS_MOUNT_ ## opt)

/*---------------------JIFS structure-----------------------*/

enum jifs_inode_type {
	jifs_inode_node,
	jifs_inode_prop,
};

union jifs_inode_data {
	struct device_node		*node;
	struct property			*prop;
};

struct jifs_inode_info {
	struct inode				vfs_inode;
	enum jifs_inode_type		type;
	union jifs_inode_data		u;

};

struct jifs_mount_opts {
		umode_t mode;
};


/*
 * Structure of an inode in JIFS. Things to keep in mind when modifying it.
 * 1) Keep the inode size to within 96 bytes if possible. This is because
 *    a 64 byte log-entry can store 48 bytes of data and we would like
 *    to log an inode using only 2 log-entries
 * 2) root must be immediately after the qw containing height because we update
 *    root and height atomically using cmpxchg16b in jifs_decrease_btree_height 
 * 3) i_size, i_ctime, and i_mtime must be in that order and i_size must be at
 *    16 byte aligned offset from the start of the inode. We use cmpxchg16b to
 *    update these three fields atomically.
 */
struct jifs_inode {
	/* first 48 bytes */
	__le16	i_rsvd;         /* reserved. used to be checksum */
	u8	    height;         /* height of data b-tree; max 3 for now */
	u8	    i_blk_type;     /* data block size this inode uses */
	__le32	i_flags;            /* Inode flags */
	__le64	root;               /* btree root. must be below qw w/ height */
	__le64	i_size;             /* Size of data in bytes */
	__le32	i_ctime;            /* Inode modification time */
	__le32	i_mtime;            /* Inode b-tree Modification time */
	__le32	i_dtime;            /* Deletion Time */
	__le16	i_mode;             /* File mode */
	__le16	i_links_count;      /* Links count */
	__le64	i_blocks;           /* Blocks count */

	/* second 48 bytes */
	__le64	i_xattr;            /* Extended attribute block */
	__le32	i_uid;              /* Owner Uid */
	__le32	i_gid;              /* Group Id */
	__le32	i_generation;       /* File version (for NFS) */
	__le32	i_atime;            /* Access time */

	struct {
		__le32 rdev;    /* major/minor # */
	} dev;              /* device inode */
	__le32 padding;     /* pad to ensure truncate_item starts 8-byte aligned */
};

#define JIFS_SB_SIZE 512		/* must be power of two */


/*
 * JIFS super-block data in memory
 */
struct jifs_sb_info {
	struct super_block *sb;
	struct jifs_super_block *jifs_sb;
	struct block_device *s_bdev;

	/*
	 * base physical and virtual address of JIFS (which is also
	 * the pointer to the super block)
	 */
	phys_addr_t	phys_addr;
	void		*virt_addr;

	unsigned long	num_blocks;

	/*
	 * Backing store option:
	 * 1 = no load, 2 = no store,
	 * else do both
	 */
	unsigned int	jifs_backing_option;

	/* Mount options */
	unsigned long	bpi;
	unsigned long	num_inodes;
	unsigned long	blocksize;
	unsigned long	initsize;
	unsigned long	s_mount_opt;
	struct jifs_mount_opts	mount_opts;
	kuid_t		uid;    /* Mount uid for root directory */
	kgid_t		gid;    /* Mount gid for root directory */
	umode_t		mode;   /* Mount mode for root directory */
	atomic_t	next_generation;
	/* inode tracking */
	unsigned long	s_inodes_used_count;
	unsigned long	reserved_blocks;

	struct mutex 	s_lock;	/* protects the SB's buffer-head */

	int cpus;
	struct proc_dir_entry *s_proc;

	/* ZEROED page for cache page initialized */
	void *zeroed_page;

	/* Per-CPU journal lock */
	spinlock_t *journal_locks;

	/* Per-CPU inode map */
	struct inode_map	*inode_maps;

	/* Decide new inode map id */
	unsigned long map_id;

	/* Per-CPU free block list */
	struct free_list *free_lists;

	/* Shared free block list */
	unsigned long per_list_blocks;
//	struct free_list shared_free_list;

	struct dax_device *s_daxdev;
};


/*
 * Structure of a directory log entry in jifs.
 * Update DIR_LOG_REC_LEN if modify this struct!
 */
struct jifs_dentry {
	u8	entry_type;
	u8	name_len;						/* length of dentry name */
	u8	file_type;
	u8	invalid;
	__le16	de_len;						/* length of this dentry */
	__le16	links_count;
	__le32	mtime;						/* for both mtime and ctime */
	__le64	ino;						/* inode no pointed to by this entry */
	__le64	size;
	char	name[JIFS_NAME_LEN + 1];	/* File name */
} __attribute((__packed__));
	




static inline struct jifs_sb_info *JIFS_SB(struct super_block *sb)
{
	return sb->s_fs_info;
}

static inline struct jifs_inode_info *JIFS_I(struct inode *inode)
{
	return container_of(inode, struct jifs_inode_info, vfs_inode);
}

static inline struct jifs_super_block *jifs_get_super(struct super_block *sb)
{
	struct jifs_sb_info *sbi = JIFS_SB(sb);

	return (struct jifs_super_block *)sbi->virt_addr;
}



/* dir.c */
extern const struct file_operations jifs_dir_ops;


/* namei.c */
extern const struct inode_operations jifs_dir_inode_ops;
extern struct dentry *jifs_get_parent(struct dentry *child);


/* inode.c */


/* file.c */
extern const struct file_operations jifs_file_ops;
