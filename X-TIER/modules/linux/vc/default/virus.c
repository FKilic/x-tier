#include <linux/module.h>
#include <linux/init.h>
//#include <linux/cryptohash.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/swab.h>
#include <asm/uaccess.h>

#include "../../../../wrapper/linux64/syscalls/syscalls.h"

#define BUFFER_SIZE 4096
#define SHA_DATA_SIZE 64
#define SHA_WORKSPACE_WORDS 80


// SHA1 implementation from the kernel
/* The SHA f()-functions.  */

#define f1(x,y,z)   (z ^ (x & (y ^ z)))		/* x ? y : z */
#define f2(x,y,z)   (x ^ y ^ z)			/* XOR */
#define f3(x,y,z)   ((x & y) + (z & (x ^ y)))	/* majority */

/* The SHA Mysterious Constants */

#define K1  0x5A827999L			/* Rounds  0-19: sqrt(2) * 2^30 */
#define K2  0x6ED9EBA1L			/* Rounds 20-39: sqrt(3) * 2^30 */
#define K3  0x8F1BBCDCL			/* Rounds 40-59: sqrt(5) * 2^30 */
#define K4  0xCA62C1D6L			/* Rounds 60-79: sqrt(10) * 2^30 */

/**
 * sha_transform - single block SHA1 transform
 *
 * @digest: 160 bit digest to update
 * @data:   512 bits of data to hash
 * @W:      80 words of workspace (see note)
 *
 * This function generates a SHA1 digest for a single 512-bit block.
 * Be warned, it does not handle padding and message digest, do not
 * confuse it with the full FIPS 180-1 digest algorithm for variable
 * length messages.
 *
 * Note: If the hash is security sensitive, the caller should be sure
 * to clear the workspace. This is left to the caller to avoid
 * unnecessary clears between chained hashing operations.
 */
void sha_transform(__u32 *digest, const char *in, __u32 *W)
{
	__u32 a, b, c, d, e, t, i;

	for (i = 0; i < 16; i++)
		W[i] = be32_to_cpu(((const __be32 *)in)[i]);

	for (i = 0; i < 64; i++)
		W[i+16] = rol32(W[i+13] ^ W[i+8] ^ W[i+2] ^ W[i], 1);

	a = digest[0];
	b = digest[1];
	c = digest[2];
	d = digest[3];
	e = digest[4];

	for (i = 0; i < 20; i++) {
		t = f1(b, c, d) + K1 + rol32(a, 5) + e + W[i];
		e = d; d = c; c = rol32(b, 30); b = a; a = t;
	}

	for (; i < 40; i ++) {
		t = f2(b, c, d) + K2 + rol32(a, 5) + e + W[i];
		e = d; d = c; c = rol32(b, 30); b = a; a = t;
	}

	for (; i < 60; i ++) {
		t = f3(b, c, d) + K3 + rol32(a, 5) + e + W[i];
		e = d; d = c; c = rol32(b, 30); b = a; a = t;
	}

	for (; i < 80; i ++) {
		t = f2(b, c, d) + K4 + rol32(a, 5) + e + W[i];
		e = d; d = c; c = rol32(b, 30); b = a; a = t;
	}

	digest[0] += a;
	digest[1] += b;
	digest[2] += c;
	digest[3] += d;
	digest[4] += e;
}








// Virustotal module
int infected_file(u32 *digest)
{
	// We do this on the host.
	// However instead of using a hypercall we in this case demonstrate communication
	// using a simple printk statement.
	return printk("SHA1: %08x%08x%08x%08x%08x\n", digest[0], digest[1], digest[2], digest[3], digest[4]);	
}

void finish_last_block(u32 *digest, u8 *block, u32 *workspace, int block_len, u64 message_len)
{
	int i;
	// Set position to block_data + 1, since we will set the next byte to 0x80 before
	// we start to fille the remainder of the block with 0s
	int current_pos = block_len + 1;
	u64 *length = 0;
	u64 length_bits = message_len * 8;

	// Set next bit of block to 1 and the following bits to 0
	*(block + block_len) = 0x80;

	// Do we have to process two blocks or a single block?
	if(block_len >= 56)
	{
		// Two blocks
		for(i = current_pos; i >= 0 && i < 64; i++)
		{
			*(block + i) = 0;
		}

		// Transform
		sha_transform(digest, block, workspace);

		// Update position
		current_pos = 0;
	}

	// Fill with 0's
	for(i = current_pos; i >= 0 && i < 56; i++)
		*(block + i) = 0;

	// Add original length - Big endian
	length = ((u64 *)(block + i));
	(*length) = length_bits;
	__swab64s(length);

	// Transform
	sha_transform(digest, block, workspace);
}

mm_segment_t my_get_fs(void)
{
	unsigned long ks = 0;
	struct thread_info *ti = 0; 

	ks = (unsigned long)__get_cpu_var(kernel_stack);

	ti = (void *)(ks + KERNEL_STACK_OFFSET - THREAD_SIZE);

	return ti->addr_limit;
}

