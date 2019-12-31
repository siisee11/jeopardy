/*
 * BRIEF DESCRIPTION
 *
 * Inode methods (allocate/free/read/write).
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
#include <linux/aio.h>
#include <linux/sched.h>
#include <linux/highuid.h>
#include <linux/module.h>
#include <linux/mpage.h>
#include <linux/backing-dev.h>
#include <linux/types.h>
#include <linux/ratelimit.h>
#include "jifs.h"

#if 0
//unsigned int blk_type_to_shift[PMFS_BLOCK_TYPE_MAX] = {12, 21, 30};
//uint32_t blk_type_to_size[PMFS_BLOCK_TYPE_MAX] = {0x1000, 0x200000, 0x40000000};



static ssize_t jifs_direct_IO(struct kiocb *iocb, struct iov_iter *iter,
		loff_t offset)
{
	struct file *filp = iocb->ki_filp;
	struct address_space *mapping = filp-> f_mapping;
	struct inode *inode = mapping->host;
	ssize_t ret = 0;

//	ret = dax_do_io(iocb, inode, iter, offset, nova_dax_get_block,
//			NULL, DIO_LOCKING);
	return ret;
}

const struct address_space_operations jifs_aops_xip = {
	.direct_IO		= jifs_direct_IO,
};
#endif
