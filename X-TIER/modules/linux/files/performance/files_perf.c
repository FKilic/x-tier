#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/fdtable.h>
#include <linux/path.h>
#include <linux/dcache.h>

#include "../../performance/performance.h"

static unsigned int loop = 1;

static void printFiles(struct task_struct *ts)
{
	struct fdtable *fdt = files_fdtable(ts->files);
	struct file *f = NULL;
	int i = 0;
	char buf[256];
	char *ret;
	char printed = 0;

	do
	{
		// Iterate over all fds
		for(i = 0; i < fdt->max_fds; i++)
		{
			// Get file
			f = fdt->fd[i];

			if(f != NULL && f->f_dentry != NULL)
			{
				ret = d_path(&f->f_path, buf, 256);
				
				// Filter Sockets and other fake paths.
				if(*ret == '/')
				{
					if(printed == 0)
						printk(" % 5d \t %-16s \t %s\n", ts->pid, ts->comm, ret);
					else
						printk("\t\t\t\t %s\n", ret);

					printed = 1;
				}
			}
		}

		fdt = fdt->next;
	}
	while(fdt != NULL);
	

	if(printed == 1)
		printk("\n");
}


static int __init listfiles_init(void)
{
	unsigned int i = 0;
	struct task_struct *t = &init_task;

	start_timing();

	for(i = 0; i < loop; i++)
	{
		printk("Open Files:\n\n");
        	printk("   PID \t CMD               \t NAME \n");
        	printk("   --- \t ---               \t ---- \n");

		for(t = &init_task; (t = next_task(t)) != &init_task; )
		{

			printFiles(t);

			if(t == NULL)
			{
				printk("t is NULL!\n");
				break;
			}
		}
	}

	print_time(end_timing(), loop);

	return 0;
}

static void __exit listfiles_exit(void)
{
        return;
}

module_param(loop, uint, 0);

module_init(listfiles_init);
module_exit(listfiles_exit);

MODULE_LICENSE("GPL");