void my_set_fs(mm_segment_t seg)
{
	unsigned long ks = 0;
        struct thread_info *ti = 0;

        ks = (unsigned long)__get_cpu_var(kernel_stack);

        ti = (void *)(ks + KERNEL_STACK_OFFSET - THREAD_SIZE);

        ti->addr_limit = seg;
}

//asmlinkage long my_open(const char __user *filename, int flags, umode_t mode)
asmlinkage long my_execve(const char __user *filename, const char __user *const __user *argv, const char __user *const __user *envp, struct pt_regs *regs)
{
	char *fname = 0;
	long fd = 0; 
	long result = 0;

	// SHA1 data
	u8 data[BUFFER_SIZE];
	u32 digest[5];
	u32 workspace[SHA_WORKSPACE_WORDS];
	u64 length = 0;
	int ret = 0;
	u8 *current_data = 0;

	// Disable userspace checks
	mm_segment_t old_fs;

        old_fs = my_get_fs();
        my_set_fs(KERNEL_DS);

	// Info
	printk("[ VC - INFO ] Checking file '%s'...\n", filename);

	// Get the filename	
	fname = getname(filename);

	if (!fname)
	{
		// Reenable userspace checks
                my_set_fs(old_fs);

		printk("[ VC - ERROR ] - Could not get the name of the file!\n");
		return (-ENOENT);
	}

//	if(strstr(filename, ".tar.gz") &&
//		((flags == O_RDONLY) || (flags & O_RDWR)))
//	if(!strstr(filename, "bin") && !strstr(filename, "gcc") && !strstr(filename, "scripts"))
//	if(!strstr(filename, "cc1"))
//	{
  
	// Open file
	fd = sys_open(filename, O_RDONLY, 0);
	
	if(fd < 0)
	{
		// Reenable userspace checks
		my_set_fs(old_fs);

		printk("[ VC - ERROR ] Could not open file '%s'!\n", filename);
	
		  // Provoke exeception
		*((int*)result) = 1;
		
		// This will never be executed
	}
	

	// init SHA
	digest[0] = 0x67452301;
	digest[1] = 0xefcdab89;
	digest[2] = 0x98badcfe;
	digest[3] = 0x10325476;
	digest[4] = 0xc3d2e1f0;

	// Read and calc SHA1
	while((ret = sys_read(fd, data, BUFFER_SIZE)) == BUFFER_SIZE)
	{
		length += ret;
		
		// Process block in SHA_DATA_SIZE Chunks
		current_data = data;
		
		while(ret > 0)
		{
			sha_transform(digest, current_data, workspace);
			current_data += SHA_DATA_SIZE;
			ret -= SHA_DATA_SIZE;
		}
	}

	// Finish last block
	if(ret >= 0)
	{
		printk("RET is %d\n", ret);
		length += ret;
		
		// Process block in SHA_DATA_SIZE Chunks
		current_data = data;
		
		while(ret >= SHA_DATA_SIZE)
		{
			sha_transform(digest, current_data, workspace);
			current_data += SHA_DATA_SIZE;
			ret -= SHA_DATA_SIZE;
		}

		//printk("Block before %x %x %x %x %llx\n", *((u32 *)(data + 40)), *((u32 *)(data + 44)), *((u32 *)(data + 48)), *((u32 *)(data + 52)), *((u64 *)(data + 56)));
		finish_last_block(digest, current_data, workspace, ret, length);		
		//printk("Block after %x %x %x %x %llx\n", *((u32 *)(data + 40)), *((u32 *)(data + 44)), *((u32 *)(data + 48)), *((u32 *)(data + 52)), *((u64 *)(data + 56)));

		printk("[ VC - INFO ] SHA1:  %08x%08x%08x%08x%08x\n", digest[0], digest[1], digest[2], digest[3], digest[4]);

		if(!(ret = infected_file(digest)))
		{
			// Reset file position
			//sys_lseek(fd, 0, SEEK_SET);
		  
			// Reenable userspace checks
			my_set_fs(old_fs);

			// close fd
			sys_close(fd);

			// Provoke exeception
			*((int*)result) = 1;
			
			// This will never be executed
		}
		else
		{
			// Reenable userspace checks
			my_set_fs(old_fs);

			printk("[ VC - INFO ] INFECTED file (Detections: %d)! Denying Access!\n", ret);

			sys_close(fd);
                        
                        // Reenable userspace checks
                        my_set_fs(old_fs);

			return (-EACCES);
		}
		
	}
	else
	{
		// Reenable userspace checks
		my_set_fs(old_fs);

		printk("[ VC - ERROR ] Could not read from file '%s'!\n", filename);
		
		// Reset file position
		sys_close(fd);

		// Reenable userspace checks
		my_set_fs(old_fs);

		// Provoke exeception
		*((int*)result) = 1;
		
		// This will never be executed
	}
}

static int __init virus_init(void)
{
	return 0;
}

static void __exit virus_exit(void)
{
        return;
}

module_init(virus_init);
module_exit(virus_exit);

MODULE_LICENSE("GPL");
