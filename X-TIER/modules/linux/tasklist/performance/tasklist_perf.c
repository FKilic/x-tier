#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/sched.h>

#include "../../performance/performance.h"

static unsigned int loop = 1;

static int __init tasklist_init(void)
{
	unsigned int i = 0;
	struct task_struct *t = &init_task;
	
	start_timing();

	for(i = 0; i < loop; i++)
	{
		printk("Running Processes:\n\n");
		printk("   PID \t CMD\n");
		printk("   --- \t ---\n");

		for(t = &init_task; (t = next_task(t)) != &init_task; )
			printk(" % 5d \t %s\n",  t->pid, t->comm);
	}

	print_time(end_timing(), loop);	
	
	return 0;
}

static void __exit tasklist_exit(void)
{
        return;
}

module_param(loop, uint, 0);

module_init(tasklist_init);
module_exit(tasklist_exit);

MODULE_LICENSE("GPL");
