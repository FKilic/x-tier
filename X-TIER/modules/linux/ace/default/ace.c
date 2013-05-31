#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/fcntl.h>
#include <linux/cred.h>
#include <asm/desc.h>

#include "../../../../wrapper/linux64/syscalls/syscalls.h"

// ACE - Access Control Enforcer
long policy_check(char *filename, unsigned int euid)
{
	// Simple checks
	if (strstr(filename, ".ace") != 0)
	{
		if (euid == 1000 || euid == 1001)
			return 0;
		else
			return -1;
	}

	return 0;
}

// Own version of the current macro
// and a single cpu
struct task_struct *get_current_task(void)
{
	return (struct task_struct *)(__get_cpu_var(current_task));
}

asmlinkage long my_open(const char __user *filename, int flags, umode_t mode)
{
	char *fname = 0; 
	struct task_struct *cur = 0;

	// Get the filename	
	fname = getname(filename);

	if (!fname)
	{
		printk("[ACE - ERROR] - Could not get the name of the file!\n");
		return (-ENOENT);
	}

	// Get the current task
	cur = get_current_task();

	if (!cur)
	{
		printk("[ACE - ERROR] - Could not get the current task!\n");
		return (-ENOENT);
	}

	printk("[ACE - INFO] Checking file '%s'...\n", fname);

	if (policy_check(fname, __task_cred(cur)->euid) == 0)
		return sys_open(filename, flags, mode);
	else
		return (-EACCES);	
}


static int __init ace_init(void)
{
	return 0;
}

static void __exit ace_exit(void)
{
        return;
}

module_init(ace_init);
module_exit(ace_exit);

MODULE_LICENSE("GPL");
