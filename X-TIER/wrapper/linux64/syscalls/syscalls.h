/*
 * Wrapper for system calls that may be used within an event-based module.
 */

#include "syscall_addresses.h"

// The command in the command register - 42 for external function call
#define COMMAND "$42" 

// The number of the command interrupt e.g. Hypercall
#define COMMAND_INTERRUPT "$42" 

/*
 * 64-bit Calling Conventions
 *
 * 1st ARG: %RDI
 * 2nd ARG: %RSI
 * 3rd ARG: %RDX
 * 4th ARG: %RCX
 * 5th ARG: %R8
 * 6th ARG: %R9
 * 7th ARG - nth ARG: on stack from right to left
 */

inline long sys_open(const char __user *filename, int flags, umode_t mode)
{  	
	unsigned long fd = 0;
    
        // CALL is executed
        __asm__ volatile("mov %1, %%rdi;"				// ARG1: filename
			 "mov %2, %%rsi;"				// ARG2; flags
			 "mov %3, %%rdx;"				// ARG3; mode
			 "mov $" SYS_OPEN_ADDRESS ", %%rbx;"		// Target Address in RBX
			 "mov " COMMAND ", %%rax;"			// COMMAND in RAX
                         "int " COMMAND_INTERRUPT ";"   		// Send command interrupt
			 // Save Return value
			 "mov %%rax, %0;"
                        : "=m"(fd)					// Return value
			: "m"(filename), "m"(flags), "m"(mode)		// ARGS
                        : "rdi", "rsi", "rdx", "rbx"
                        );
	
	// Return to caller
        return fd;
}

inline long sys_read(unsigned int fd, char __user *data, size_t count)
{
	unsigned long bytes = 0;
    
        // CALL is executed
        __asm__ volatile("mov %1, %%rdi;"				// ARG1: fd
			 "mov %2, %%rsi;"				// ARG2; data
			 "mov %3, %%rdx;"				// ARG3; count
			 "mov $" SYS_READ_ADDRESS ", %%rbx;"		// Target Address in RBX
			 "mov " COMMAND ", %%rax;"			// COMMAND in RAX
                         "int " COMMAND_INTERRUPT ";"   		// Send command interrupt
			 // Save Return value
			 "mov %%rax, %0;"
                        : "=m"(bytes)					// Return value
			: "m"(fd), "m"(data), "m"(count)		// ARGS
                        : "rdi", "rsi", "rdx", "rbx"
                        );
	
	// Return to caller
        return bytes;
}


inline long sys_close(unsigned int fd)
{
      unsigned long err = 0;
    
      // CALL is executed
	__asm__ volatile("mov %1, %%rdi;"				// ARG1: fd
			 "mov $" SYS_CLOSE_ADDRESS ", %%rbx;"		// Target Address in RBX
			 "mov " COMMAND ", %%rax;"			// COMMAND in RAX
			 "int " COMMAND_INTERRUPT ";"   			// Send command interrupt
			 // Save Return value
			 "mov %%rax, %0;"
                        : "=m"(err)					// Return value
			: "m"(fd)					// ARGS
                        : "rdi", "rbx"
                        );
      
      // Return to caller
      return err;
}

inline long sys_lseek(unsigned int fd, off_t offset, unsigned int origin)
{
      unsigned long err = 0;
    
        // CALL is executed
        __asm__ volatile("mov %1, %%rdi;"				// ARG1: fd
			 "mov %2, %%rsi;"				// ARG2; offset
			 "mov %3, %%rdx;"				// ARG3; origin
			 "mov $" SYS_LSEEK_ADDRESS ", %%rbx;"		// Target Address in RBX
			 "mov " COMMAND ", %%rax;"			// COMMAND in RAX
                         "int " COMMAND_INTERRUPT ";"   		// Send command interrupt
			 // Save Return value
			 "mov %%rax, %0;"
                        : "=m"(err)					// Return value
			: "m"(fd), "m"(offset), "m"(origin)		// ARGS
                        : "rdi", "rsi", "rdx", "rbx"
                        );
	
	// Return to caller
        return err;
}


inline long sys_execve(const char __user *name, const char __user *const __user *argv,
		const char __user *const __user *envp, struct pt_regs *regs)
{
      unsigned long err = 0;
    
        // CALL is executed
        __asm__ volatile("mov %1, %%rdi;"				// ARG1: name
			 "mov %2, %%rsi;"				// ARG2; argv
			 "mov %3, %%rdx;"				// ARG3: envp
			 "mov %4, %%rcx;"				// ARG4: regs
			 "mov $" SYS_EXECVE_ADDRESS ", %%rbx;"		// Target Address in RBX
			 "mov " COMMAND ", %%rax;"			// COMMAND in RAX
                         "int " COMMAND_INTERRUPT ";"   		// Send command interrupt
			 // Save Return value
			 "mov %%rax, %0;"
                        : "=m"(err)					// Return value
			: "m"(name), "m"(argv), "m"(envp),
			  "m"(regs)					// ARGS
                        : "rdi", "rsi", "rdx", "rcx", "rbx"
                        );
	
	// Return to caller
        return err;
}

inline long sys_getdents(unsigned int fd, struct linux_dirent __user *dirent, unsigned int count)
{
      unsigned long err = 0;
    
        // CALL is executed
        __asm__ volatile("mov %1, %%rdi;"                               // ARG1: fd
                         "mov %2, %%rsi;"                               // ARG2; dirent
                         "mov %3, %%rdx;"                               // ARG3; count
                         "mov $" SYS_GETDENTS_ADDRESS ", %%rbx;"        // Target Address in RBX
                         "mov " COMMAND ", %%rax;"                      // COMMAND in RAX
                         "int " COMMAND_INTERRUPT ";"                   // Send command interrupt
                         // Save Return value
                         "mov %%rax, %0;"
                        : "=m"(err)                                     // Return value
                        : "m"(fd), "m"(dirent), "m"(count)                // ARGS
                        : "rdi", "rsi", "rdx", "rbx"
                        );
        
        // Return to caller
        return err;
}
