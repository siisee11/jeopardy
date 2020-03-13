#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <asm/segment.h>
#include <linux/buffer_head.h>

static int read_file (char *filename)
{
	struct file *f;
	char buf[128];
	loff_t pos = 0;

	int i;
	for (i = 0 ; i < 128; i++)
		buf[i] = 0;

	f = filp_open(filename, O_RDONLY, 0);
	if (IS_ERR(f))
		printk(KERN_ALERT "filp_open error!!.\n");
	else {
		kernel_read (f, buf, sizeof(buf), &f->f_pos);
		printk(KERN_INFO "buf:%s\n", buf);
	}
	filp_close(f, NULL);
	return 0;
}

static void write_file (char *filename, char *data)
{
	struct file *f;
	loff_t pos = 0;

	f = filp_open(filename, O_WRONLY | O_CREAT, 0644);
	if (IS_ERR(f))
		printk(KERN_ALERT "filp_open error!!.\n");
	else {
		kernel_write(f, data, sizeof(data), &f->f_pos);
	}
	filp_close(f, NULL);
}

static int __init init (void)
{
	printk ("<0> file handling start\n");
	read_file ("/etc/issue");
	write_file ("/etc/temp", "foo");
	printk ("<0> file handing end\n");

	return 0;
}

static void __exit exit (void)
{
	printk ("<0> bye\n");
}

MODULE_LICENSE ("GPL");
module_init (init);
module_exit (exit);
