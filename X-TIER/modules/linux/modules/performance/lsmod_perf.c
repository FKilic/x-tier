#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/list.h>


#include "../../performance/performance.h"

static unsigned int loop = 1;

static int __init lsmod_init(void)
{
	struct list_head *modules = (void *)0xffffffff81a28b10;
	
	struct module *m = NULL;
	
	unsigned int i = 0;


	start_timing();

	for(i = 0; i < loop; i++)
	{

		printk("Loaded Modules:\n\n");
		printk(" BASE ADDRESS \t   SIZE \t NAME\n");
		printk(" ------------ \t   ---- \t ----\n");

		list_for_each_entry(m, modules, list)
		{
			printk("%p \t % 7d \t %s\n", m->module_core, m->core_size, m->name);
		}
	}
	
	print_time(end_timing(), loop);

	return 0;
}

static void __exit lsmod_exit(void)
{
        return;
}

module_param(loop, uint, 0);

module_init(lsmod_init);
module_exit(lsmod_exit);

MODULE_LICENSE("GPL");
