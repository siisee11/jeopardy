/*
 * Demonstrate a trivial filesystem using libfs.
 *
 * Copyright 2002, 2003 Jonathan Corbet <corbet@lwn.net>
 * This file may be redistributed under the terms of the GNU GPL.
 *
 * Chances are that this code will crash your system, delete your
 * nethack high scores, and set your disk drives on fire.  You have
 * been warned.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pagemap.h> 	/* PAGE_CACHE_SIZE */
#include <linux/fs.h>     	/* This is where libfs stuff is declared */
#include <asm/atomic.h>
#include <asm/uaccess.h>	/* copy_to_user */

/*
 * Boilerplate stuff.
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jonathan Corbet");

#define LFS_MAGIC 0x19920342

#define TMPSIZE 20


/*
 * Anytime we make a file or directory in our filesystem we need to
 * come up with an inode to represent it internally.  This is
 * the function that does that job.  All that's really interesting
 * is the "mode" parameter, which says whether this is a directory
 * or file, and gives the permissions.
 */

static struct inode *lfs_make_inode(struct super_block *sb, int mode, const struct file_operations* fops)
{
	struct inode* inode;            
	inode = new_inode(sb);
	if (!inode) {
		return NULL;
	}
	inode->i_mode = mode;
	inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
	inode->i_fop = fops;
	inode->i_ino = get_next_ino();
	return inode;

}


/*
 * Open a file.  All we have to do here is to copy over a
 * copy of the counter pointer so it's easier to get at.
 */
static int lfs_open(struct inode *inode, struct file *filp)
{
	filp->private_data = inode->i_private;
	return 0;
}

/*
 * Read a file.  Here we increment and read the counter, then pass it
 * back to the caller.  The increment only happens if the read is done
 * at the beginning of the file (offset = 0); otherwise we end up counting
 * by twos.
 */

static ssize_t lfs_read_file(struct file *filp, char *buf,
		size_t count, loff_t *offset)
{
	atomic_t *counter = (atomic_t *) filp->private_data;
	int v, len;
	char tmp[TMPSIZE];
	/*
	 * Encode the value, and figure out how much of it we can pass back.
	 */
	v = atomic_read(counter);
	if (*offset > 0)
		v -= 1;  /* the value returned when offset was zero */
	else
		atomic_inc(counter);
	len = snprintf(tmp, TMPSIZE, "%d\n", v);
	if (*offset > len)
		return 0;
	if (count > len - *offset)
		count = len - *offset;
	/*
	 * Copy it back, increment the offset, and we're done.
	 */
	if (copy_to_user(buf, tmp + *offset, count))
		return -EFAULT;
	*offset += count;
	return count;
}

/*
 * Write a file.
 */
static ssize_t lfs_write_file(struct file *filp, const char *buf,
		size_t count, loff_t *offset)
{
	atomic_t *counter = (atomic_t *) filp->private_data;
	char tmp[TMPSIZE];
	/*
	 * Only write from the beginning.
	 */
	if (*offset != 0)
		return -EINVAL;
	/*
	 * Read the value from the user.
	 */
	if (count >= TMPSIZE)
		return -EINVAL;
	memset(tmp, 0, TMPSIZE);
	if (copy_from_user(tmp, buf, count))
		return -EFAULT;
	/*
	 * Store it in the counter and we are done.
	 */
	atomic_set(counter, simple_strtol(tmp, NULL, 10));
	return count;
}


/*
 * Now we can put together our file operations structure.
 */
static struct file_operations lfs_file_ops = {
	.open	= lfs_open,
	.read 	= lfs_read_file,
	.write  = lfs_write_file,
};


/*
 * Create a file mapping a name to a counter.
 */

const struct inode_operations lwfs_inode_operations = {
	.setattr        = simple_setattr,
	.getattr        = simple_getattr,
};

static struct dentry *lfs_create_file (struct super_block *sb,
		struct dentry *dir, const char *name,
		atomic_t *counter)
{
	struct dentry *dentry;
	struct inode *inode;

	/*
	 * Now we can create our dentry and the inode to go with it.
	 */
	dentry = d_alloc_name(dir, name);
	if (! dentry)
		goto out;
	inode = lfs_make_inode(sb, S_IFREG | 0644, &lfs_file_ops);
	if (! inode)
		goto out_dput;
	inode->i_private = counter;
	/*
	 * Put it all into the dentry cache and we're done.
	 */
	d_add(dentry, inode);
	return dentry;
	/*
	 * Then again, maybe it didn't work.
	 */
out_dput:
	dput(dentry);
out:
	return 0;
}


