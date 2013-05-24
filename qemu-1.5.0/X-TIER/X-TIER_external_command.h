/*
 * X-TIER_external_command.h
 *
 *  Created on: Feb 22, 2013
 *      Author: fenix
 */

#ifndef XTIER_EXTERNAL_COMMAND_H_
#define XTIER_EXTERNAL_COMMAND_H_

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define XTIER_EXTERNAL_COMMAND_PIPE "/tmp/qemu_XTIER_external_command_pipe"

// The return value has exactly 19 characters
// "0x" followed by +/- followed by 16 hexadecimal characters
#define XTIER_EXTERNAL_COMMAND_RETURN_VALUE_FORMAT "0x%016lx"
#define XTIER_EXTERNAL_COMMAND_RETURN_VALUE_SIZE 18

/*
 * Structs for receiving external commands.
 */
enum external_command_type
{
	INJECTION = 1
};

enum externel_command_redirect
{
	NONE = 0,
	PIPE = 1
};

struct XTIER_external_command
{
	// Type of the command
	enum external_command_type type;
	// The length of the data that will follow the external command struct
	unsigned int data_len;
	// Output redirection?
	enum externel_command_redirect redirect;
};

struct XTIER_external_command_redirect
{
	// Type
	enum externel_command_redirect type;
	// Pipe or socket name
	char filename[255];
	// Stream for the opened file
	FILE *stream;
};

struct XTIER_data_transfer
{
	// Length of the data to come
	unsigned int len;
};

// Return value
static inline void XTIER_external_command_send_return_value(FILE *stream, long value)
{
	fprintf(stream, "" XTIER_EXTERNAL_COMMAND_RETURN_VALUE_FORMAT, value);
}

static inline long XTIER_external_command_extract_return_value(char *data, long data_size)
{
	char **endptr = NULL;

	return (signed long)strtoul(data + data_size - XTIER_EXTERNAL_COMMAND_RETURN_VALUE_SIZE, endptr, 16);
}


#endif /* XTIER_EXTERNAL_COMMAND_H_ */
