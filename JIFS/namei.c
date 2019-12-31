/*
 * BRIEF DESCRIPTION
 *
 * Inode operations for directories.
 *
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
#include <linux/pagemap.h>

#include "jifs.h"

static struct dentry *jifs_lookup(struct inode *dir, struct dentry *dentry,
		unsigned int flags)
{
	struct inode *file_inode = NULL;

	if (dir->i_ino != JIFS_ROOT_INO ||
			strlen("hello.txt") != dentry->d_name.len ||
			strcmp(dentry->d_name.name, "hello.txt"))
		return ERR_PTR(-ENOENT);

	/* allocate an inode object */
	file_inode = iget_locked(dir->i_sb, FILE_INO);
	if (!file_inode)
		return ERR_PTR(-ENOMEM);

	file_inode->i_size = 0;		/* file size */
	file_inode->i_mode = S_IFREG|S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;
	file_inode->i_fop = &jifs_file_ops;

	/* add the inode to the dentry object */
	d_add(dentry, file_inode);
	jifs_dbg_verbose("%s: lookup called with dentry %s.\n", __func__, 
			dentry->d_name.name);

	return NULL;
}

#if 0
static ino_t jifs_inode_by_name(struct inode *dir, struct qstr *entry,
		struct jifs_dentry **res_entry)
{
	struct super_block *sb = dir->i_sb;
	struct jifs_dentry *direntry;

	direntry = jifs_find_dentry(sb, NULL, dir,
			entry->name, entry->len);
	if (direntry == NULL)
		return 0;

	*res_entry = direntry;
	return direntry->ino;
}

struct dentry *jifs_get_parent(struct dentry *child)
{
	struct inode *inode;
	struct qstr dotdot = QSTR_INIT("..", 2);
	struct jifs_dentry *de = NULL;
	ino_t ino;

	jifs_inode_by_name(child->d_inode, &dotdot, &de);
	if (!de)
		return ERR_PTR(-ENOENT);
	ino = le64_to_cpu(de->ino);

	if (ino)
		inode = jifs_iget(child->d_inode->i_sb, ino);
	else
		return ERR_PTR(-ENOENT);

	return d_obtain_alias(inode);
}
#endif


/* inode operations */
const struct inode_operations jifs_dir_inode_ops = {
	.lookup			= jifs_lookup,
};