/*
 * Create a directory which can be used to hold files.  This code is
 * almost identical to the "create file" logic, except that we create
 * the inode with a different mode, and use the libfs "simple" operations.
 */
static struct dentry *lfs_create_dir (struct super_block *sb,
		struct dentry *parent, const char *name)
{
	struct dentry *dentry;
	struct inode *inode;

	dentry = d_alloc_name(parent, name);
	if (! dentry)
		goto out;

	inode = lfs_make_inode(sb, S_IFDIR | 0755, &simple_dir_operations);
	if (! inode)
		goto out_dput;
	inode->i_op = &simple_dir_inode_operations;

	d_add(dentry, inode);
	return dentry;

out_dput:
	dput(dentry);
out:
	return 0;
}



/*
 * OK, create the files that we export.
 */
static atomic_t counter, subcounter;

static void lfs_create_files (struct super_block *sb, struct dentry *root)
{
	struct dentry *subdir;
	/*
	 * One counter in the top-level directory.
	 */
	atomic_set(&counter, 0);
	lfs_create_file(sb, root, "counter", &counter);
	/*
	 * And one in a subdirectory.
	 */
	atomic_set(&subcounter, 0);
	subdir = lfs_create_dir(sb, root, "subdir");
	if (subdir)
		lfs_create_file(sb, subdir, "subcounter", &subcounter);
}



/*
 * Superblock stuff.  This is all boilerplate to give the vfs something
 * that looks like a filesystem to work with.
 */

/*
 * Our superblock operations, both of which are generic kernel ops
 * that we don't have to write ourselves.
 */
static struct super_operations lfs_s_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
};

/*
 * "Fill" a superblock with mundane stuff.
 */
static int lfs_fill_super (struct super_block *sb, void *data, int silent)
{
	struct inode *root;
	struct dentry *root_dentry;
	/*
	 * Basic parameters.
	 */
	sb->s_blocksize = VMACACHE_SIZE;
	sb->s_blocksize_bits = VMACACHE_SIZE;
	sb->s_magic = LFS_MAGIC;
	sb->s_op = &lfs_s_ops;
	/*
	 * We need to conjure up an inode to represent the root directory
	 * of this filesystem.  Its operations all come from libfs, so we
	 * don't have to mess with actually *doing* things inside this
	 * directory.
	 */
	root = lfs_make_inode (sb, S_IFDIR | 0755, &simple_dir_operations);
	inode_init_owner(root, NULL, S_IFDIR | 0755);
	if (unlikely(! root))
		goto out;
	root->i_op = &simple_dir_inode_operations;
	//	root->i_fop = &simple_dir_operations;
	/*
	 * Get a dentry to represent the directory in core.
	 */
	set_nlink(root, 2);
	root_dentry = d_make_root(root);
	if (! root_dentry)
		goto out_iput;
	/*
	 * Make up the files which will be in this filesystem, and we're done.
	 */
	lfs_create_files (sb, root_dentry);
	sb->s_root = root_dentry;
	return 0;

out_iput:
	iput(root);
out:
	return -ENOMEM;
}


/*
 * Stuff to pass in when registering the filesystem.
 */
static struct dentry *lfs_get_super(struct file_system_type *fst,
		int flags, const char *devname, void *data)
{
	return mount_nodev(fst, flags, data, lfs_fill_super);
}

static struct file_system_type lfs_type = {
	.owner 		= THIS_MODULE,
	.name		= "lwnfs",
	.mount		= lfs_get_super,
	.kill_sb	= kill_litter_super,
};




/*
 * Get things set up.
 */
static int __init lfs_init(void)
{
	printk("lfs init\n");
	return register_filesystem(&lfs_type);
}

static void __exit lfs_exit(void)
{
	printk("lfs exit\n");
	unregister_filesystem(&lfs_type);
}

module_init(lfs_init);
module_exit(lfs_exit);
