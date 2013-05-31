#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/sched.h>


static int __init tasklist_init(void)
{
	struct task_struct *t = &init_task;

	printk("Running Processes:\n\n");
	printk("   PID \t CMD\n");
	printk("   --- \t ---\n");

	for(t = &init_task; (t = next_task(t)) != &init_task; )
		printk(" % 5d \t %s\n",  t->pid, t->comm);

	return 0;
}

static void __exit tasklist_exit(void)
{
        return;
}

module_init(tasklist_init);
module_exit(tasklist_exit);

MODULE_LICENSE("GPL");
