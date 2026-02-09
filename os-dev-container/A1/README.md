# 3SH3 A1 readme

## Group Name

Group - 2

## Student number and name

Johnson Ji 400499564 \
Hongliang Qi 400493278

## Assignment contribution description:

Since this assignment is not very code-intensive, so both of the members in the group write indepedently and check with each other when we are done, so technically a separate solution is submitted by each person.

## Explaination for this assignment:

Based on the given file and starting code, I add a static variable called starting_jiffies to calculate the delta jiffies and divide by HZ to calculate the total count time, which is stored as count_time. (P.S, I find there is a warning message cause by copy_to_user(usr_buf, buffer, rv), since there is no exception handle the failure, so when compling it, there is a warning message. The solution is to use if condition to brack it and return -EFAULT when fail to copy the number byte to string).

## Suppliment code for Hongliang Qi, proof of participation:

```
/**
 * seconds.c
 *
 * Kernel module that communicates with /proc file system.
 *
 * Name: Hongliang QI
 * Group: 02
 *
  -a linux distribution information
  No LSB modules are available.
  Distributor ID:	Ubuntu
  Description:	Ubuntu 25.10
  Release:	25.10
  Codename:	questing
  -r kernel version
  6.17.0-8-generic

 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#define BUFFER_SIZE 128
#define PROC_NAME "seconds"

static unsigned long start_jiffies;
/**
 * Function prototypes
 */
ssize_t proc_read(struct file *file, char *buf, size_t count, loff_t *pos);
static const struct proc_ops my_proc_ops = {.proc_read = proc_read,};
/* This function is called when the module is loaded. */
int proc_init(void)
{
  start_jiffies = jiffies;
  proc_create(PROC_NAME, 0, NULL, &my_proc_ops);
  printk(KERN_INFO "/proc/%s created\n", PROC_NAME);
  return 0;
}

/* This function is called when the module is removed. */
void proc_exit(void) {
        // removes the /proc/seconds entry
  remove_proc_entry(PROC_NAME, NULL);
  printk( KERN_INFO "/proc/%s removed\n", PROC_NAME);
}

/**
 * This function is called each time the /proc/seconds is read.
 */
ssize_t proc_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos)
{
  int rv = 0;
  char buffer[BUFFER_SIZE];
  static int completed = 0;
  unsigned long elapsed_seconds;

  if (completed) {
        completed = 0;
        return 0;
  }
  completed = 1;
  elapsed_seconds = (jiffies - start_jiffies) / HZ;
  rv = sprintf(buffer, "%lu\n", elapsed_seconds);
  // copies the contents of buffer to userspace usr_buf
  copy_to_user(usr_buf, buffer, rv);
  return rv;
}

/* Macros for registering module entry and exit points. */
module_init( proc_init );
module_exit( proc_exit );
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Seconds Module");
MODULE_AUTHOR("Hongliang QI");
```

```
## Explaination for this assignment:

Based on the given file and starting code, I add a static variable called starting_jiffies to calculate the delta jiffies and divide by HZ to calculate the total count time, which is stored as count_time. (P.S, I find there is a warning message cause by copy_to_user(usr_buf, buffer, rv), since there is no exception handle the failure, so when compling it, there is a warning message. The solution is to use if condition to brack it and return -EFAULT when fail to copy the number byte to string).
```
