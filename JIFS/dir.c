/*
 * BRIEF DESCRIPTION
 *
 * File operations for directories.
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


#if 0
static int pmfs_readdir(struct file *file, struct dir_context *ctx)
{
	struct inode *inode = file_inode(file);
	struct super_block *sb = inode->i_sb;
	struct pmfs_inode *pi;
	char *blk_base;
	unsigned long offset;
	struct pmfs_direntry *de;
	ino_t ino;

	offset = ctx->pos & (sb->s_blocksize - 1);
	while (ctx->pos < inode->i_size) {
		unsigned long blk = ctx->pos >> sb->s_blocksize_bits;

		blk_base =
			pmfs_get_block(sb, pmfs_find_data_block(inode, blk));
		if (!blk_base) {
			pmfs_dbg("directory %lu contains a hole at offset %lld\n",
					inode->i_ino, ctx->pos);
			ctx->pos += sb->s_blocksize - offset;
			continue;
		}
		if (file->f_version != inode->i_version) {
			for (i = 0; i < sb->s_blocksize && i < offset; ) {
				de = (struct pmfs_direntry *)(blk_base + i);
				/* It's too expensive to do a full
				 * dirent test each time round this
				 * loop, but we do have to test at
				 * least that it is non-zero.  A
				 * failure will be detected in the
				 * dirent test below. */
				if (le16_to_cpu(de->de_len) <
						PMFS_DIR_REC_LEN(1))
					break;
				i += le16_to_cpu(de->de_len);
			}
			offset = i;
			ctx->pos =
				(ctx->pos & ~(sb->s_blocksize - 1)) | offset;
			file->f_version = inode->i_version;
		}
		while (ctx->pos < inode->i_size
				&& offset < sb->s_blocksize) {
			de = (struct pmfs_direntry *)(blk_base + offset);
			if (!pmfs_check_dir_entry("pmfs_readdir", inode, de,
						blk_base, offset)) {
				/* On error, skip to the next block. */
				ctx->pos = ALIGN(ctx->pos, sb->s_blocksize);
				break;
			}
			offset += le16_to_cpu(de->de_len);
			if (de->ino) {
				ino = le64_to_cpu(de->ino);
				pi = pmfs_get_inode(sb, ino);
				if (!dir_emit(ctx, de->name, de->name_len,
							ino, IF2DT(le16_to_cpu(pi->i_mode))))
					return 0;
			}
			ctx->pos += le16_to_cpu(de->de_len);
		}
		offset = 0;
	}
	return 0;
}
#endif

static int jifs_readdir(struct file *file, struct dir_context *ctx)
{
	struct dentry *de = file->f_path.dentry;

	jifs_dbg("file_operations.readdir called\n");

	/* This shows that directory structure is filled already */
	if (ctx->pos > 0)
		return 0;

	/* Fill the directory structure for the first time */
	dir_emit(ctx, ".", 1, de->d_inode->i_ino, DT_DIR);
	dir_emit(ctx, "..", 2, de->d_parent->d_inode->i_ino, DT_DIR);
	dir_emit(ctx, "hello.txt", 9, FILE_INO, DT_REG);
	ctx->pos = ctx->pos + 14;

	return 0;
}




const struct file_operations jifs_dir_ops = {
	.llseek			= generic_file_llseek,
	.read			= generic_read_dir,
	.iterate		= jifs_readdir,
	.fsync			= noop_fsync,
};
