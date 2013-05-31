#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/fcntl.h>
#include <linux/cred.h>
#include <asm/cacheflush.h>

// ACE - Access Control Enforcer
unsigned long orig_open = 0xc1128a00;
unsigned long *open_tbl = 0;
asmlinkage long (*orig_sys_open)(const char __user *filename, int flags, umode_t mode) = (void *)0xc1128a00;


// Remvoe later
void set_addr_rw(unsigned long addr) {

    unsigned int level;
    pte_t *pte = lookup_address(addr, &level);

    if (pte->pte &~ _PAGE_RW) pte->pte |= _PAGE_RW;

}

void set_addr_ro(unsigned long addr) {

    unsigned int level;
    pte_t *pte = lookup_address(addr, &level);

    pte->pte = pte->pte &~_PAGE_RW;

}


long policy_check(char *filename, unsigned int euid)
{
	// Simple checks
	if (strcmp(filename, "/tmp/secret.txt") == 0)
	{
		if (euid == 1001)
			return 0;
		else
			return -1;
	}

	return 0;
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
	cur = get_current();

	if (!cur)
	{
		printk("[ACE - ERROR] - Could not get the current task!\n");
		return (-ENOENT);
	}

	// Get the credential of the task
	if (!cur->cred)
	{
		printk("[ACE - ERROR] - Could not get the credential of the current task!\n");
		return (-ENOENT);
	}

	printk("[ACE - INFO] Checking file '%s'...\n", fname);

	if (policy_check(fname, cur->cred->euid) == 0)
		return orig_sys_open(filename, flags, mode);
	else
		return (-EACCES);	
}


static int __init ace_init(void)
{
	open_tbl = (unsigned long *)(0xc1538160 + (4*5));


	set_addr_rw(0xc1538160);

	(*open_tbl) = (unsigned long)&my_open;

	set_addr_ro(0xc1538160);

	return 0;
}

static void __exit ace_exit(void)
{
	
	set_addr_rw(0xc1538160);

	(*open_tbl) = orig_open;

	set_addr_ro(0xc1538160);
        return;
}

module_init(ace_init);
module_exit(ace_exit);

MODULE_LICENSE("GPL");
