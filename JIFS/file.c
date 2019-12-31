/*
 * BRIEF DESCRIPTION
 *
 * File operations for files.
 *
 * Copyright 2019 SungKyunKwan university
 * Copyright 2015-2016 Regents of the University of California,
 * UCSD Non-Volatile Systems Lab, Andiry Xu <jix024@cs.ucsd.edu>
 * Copyright 2012-2013 Intel Corporation
 * Copyright 2009-2011 Marco Stornelli <marco.stornelli@gmail.com>
 * Copyright 2003 Sony Corporation
 * Copyright 2003 Matsushita Electric Industrial Co., Ltd.
 * 2003-2004 (c) MontaVista Software, Inc. , Steve Longerbeam
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/slab.h>
#include <linux/uio.h>
#include <linux/uaccess.h>
#include <linux/falloc.h>
#include <asm/mman.h>
#include "jifs.h"


/*
 * 파일을 여는 것.
 * inode->i_private에 저장해둔 counter를 복사해주기만 하면된다.
 */
static int jifs_open(struct inode *inode, struct file *filp)
{
	filp->private_data = inode->i_private;
	return 0;
}

/*
 * 파일 읽기.
 * 카운터를 읽고 증가시킨다.
 * ......
 */
static ssize_t jifs_read_file(struct file *filp, char *buf,
		size_t count, loff_t *offset)
{
	/* atomic_t 변수는 int이다. */
	atomic_t *counter = (atomic_t *) filp->private_data;
	int v, len;
	char tmp[20];

	v = atomic_read(counter);
	if (*offset > 0)
		v -= 1;
	else
		atomic_inc(counter);

	jifs_dbg_verbose("%s: counter = %d", __func__, v);

	len = snprintf(tmp, 20, "%d\n", v);
	if (*offset > len)
		return 0;

	if (count > len - *offset)
		count = len - *offset;

	if (copy_to_user(buf, tmp + *offset, count))
		return -EFAULT;
	*offset += count;

	return count;
}

/*
 * 파일 쓰기.
 */
static ssize_t jifs_write_file(struct file *filp, const char *buf,
		size_t count, loff_t *offset)
{
	atomic_t *counter = (atomic_t *) filp->private_data;
	char tmp[20];

	jifs_dbg_verbose("%s called", __func__);

	if (*offset != 0)
		return -EINVAL;

	if (count >= 20)
		return -EINVAL;

	memset(tmp, 0, 20);

	if (copy_from_user(tmp, buf, count))
		return -EFAULT;

	/* simple_strtol(버퍼, NULL, 진수) */
	atomic_set(counter, simple_strtol(tmp, NULL, 10));

	return count;
}

/* file operations */
const struct file_operations jifs_file_ops= {
	.open		= jifs_open,
	.read		= jifs_read_file,
	.write		= jifs_write_file,
};
