// The command in the command register - 42 for external function call
#define COMMAND "$42" 

// The number of the command interrupt e.g. Hypercall
#define COMMAND_INTERRUPT "$42" 

// Data will be patched by the shellcode
// Place this variables into text to get a fixed offset
unsigned long kernel_esp __attribute__ ((section (".text"))) = 0;
unsigned long target_address __attribute__ ((section (".text"))) = 0;


struct ansi_string
{
	short Length;
	short MaximumLength;
	char *buf;
};

struct unicode_string
{
        short Length;
        short MaximumLength;
        char *buf;
};

/*
 * Calling Conventions
 * 
 * All arguments are on the stack from right to left.
 *
 */

/*
 * RtlUnicodeStringToAnsiString
 * 
 * Convert a Unicode String into an Ansi String.
 * \note We assume the unicode_string resides in kernel accessible memory e.g.
 * outside of the modules data/stack area.
 */
unsigned long RtlUnicodeStringToAnsiString(struct ansi_string *s, struct unicode_string *us, char allocate)
{
	// Stores the size of the data that has to be placed on 
	// the kernel stack
	unsigned long esp_offset = 0;
	
	// Stores the return value of the function
	unsigned long rtl_unicode_string_to_ansi_string_ret = 0;
	
	// Loop counter
	int i, j;

        // COPY arguments
	/*
	 * Stack Layout
	 * ANSI_STRING
	 * BUF
	 * ARGS
	 */
	struct ansi_string *new_s = 0;
	unsigned long new_buf = 0; // The new buf on the kernel stacl
	// Reserve space for string and buf on kernel stack
	esp_offset += sizeof(struct ansi_string) + s->MaximumLength;
	// Change buf pointer to new value
	new_buf = kernel_esp - (sizeof(struct ansi_string) + s->MaximumLength);
	// Set AnsiString Pointer
	new_s = (void *)(kernel_esp - sizeof(struct ansi_string));
	new_s->Length = s->Length;
	new_s->MaximumLength = s->MaximumLength;
	new_s->buf = (void *)new_buf;

        // CALL is executed
        __asm__ volatile("mov %0, %%ebx;"		// Target Address in EBX
			 "mov %1, %%eax;"	        // MOV orig kernel_stack into eax
			 "sub %2, %%eax;"               // Decrease the stack pointer by the amount
							// of data that has been added to the kernel stack.
			 // Move args to registers
			 // Since the stack will be changed
			 "mov %3, %%edi;"		// allocate
			 "mov %4, %%esi;"		// us
			 "mov %5, %%edx;"		// new_s
			 
			 "push %%ebp;"			// SAVE EBP
			 "mov %%esp, %%ebp;"		// SAVE stack pointer
			 "mov %%eax, %%esp;" 		// Set stack pointer
			 
			 // Set Args
			 "push %%edi;"			// allocate
			 "push %%esi;"			// us
			 "push %%edx;"			// s
			 
			 // Execute call
			 "mov " COMMAND ", %%eax;"	// COMMAND in RAX
                         "int " COMMAND_INTERRUPT ";"   // Send command interrupt
			 "mov %%ebp, %%esp;"		// Restore RSP
			 "pop %%ebp;"			// Restore RBP
			 // Save Return value
			 "mov %%eax, %6;"
                        :
                        :"m"(target_address), "m"(kernel_esp), 
			 "m"(esp_offset),
			 // ARGS
			 "m"(allocate), "m"(us), "m"(new_s),
			 // Return value
			 "m"(rtl_unicode_string_to_ansi_string_ret)
                        :"eax", "ebx", "edi", "esi", "edx"
                        );
	
	// Copy the data back
	s->Length = new_s->Length;
	
	// Must be <= since length does not include the terminating
	// \0 character.
	for(i = 0; i <= new_s->Length; i++)
	{
	  s->buf[i] = ((char *)new_buf)[i];
	}
	
	return rtl_unicode_string_to_ansi_string_ret;
}
