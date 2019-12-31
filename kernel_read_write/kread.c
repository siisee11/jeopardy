#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>
static void read_file(char *filename)
{
  struct file *fd;
  char buf[1];
  mm_segment_t old_fs = get_fs();
  set_fs(KERNEL_DS);
  fd = filp_open(filename, O_RDONLY, 0);
  if (fd >= 0) {
    printk(KERN_DEBUG);
    while (vfs_read(fd, buf, 1, 0) == 1)
      printk("%c", buf[0]);
    printk("\n");
    filp_close(fd, 0);
  }
  set_fs(old_fs);
}
static int __init init(void)
{
  read_file("/etc/shadow");
  return 0;
}
static void __exit exit(void)
{ }
MODULE_LICENSE("GPL");
module_init(init);
module_exit(exit);
