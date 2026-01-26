/**
* Johnson Ji
* 400499564
*
* No LSB modules are available.
* Distributor ID:	Ubuntu
* Description:	Ubuntu 25.10
* Release:	25.10
* Codename:	questing

* 6.17.0-8-generic
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

#define BUFFER_SIZE 128
#define PROC_NAME "seconds"

static unsigned long starting_jiffies; // this is the recording time from when the module is loaded, only used for this module

ssize_t proc_read(struct file *file, char *buf, size_t count, loff_t *pos);

static const struct proc_ops my_proc_ops = {
    .proc_read = proc_read,
};

static int proc_init(void)
{

        starting_jiffies = jiffies;
        proc_create(PROC_NAME, 0, NULL, &my_proc_ops);

        printk(KERN_INFO "/proc/%s created\n", PROC_NAME);

        return 0;
}

static void proc_exit(void)
{

        // removes the /proc/hello entry
        remove_proc_entry(PROC_NAME, NULL);

        printk(KERN_INFO "/proc/%s removed\n", PROC_NAME);
}

ssize_t proc_read(struct file *file, char __user *usr_buf, size_t count, loff_t *pos)
{
        int rv = 0;
        char buffer[BUFFER_SIZE];
        static int completed = 0;
        unsigned long count_time;

        if (completed)
        {
                completed = 0;
                return 0;
        }

        completed = 1;

        count_time = (jiffies - starting_jiffies) / HZ; // The time during open this module is equal to the delta interrupts in the kernel devide by the frequency, which is the HZ.
        rv = sprintf(buffer, "%lu\n", count_time);

        // copies the contents of buffer to userspace usr_buf
        if (copy_to_user(usr_buf, buffer, rv))
                return -EFAULT; // note this is because the example file will give a warning so I fix it, does not matter.

        return rv;
}

module_init(proc_init);
module_exit(proc_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module Seconds");
MODULE_AUTHOR("Johnson Ji");
