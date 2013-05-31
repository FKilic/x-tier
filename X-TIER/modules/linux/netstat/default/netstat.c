#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/fdtable.h>
#include <linux/net.h>
#include <net/sock.h>
#include <net/inet_sock.h>
#include <linux/byteorder/generic.h>


static void printSockets(struct task_struct *ts)
{
	struct fdtable *fdt = files_fdtable(ts->files);
	struct socket *s = NULL;
	struct inet_sock *is = NULL;
	struct file *f = NULL;
	int i = 0;


	do
	{
		// Iterate over all fds
		for(i = 0; i < fdt->max_fds; i++)
		{
			// Get file
			f = fdt->fd[i];

			if(f != NULL)
			{

				s = (struct socket *)f->private_data;

				// Check for valid socket
				if(s != NULL && s->file == f)
				{
					if(s->sk->__sk_common.skc_family == AF_INET)
					{
						is = (struct inet_sock *)s->sk;
						
						printk(" %d.%d.%d.%d \t\t % 5d \t %d.%d.%d.%d \t\t % 5d\n", is->inet_saddr & 0xff, ((is->inet_saddr >> 8) & 0xff), ((is->inet_saddr >> 16) & 0xff), ((is->inet_saddr >> 24) & 0xff),  ntohs(is->inet_sport), is->inet_daddr & 0xff, ((is->inet_daddr >> 8) & 0xff), ((is->inet_daddr >> 16) & 0xff), ((is->inet_daddr >> 24) & 0xff), ntohs(is->inet_dport)); 			
					}
				}
			}
		}

		fdt = fdt->next;
	}
	while(fdt != NULL);
	
}


static int __init netstat_init(void)
{
	struct task_struct *t = &init_task;

	printk("Network Connections:\n\n");
        printk(" SOURCE IP      \t  PORT \t DESTINATION IP \t   PORT\n");
        printk(" ---------      \t  ---- \t -------------- \t   ----\n");

	for(t = &init_task; (t = next_task(t)) != &init_task; )
		printSockets(t);
	return 0;
}

static void __exit netstat_exit(void)
{
        return;
}

module_init(netstat_init);
module_exit(netstat_exit);

MODULE_LICENSE("GPL");
