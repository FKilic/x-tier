/*
 * X-TIER_qemu.c
 *
 *  Created on: Nov 14, 2011
 *      Author: Sebastian Vogl <vogls@sec.in.tum.de>
 */
#include "X-TIER_qemu.h"
#include "X-TIER_debug.h"
#include "X-TIER_inject.h"
#include "X-TIER_external_command.h"

#include "../include/sysemu/kvm.h"
#include "../include/sysemu/sysemu.h"

#include "../include/exec/cpu-all.h"
#include "../include/exec/cpu-common.h"

#include "../include/exec/memory.h"
#include "../include/exec/address-spaces.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

/*
 * Questions
 */
#define XTIER_QUESTION_OBTAIN_FILE_NAME 2
#define XTIER_QUESTION_OS 8
#define XTIER_QUESTION_INJECT_GET_FILE 9
#define XTIER_QUESTION_EVENT_INJECT_SELECT_MODULE 10


/*
 * TIME
 */
#define NSEC_PER_SEC 1000000000L
#define NSEC_PER_MSEC 1000000L

/*
 * TYPEDEFS
 */
// Ask a questions. Returns 1 on valid answer and -X on error.
typedef int (*XTIER_question_callback)(const char *cmdline);
typedef void (*XTIER_command_callback)(const char *cmdline);

/*
 * Structures
 */
struct XTIER_time
{
	u64 sec;
	u64 ms;
	u64 ns;
};

struct XTIER_question;

struct XTIER_choice
{
	// The number representing this choice
	int choice;
	// The description of the choice (Printed if help is requested)
	const char *description;
	// A Pointer to a sub question that may follow this choice
	struct XTIER_question *sub_question;
};

struct XTIER_question
{
	// should be one of the question ids
	int id;
	// The function that will handle the user input.
	XTIER_question_callback callback;
	// The different choices for the question.
	struct XTIER_choice *choices;
	int number_of_choices;
};

struct XTIER_command
{
	// The name of the command.
	// This string will be matched against the user input.
	const char *name;
	// The description of the command
	const char *description;
	// The callback that will be used.
	XTIER_command_callback callback;
	// The list of subcommands
	struct XTIER_command *sub_commands;
	// The number of subcommands
	int number_of_sub_commands;
};


/*
 * Globals
 */
// The current XTIER config.
struct XTIER_state _XTIER;

CPUState *_env = NULL;

int _initialized = 0;

MemoryRegion *_inject_region;

int _event_injection = 0;
u32 _auto_injection = 0;
u32 _time_injection = 0;
struct injection *_injection = 0;

struct XTIER_external_command _external_command;
struct XTIER_external_command_redirect _external_command_redirect;
int _external_command_fd = 0;

/*
 * Questions and Choices
 *
 * Update these structures to provide new questions and choices for
 * a user.
 */
struct XTIER_question *_current_question;
XTIER_command_callback _return_to;


struct XTIER_choice os_choices[] = {
		{
			.choice = XTIER_OS_UBUNTU_64,
			.description = "Ubuntu Server 64-bit",
			.sub_question = NULL,
		},
		{
			.choice = XTIER_OS_WINDOWS_7_32,
			.description = "Windows 7 32-bit",
			.sub_question = NULL,
		},
		{
			.choice = XTIER_OS_UBUNTU_32,
			.description = "Ubuntu Server 32-bit",
			.sub_question = NULL,
		},
};

struct XTIER_choice XTIER_event_inject[] = {
		{
			.choice = XTIER_INJECT_EVENT_MODULE_ACE,
			.description = "Access Control Enforcer",
			.sub_question = NULL,
		},
		{
			.choice = XTIER_INJECT_EVENT_MODULE_VIRUSTOTAL,
			.description = "Virustotal",
			.sub_question = NULL,
		}
};

struct XTIER_question top_level_questions[] = {
		{
			.id = XTIER_QUESTION_OS,
			.callback = XTIER_question_specify_os,
			.choices = os_choices,
			.number_of_choices = ARRAY_SIZE(os_choices),
		},
		{
			.id = XTIER_QUESTION_INJECT_GET_FILE,
			.callback = XTIER_question_inject_get_file,
			.choices = NULL,
			.number_of_choices = 0,
		},
		{
			.id = XTIER_QUESTION_EVENT_INJECT_SELECT_MODULE,
			.callback = XTIER_question_event_inject_select_module,
			.choices = XTIER_event_inject,
			.number_of_choices = ARRAY_SIZE(XTIER_event_inject),
		},
};


/*
 * Commands
 *
 * Update these structures to add new commands.
 */
struct XTIER_command *_current_command;
struct XTIER_command *_current_command_parent;

struct XTIER_command top_level_commands[] = {
		{
			.name = "auto-inject",
			.description = "Enable auto injection for the currently injected module.",
			.callback = XTIER_command_auto_inject,
			.sub_commands = NULL,
			.number_of_sub_commands = 0,
		},
		{
			.name = "time-inject",
			.description = "Enable timed-injection for the currently injected module.",
			.callback = XTIER_command_time_inject,
			.sub_commands = NULL,
			.number_of_sub_commands = 0,
		},
		{
			.name = "event-inject",
			.description = "Event-based injection of a kernel module.",
			.callback = XTIER_command_event_based_inject,
			.sub_commands = NULL,
			.number_of_sub_commands = 0,
		},
		{
			.name = "cont",
			.description = "Resume VM and return to 'monitor' Mode.",
			.callback = XTIER_switch_to_monitor_mode,
			.sub_commands = NULL,
			.number_of_sub_commands = 0,
		},
		{
			.name = "inject",
			.description = "Inject code into the Virtual Machine",
			.callback = XTIER_command_inject,
			.sub_commands = NULL,
			.number_of_sub_commands = 0,
		},
		{
			.name = "monitor",
			.description = "Return to 'monitor' Mode, but do not resume VM.",
			.callback = XTIER_switch_to_monitor_mode_keep_paused,
			.sub_commands = NULL,
			.number_of_sub_commands = 0,
		},
		{
			.name = "quit",
			.description = "Return to 'monitor' Mode. VM will be paused.",
			.callback = XTIER_switch_to_monitor_mode_keep_paused,
			.sub_commands = NULL,
			.number_of_sub_commands = 0,
		},
		{
			.name = "exit",
			.description = "Return to 'monitor' Mode. VM will be paused.",
			.callback = XTIER_switch_to_monitor_mode_keep_paused,
			.sub_commands = NULL,
			.number_of_sub_commands = 0,
		},
		{
			.name = "help",
			.description = "Print help information.",
			.callback = XTIER_print_help,
			.sub_commands = NULL,
			.number_of_sub_commands = 0,
		},
		{
			.name = "external",
			.description = "Receive an external command.",
			.callback = XTIER_command_receive_external_command,
			.sub_commands = NULL,
			.number_of_sub_commands = 0,
		},
};


/*
 * Functions
 */
static void _XTIER_init(void)
{
	// Init State
	_XTIER.mode = 0;
	_XTIER.os = XTIER_OS_UNKNOWN;

	// Allocate memory within the guest
	_inject_region = g_malloc(sizeof(*_inject_region));

	if (!_inject_region)
		PRINT_ERROR("Could not allocate memory");

	// Fixed size (1024 * 4096) for now.
	memory_region_init_ram(_inject_region, "X-TIER.data",
						   XTIER_MEMORY_AREA_SIZE);

	// Fixed offset (1 UL << 30) and priority (1337) for now
	memory_region_add_subregion_overlap(get_system_memory(),
										XTIER_MEMORY_AREA_ADDRESS,
									   _inject_region,
	                                    1337);

	_initialized = 1;
}

int XTIER_ioctl(unsigned int command, void *arg)
{
	return kvm_vcpu_ioctl(ENV_GET_CPU((struct CPUX86State *)_env), command, arg);
}

static struct XTIER_question * _find_top_level_question(int id)
{
	int i;
	int size = ARRAY_SIZE(top_level_questions);

	for(i = 0; i < size; i++)
	{
		if(top_level_questions[i].id == id)
			return &top_level_questions[i];
	}

	return NULL;
}

static struct XTIER_command * _find_command(const char *name)
{
	int i;
	int size;
	struct XTIER_command *cmd;

	if(_current_command)
	{
		if(!_current_command->sub_commands)
			return NULL;

		size = _current_command->number_of_sub_commands;
		cmd = _current_command->sub_commands;
	}
	else
	{
		size = ARRAY_SIZE(top_level_commands);
		cmd = top_level_commands;
	}


	for(i = 0; i < size; i++)
	{
		if(strcmp(cmd[i].name, name) == 0)
			return &cmd[i];
	}

	return NULL;
}

static struct XTIER_choice * _find_choice(struct XTIER_question *q, int choice)
{
	int i;

	for(i = 0; i < q->number_of_choices; i++)
	{
		if (q->choices[i].choice == choice)
			return &q->choices[i];
	}

	return NULL;
}

/*
 * Print a long line of data to the console.
 *
 * @param padding_left The space that will be left free on each line that the data occupies
 * @param width The width of a single line
 * @param data The pointer to the data to be printed.
 */
static void _print_long_line(int padding_left, int width, const char *data)
{
	int distance = 0;
	int printed = 0;
	char data_copy[strlen(data)];
	char *next;
	int space_left = width;

	// Copy data to be able to manipulate it
	strcpy(data_copy, data);
	next = strtok(data_copy, " \n");

	while(next)
	{
		// If the word is longer than width, we abort
		distance = strlen(next);
		if(distance > width)
		{
			PRINT_WARNING("The given data contains a word that is too long for the specified width!\n");
			return;
		}

		// Does the word fit in the current line?
		if(space_left - distance >= 0)
		{
			printed += distance;

			if(data[printed] == ' ')
			{
				PRINT_OUTPUT("%s ", next);
				space_left -= (distance + 1);
			}
			else
			{
				PRINT_OUTPUT("%s", next);
				PRINT_OUTPUT("\n%*.*s", padding_left, padding_left, "");
				space_left = width;
			}

			// Space / \n
			printed++;

			// Find next word
			next = strtok(NULL, " \n");
		}
		else
		{
			// Print new line
			PRINT_OUTPUT("\n%*.*s", padding_left, padding_left, "");
			space_left = width;
		}
	}

	PRINT_OUTPUT("\n");
}

/*
 * Print the question that is currently stored in the _current_question
 * variable.
 */
void XTIER_ask_current_question(void)
{
	if(!_current_question)
		PRINT_WARNING("There is currently no question set!\n");

	// Add new questions here
	switch(_current_question->id)
	{
		case XTIER_QUESTION_OBTAIN_FILE_NAME:
			PRINT_OUTPUT("\nPlease enter the name of the file where to save to.\n"
						"Existing files will be overwritten!\n");
			break;
		case XTIER_QUESTION_OS:
			PRINT_OUTPUT("\nPlease specify the guest OS.\nType 'help' to see the available choices.\n");
			break;
		case XTIER_QUESTION_INJECT_GET_FILE:
			PRINT_OUTPUT("\nPlease specify the binary file that contains the code that will be injected.\n");
			break;
		case XTIER_QUESTION_EVENT_INJECT_SELECT_MODULE:
			PRINT_OUTPUT("\nPlease select the event based module that you want to inject.\n");
			break;
		default:
			PRINT_WARNING("Unknown question id!\n");
			break;
	}


}


/*
 * Generic function that prints help for questions or commands.
 */
void XTIER_print_help(const char *cmdline)
{
	int size;
	int i;

	// Is help requested for the current question?
	if(_current_question)
	{
		if(!_current_question->choices)
		{
			PRINT_ERROR("This question has no options!\n");
			return;
		}

		// Print Options
		size = _current_question->number_of_choices;

		PRINT_OUTPUT("\n Available Options:\n");

		// Header
		PRINT_OUTPUT("%5.5s%-10.10s%5.5s%-40.40s\n", "", "CHOICE", "", "MEANING");
		PRINT_OUTPUT("%5.5s%-10.10s%5.5s%-40.40s\n", "", "------", "",
					"--------------------------------------------------------------------------------------");

		for(i = 0; i < size; i++)
		{
			PRINT_OUTPUT("%5.5s%-10d%5.5s", "", _current_question->choices[i].choice, "");

			_print_long_line(20, 40, _current_question->choices[i].description);
		}
	}
	// Print all subcommands if a certain command was selected
	else if(_current_command_parent && _current_command_parent->sub_commands)
	{
		size = _current_command_parent->number_of_sub_commands;

		PRINT_OUTPUT("\n Sub Commands of '%s':\n\n", _current_command_parent->name);

		// Header
		PRINT_OUTPUT("%5.5s%-20.20s%5.5s%-40.40s\n", "", "SUB COMMANDS", "", "DESCRIPTION");
		PRINT_OUTPUT("%5.5s%-20.20s%5.5s%-40.40s\n", "", "------------", "",
					"--------------------------------------------------------------------------------------");


		for(i = 0; i < size; i++)
		{
			PRINT_OUTPUT("%5.5s%-20.20s%5.5s", "", _current_command_parent->sub_commands[i].name,"");
			_print_long_line(30, 40, _current_command_parent->sub_commands[i].description);
		}

	}
	else
	{
		// Print available commands
		PRINT_OUTPUT("\n Available Commands:\n");

		// Header
		PRINT_OUTPUT("%5.5s%-20.20s%5.5s%-40.40s\n", "", "COMMANDS", "", "DESCRIPTION");
		PRINT_OUTPUT("%5.5s%-20.20s%5.5s%-40.40s\n", "", "--------", "",
					"--------------------------------------------------------------------------------------");

		size = ARRAY_SIZE(top_level_commands);

		for(i = 0; i < size; i++)
		{
			PRINT_OUTPUT("%5.5s%-20.20s%5.5s", "", top_level_commands[i].name,"");
			_print_long_line(30, 40, top_level_commands[i].description);
		}
	}

	PRINT_OUTPUT("\n");
}

/*
 * Generic function that handles all XTIER input.
 */
void XTIER_handle_input(Monitor *mon, const char *cmdline, void *opaque)
{
	char cmdline_copy[strlen(cmdline)];
	char *cmd_part = NULL;

	int ret = 0;
	int choice = 0;
	int cmdline_pointer = 0;
	struct XTIER_choice *ch = NULL;
	struct XTIER_command *cmd = NULL;

	// Copy cmdline
	strcpy(cmdline_copy, cmdline);
	cmd_part = strtok(cmdline_copy, " ");

	// Do we process a question or a commmand?
	// Lets first check if we can find the command
	while(cmd_part)
	{
		cmd = _find_command(cmd_part);

		if(cmd)
		{
			_current_command_parent = _current_command;
			_current_command = cmd;
			cmdline_pointer += strlen(cmd_part) + 1;
		}
		else
			break;

		cmd_part = strtok(NULL, " ");
	}

	if(_current_command)
	{
		// BINGO
		if(_current_command->callback)
		{
			_current_command->callback((cmdline + cmdline_pointer));
		}
		else
		{
			PRINT_ERROR("Specified command is invalid without options!\n");
			XTIER_print_help(cmdline);
		}

		_current_command = NULL;
		_current_command_parent = NULL;
	}
	// Question?
	else if(_current_question)
	{
		// Handle choice
		choice = atoi(cmdline);
		ret = _current_question->callback(cmdline);

		if(ret < 0)
		{
			_current_question = NULL;
		}
		else
		{
			// Update current question if possible.
			ch = _find_choice(_current_question, choice);
			if(ch && ch->sub_question)
			{
				// Set Question
				_current_question = ch->sub_question;
				// Ask Question
				if(_current_question)
					XTIER_ask_current_question();
			}
			else if(_return_to)
			{
				// The new command may specify a question
				_return_to = NULL;
			}
			else
			{
				_current_question = NULL;
			}
		}
	}
	else
	{
		PRINT_ERROR("Unknown Command.\n");
	}

	if(strcmp(cmdline, "cont") && strcmp(cmdline, "quit") && strcmp(cmdline, "exit"))
		PRINT_OUTPUT(XTIER_PROMPT);
}

/*
 * Enter XTIER input mode.
 * The machine will be paused and the XTIER shell will be enabled.
 */
void XTIER_switch_to_XTIER_mode(CPUState *env)
{
	PRINT_OUTPUT("\tSwitching to 'XTIER' Mode...\n\tThe VM will be stopped...\n");

	vm_stop(RUN_STATE_PAUSED);

	// set env
	_env = env;

	// Init if necessary
	if(!_initialized)
		_XTIER_init();

	// Print the current question if any
	if(_current_question)
	{
		XTIER_ask_current_question();
		PRINT_OUTPUT(XTIER_PROMPT);
	}

	// Start "shell" ;)
	XTIER_start_getting_user_input(XTIER_handle_input);
}


/*
 * Return to normal input mode.
 */
void XTIER_switch_to_monitor_mode(const char *cmdline)
{
	PRINT_OUTPUT("\tSwitching to 'monitor' Mode...\n\tThe VM will be started...\n");

	XTIER_stop_getting_user_input();

	// Reset
	_current_question = NULL;
	_current_command = NULL;

	// Continue
	vm_start();
}

/*
 * Return to normal input mode. However the VM will remain paused.
 */
void XTIER_switch_to_monitor_mode_keep_paused(const char *cmdline)
{
	PRINT_OUTPUT("\tSwitching to 'monitor' Mode...\n");

	XTIER_stop_getting_user_input();

	// Reset
	_current_question = NULL;
	_current_command = NULL;
}

void XTIER_command_receive_external_command(const char *cmdline)
{
	const char *filename = XTIER_EXTERNAL_COMMAND_PIPE;

	char *prev_module_name = NULL;
	char *prev_module_code = NULL;
	unsigned int prev_module_code_len = 0;

	int ret = 0;

	// Create named pipe - User Read, Write, Exec
	if (!_external_command_fd && mkfifo(filename, S_IRWXU) != 0)
	{
		if (errno != EEXIST)
		{
			PRINT_ERROR("Could not create named pipe '%s'!\n", filename);
			return;
		}
	}

	// Open the fd
	if (!_external_command_fd && (_external_command_fd = open(filename, O_RDONLY)) < 0)
	{
		PRINT_ERROR("Could not open fd to named pipe '%s'!\n", filename);
		return;
	}

	PRINT_INFO("Opened named pipe '%s' for reading...\n", filename);
	PRINT_INFO("Waiting for Input... Process will be blocked!\n");

	// Get cmd
	if (read(_external_command_fd, &_external_command, sizeof(struct XTIER_external_command)) !=
	    sizeof(struct XTIER_external_command))
	{
		PRINT_ERROR("An error occurred while receiving the command struct. Aborting...\n");
		return;
	}

	PRINT_DEBUG("Received command structure...\n");

	// Output redirection?
	if (_external_command.redirect != NONE)
	{
		// Get the redirection struct
		if (read(_external_command_fd, &_external_command_redirect,
				sizeof(struct XTIER_external_command_redirect)) !=
			    sizeof(struct XTIER_external_command_redirect))
		{
			PRINT_ERROR("An error occurred while receiving the redirect struct. Aborting...\n");
			return;
		}

		PRINT_DEBUG("Received redirect structure...\n");

		// Make sure the steam is NULL
		_external_command_redirect.stream = NULL;

		// Try to open file
		// This must be done here to ensure that the BEGIN marker is sent!

		// Is this a pipe redirection?
		if (_external_command_redirect.type == PIPE)
		{
			// Check if we must open the file
			if (!_external_command_redirect.stream)
			{
				_external_command_redirect.stream = fopen(_external_command_redirect.filename, "w");

				if (_external_command_redirect.stream <= 0)
				{
					PRINT_ERROR("Could not open file '%s'\n", _external_command_redirect.filename);
					return;
				}

				// We just opened the file, so we send the data begin header
				fprintf(_external_command_redirect.stream, "" XTIER_EXTERNAL_OUTPUT_BEGIN);
			}
		}
	}

	// Get the command itself
	if (_external_command.type == INJECTION)
	{
		// Injection command
		// Free old injection structure if there is any
		if (_injection)
		{
			// Save old path and code
			prev_module_name = malloc(_injection->path_len);
			prev_module_code = _injection->code;
			prev_module_code_len = _injection->code_len;

			if (prev_module_name)
				strcpy(prev_module_name, _injection->module_path);
			else
			{
				PRINT_ERROR("Could not reserve memory!\n");
				return;
			}

			free_injection_without_code(_injection);
		}


		// Make sure the OS is set
		if(_XTIER.os == XTIER_OS_UNKNOWN)
		{
			_XTIER.os = XTIER_OS_UBUNTU_64;
			XTIER_ioctl(XTIER_IOCTL_SET_GLOBAL_XTIER_STATE, &_XTIER);
		}

		// Get the injection structure
		_injection = injection_from_fd(_external_command_fd);

		// Read in data if required
		if (_injection->code_len == 0 && (!prev_module_name || strcmp(_injection->module_path, prev_module_name)))
		{
			// free prev data
			if (prev_module_code)
				free(prev_module_code);

			injection_load_code(_injection);
		}
		else if(prev_module_name && !strcmp(_injection->module_path, prev_module_name))
		{
			PRINT_DEBUG("Reusing existing code!\n");
			_injection->code_len = prev_module_code_len;
			_injection->code = prev_module_code;
		}

		// Free old name if any
		if (prev_module_name)
			free(prev_module_name);

		// Inject
		PRINT_DEBUG("Injecting file %s which consists of %d bytes...\n", _injection->module_path, _injection->code_len);
		ret =  XTIER_ioctl(XTIER_IOCTL_INJECT, _injection);

		if(ret < 0)
			PRINT_ERROR("An error occurred while injecting the file!\n");

		// Synchronize new cpu_state
		_env->kvm_vcpu_dirty = 0; // Force the sync
		XTIER_synchronize_state(_env);

		// fprintf(stderr, "RIP now: 0x%llx\n", _env->eip);
	}
	else
	{
		// Unknown command
		PRINT_ERROR("Unkown command type (%d)\n", _external_command.type);
		return;
	}

	// Close fd and remove pipe
	//close(fd);
	//remove(filename);
}

void XTIER_command_inject(const char *cmdline)
{
	// Check if the OS has been set yet.
	if(_XTIER.os == XTIER_OS_UNKNOWN)
	{
		// Nope. We need to do that first.
		PRINT_OUTPUT("\nSo far no OS has been specified. Please do so now.\n");
		_current_question = _find_top_level_question(XTIER_QUESTION_OS);
		XTIER_ask_current_question();

		// Return to us afterwards
		_return_to = &XTIER_command_inject;

		return;
	}

	_current_question = _find_top_level_question(XTIER_QUESTION_INJECT_GET_FILE);

	XTIER_ask_current_question();
}

static void XTIER_ns_to_time(u64 ns, struct XTIER_time *time)
{
	if(!time)
		return;

	time->sec = ns / NSEC_PER_SEC;
	ns = ns - (time->sec * NSEC_PER_SEC);
	time->ms = ns / NSEC_PER_MSEC;
	time->ns = ns % NSEC_PER_MSEC;

	return;
}

/*
 * Returns the return value of the injection.
 */
static long XTIER_print_injection_performance(void)
{
	int ret;
	struct XTIER_time t;
	struct XTIER_performance perf;

	// Get performacne data
	ret = XTIER_ioctl(XTIER_IOCTL_INJECT_GET_PERFORMANCE, &perf);

	if(ret < 0)
	{
		PRINT_ERROR("An error occurred while obtaining the performance data of the injection!\n");
		return ret;
	}

	// Print statistics
	PRINT_OUTPUT("\n\nInjection Statistics:\n");
	PRINT_OUTPUT("\t | File: '%s'\n", _injection->module_path);
	PRINT_OUTPUT("\t | Injections: %u\n", perf.injections);
	PRINT_OUTPUT("\t | Temp Removals/Resumes: %u\n", perf.temp_removals);
	PRINT_OUTPUT("\t | Hypercalls: %u\n", perf.hypercalls);

	if(perf.injections)
	{
		XTIER_ns_to_time(perf.total_module_load_time / perf.injections, &t);
		PRINT_OUTPUT("\t | Average Load Time: %llu s %llu ms %llu ns\n", t.sec, t.ms, t.ns);
		XTIER_ns_to_time(perf.total_module_exec_time / perf.injections, &t);
		PRINT_OUTPUT("\t | Average Exec Time: %llu s %llu ms %llu ns\n", t.sec, t.ms, t.ns);
		XTIER_ns_to_time(perf.total_module_unload_time / perf.injections, &t);
		PRINT_OUTPUT("\t | Average Unload Time: %llu s %llu ms %llu ns\n", t.sec, t.ms, t.ns);

		PRINT_OUTPUT("\t |\n");

		if(perf.hypercalls)
		{
			XTIER_ns_to_time(perf.total_module_hypercall_time / perf.hypercalls, &t);
			PRINT_OUTPUT("\t | Average Hypercall Time: %llu s %llu ms %llu ns\n", t.sec, t.ms, t.ns);

			XTIER_ns_to_time(perf.total_module_hypercall_time / perf.injections, &t);
			PRINT_OUTPUT("\t | Average Hypercall Time per Injection: %llu s %llu ms %llu ns\n", t.sec, t.ms, t.ns);

			XTIER_ns_to_time(perf.total_module_hypercall_time, &t);
			PRINT_OUTPUT("\t | Total Hypercall Time: %llu s %llu ms %llu ns\n", t.sec, t.ms, t.ns);

			PRINT_OUTPUT("\t |\n");
		}

		if(perf.temp_removals)
		{
			XTIER_ns_to_time(perf.total_module_temp_removal_time / perf.temp_removals, &t);
			PRINT_OUTPUT("\t | Average Temp Removal Time: %llu s %llu ms %llu ns\n", t.sec, t.ms, t.ns);

			XTIER_ns_to_time(perf.total_module_temp_resume_time / perf.temp_removals, &t);
			PRINT_OUTPUT("\t | Average Temp Resume Time: %llu s %llu ms %llu ns\n", t.sec, t.ms, t.ns);

			XTIER_ns_to_time((perf.total_module_temp_removal_time + perf.total_module_temp_resume_time) / perf.temp_removals, &t);
			PRINT_OUTPUT("\t | Average Temp Removal/Resume Time: %llu s %llu ms %llu ns\n", t.sec, t.ms, t.ns);

			XTIER_ns_to_time((perf.total_module_temp_removal_time + perf.total_module_temp_resume_time) / perf.injections, &t);
			PRINT_OUTPUT("\t | Average Temp Removal/Resume Time per Injection: %llu s %llu ms %llu ns\n", t.sec, t.ms, t.ns);

			XTIER_ns_to_time((perf.total_module_temp_removal_time + perf.total_module_temp_resume_time), &t);
			PRINT_OUTPUT("\t | Total Temp Removal/Resume Time: %llu s %llu ms %llu ns\n", t.sec, t.ms, t.ns);

			PRINT_OUTPUT("\t |\n");
		}

		XTIER_ns_to_time((perf.total_module_load_time +
							perf.total_module_exec_time +
							perf.total_module_unload_time)
							/ perf.injections, &t);
		PRINT_OUTPUT("\t | Average Total Time: %llu s %llu ms %llu ns\n", t.sec, t.ms, t.ns);

		if(perf.temp_removals)
		{
			XTIER_ns_to_time((perf.total_module_exec_time -
											(perf.total_module_temp_removal_time + perf.total_module_temp_resume_time))
											/ perf.injections, &t);
			PRINT_OUTPUT("\t | Average Exec Time w/o TEMP Removal/Resume: %llu s %llu ms %llu ns\n", t.sec, t.ms, t.ns);
		}

		if(perf.hypercalls)
		{
			XTIER_ns_to_time((perf.total_module_exec_time - perf.total_module_hypercall_time) / perf.injections, &t);
			PRINT_OUTPUT("\t | Average Exec Time w/o Hypercalls: %llu s %llu ms %llu ns\n", t.sec, t.ms, t.ns);
		}

		if(perf.temp_removals && perf.hypercalls)
		{
			XTIER_ns_to_time((perf.total_module_exec_time -
								(perf.total_module_temp_removal_time + perf.total_module_temp_resume_time + perf.total_module_hypercall_time))
								/ perf.injections, &t);
			PRINT_OUTPUT("\t | Average Exec Time w/o TEMP R/R & Hypercalls: %llu s %llu ms %llu ns\n", t.sec, t.ms, t.ns);
		}

		XTIER_ns_to_time((perf.total_module_load_time +
							perf.total_module_exec_time +
							perf.total_module_unload_time), &t);
		PRINT_OUTPUT("\t | Total Time: %llu s %llu ms %llu ns\n", t.sec, t.ms, t.ns);
	}

	PRINT_OUTPUT("\t ___________________________________\n\n");

	return perf.return_value;
}

static void XTIER_handle_injection_finished(void)
{
	// Print performance data
	long return_value = XTIER_print_injection_performance();

	PRINT_OUTPUT("Injection Finished (Return value %ld)!\n", return_value);

	// Notify waiting applications if any
	if (_external_command_redirect.type != NONE &&
		_external_command_redirect.stream)
	{
		// Send Return Value
		XTIER_external_command_send_return_value(_external_command_redirect.stream, return_value);

		// Send end Notifier
		fprintf(_external_command_redirect.stream, "" XTIER_EXTERNAL_OUTPUT_END);

		// Close and reset
		fclose(_external_command_redirect.stream);

		_external_command_redirect.type = NONE;
		_external_command_redirect.stream = NULL;
	}

	// Sync
	XTIER_synchronize_state(_env);
}

static void XTIER_handle_injection_fault(void)
{
	PRINT_ERROR("An exception occurred during the injection that could not be handled!\n");

	// Sync
	XTIER_synchronize_state(_env);
}

void XTIER_command_event_based_inject(const char *cmdline)
{
	// Check if the OS has been set yet.
	if(_XTIER.os == XTIER_OS_UNKNOWN)
	{
		// Nope. We need to do that first.
		PRINT_OUTPUT("\nSo far no OS has been specified. Please do so now.\n");
		_current_question = _find_top_level_question(XTIER_QUESTION_OS);
		XTIER_ask_current_question();

		// Return to us afterwards
		_return_to = &XTIER_command_event_based_inject;

		return;
	}

	if(_event_injection == 1)
	{
		PRINT_OUTPUT("\nDisabling event injection...\n");

		XTIER_ioctl(XTIER_IOCTL_INJECT, 0);

		_event_injection = 0;

		// Print performance data if event-injection was disabled
		XTIER_print_injection_performance();
	}
	else
	{
		PRINT_OUTPUT("\nEnabling event injection...\n");

		_current_question = _find_top_level_question(XTIER_QUESTION_EVENT_INJECT_SELECT_MODULE);

		XTIER_ask_current_question();
	}
}

void XTIER_command_time_inject(const char *cmdline)
{
	u32 choice;

	// Was a value given?
	if(!cmdline)
	{
		PRINT_ERROR("Please specify the period of time after the a module will be reinjected.\n");
		return;
	}

	// Get the choice
	choice = (u32)atoi(cmdline);

	if(!choice)
		PRINT_OUTPUT("Timed-Injection will be disabled!\n");
	else
		PRINT_OUTPUT("Timed-Injection will be set to %u seconds...\n", choice);

	_time_injection = choice;
	XTIER_ioctl(XTIER_IOCTL_INJECT_SET_TIME_INJECT, (void *)(u64)choice);

	// Print performance
	if(!_time_injection)
	{
		// Print performance data if timed-injection was disabled
		XTIER_print_injection_performance();
	}
}

void XTIER_command_auto_inject(const char *cmdline)
{
	u32 choice;

	// Was a value given?
	if(!cmdline)
	{
		PRINT_ERROR("Please specify the number of times the module should be auto-injected.\n");
		return;
	}

	// Get the choice
	choice = (u32)atoi(cmdline);

	if(!choice)
		PRINT_OUTPUT("Auto-Injection will be disabled!\n");
	else
		PRINT_OUTPUT("Auto-Injection will be set to %u...\n", choice);

	_auto_injection = choice;
	XTIER_ioctl(XTIER_IOCTL_INJECT_SET_AUTO_INJECT, (void *)(u64)choice);
}

/*
 * Get the file that will be injected.
 */
int XTIER_question_inject_get_file(const char *cmdline)
{
	int ret = 0;

	// Free old injection structure if there is any
	if (_injection)
		free_injection(_injection);

	// Create new injection structure
	_injection = new_injection(cmdline);

	if (!_injection)
	{
		PRINT_ERROR("An error occurred while creating the injection structure!\n");
		return ret;
	}

	// Load code
	injection_load_code(_injection);

	if (!_injection->code)
	{
		PRINT_ERROR("An error occurred while loading the module code!\n");
		return ret;
	}

	// Set params
	_injection->event_based = 0;
	_injection->exit_after_injection = 1;
	_injection->event_address = 0;

	// Args
	// add_int_argument(_injection, 0x1337);
	// add_string_argument(_injection, "Awesome this works!");
	// consolidate_args(_injection);


	// We could include timed injection here such that it is exclusive from
	// auto injection.
	if(_auto_injection)
	{
		_injection->auto_inject = _auto_injection;
	}
	else
	{
		// Exactly inject once
		_injection->auto_inject = 1;
	}

	_injection->time_inject = _time_injection;

	// Inject
	PRINT_DEBUG("Injecting file %s which consists of %d bytes...\n", _injection->module_path, _injection->code_len);
	ret = XTIER_ioctl(XTIER_IOCTL_INJECT, _injection);

	if(ret < 0)
		PRINT_ERROR("An error (%d) occurred while injecting the file!\n", ret);

	// Synchronize new cpu_state
	_env->kvm_vcpu_dirty = 0; // Force the sync
	XTIER_synchronize_state(_env);

	// fprintf(stderr, "RIP now: 0x%llx\n", _env->eip);

	return ret;
}

int XTIER_question_event_inject_select_module(const char *cmdline)
{
	int choice;
	int ret = 0;

	// Get the choice
	choice = atoi(cmdline);

	// Free old injection structure if there is any
	if (_injection)
		free_injection(_injection);

	if(choice == XTIER_INJECT_EVENT_MODULE_ACE)
	{
		// Set params
		// Create new injection structure
		_injection = new_injection("/tmp/linux/ace.inject");

		if (!_injection)
		{
			PRINT_ERROR("An error occurred while creating the injection structure!\n");
			return ret;
		}

		if(_XTIER.os == XTIER_OS_UBUNTU_64)
		{
			// Open system call:   system call table address + (nr_sys_open (2) * 8)
			// inject.event_address = 0xffffffff816002e0 + (2 * 8);
			_injection->event_address = (void *)0xffffffff81164830;
		}
		else
		{
			PRINT_ERROR("Target OS unsupported or not set!\n");
			return 0;
		}
	}
	else if(choice == XTIER_INJECT_EVENT_MODULE_VIRUSTOTAL)
	{
		// Set params
		_injection = new_injection("/tmp/linux/virus.inject");

		if (!_injection)
		{
			PRINT_ERROR("An error occurred while creating the injection structure!\n");
			return ret;
		}

		if(_XTIER.os == XTIER_OS_UBUNTU_64)
		{
			// Open system call:   system call table address + (nr_sys_open (2) * 8)
			// inject.event_address = 0xffffffff816002e0 + (2 * 8);
			// inject.event_address = 0xffffffff81164830;
			_injection->event_address = (void *)0xffffffff81015140;
		}
		else
		{
			PRINT_ERROR("Target OS unsupported or not set!\n");
			return 0;
		}
	}
	else
	{
		PRINT_ERROR("Unknown Choice!\n");
		return ret;
	}

	// Get File
	injection_load_code(_injection);

	if (!_injection->code)
	{
		PRINT_ERROR("An error occurred while loading the module code!\n");
		return ret;
	}

	// Set params
	_injection->event_based = 1;
	_injection->exit_after_injection = 0;
	_injection->auto_inject = 0;
	_injection->time_inject = 0;

	// Args
	_injection->args = 0;
	_injection->args_size = 0;
	_injection->size_last_arg = 0;

	// Inject
	PRINT_DEBUG("Injecting even based file %s which consists of %d bytes...\n", _injection->module_path, _injection->code_len);
	ret =  XTIER_ioctl(XTIER_IOCTL_INJECT, _injection);

	if(ret < 0)
		PRINT_ERROR("An error occurred while injecting the file!\n");

	// Enable event injection
	_event_injection = 1;

	// Synchronize new cpu_state
	_env->kvm_vcpu_dirty = 0; // Force the sync
	XTIER_synchronize_state(_env);

	// fprintf(stderr, "RIP now: 0x%llx\n", _env->eip);

	return ret;
}

int XTIER_question_specify_os(const char *cmdline)
{
	int choice;
	int ret;

	choice = atoi(cmdline);

	if(choice == XTIER_OS_UBUNTU_64)
	{
		PRINT_OUTPUT("\t-> OS will be set to 'Ubuntu 64-bit'\n");
	}
	else if(choice == XTIER_OS_WINDOWS_7_32)
	{
		PRINT_OUTPUT("\t-> OS will be set to 'Windows 7 32-bit'\n");
	}
	else if(choice == XTIER_OS_UBUNTU_32)
	{
		PRINT_OUTPUT("\t-> OS will be set to 'Ubuntu 32-bit'\n");
	}
	else
	{
		PRINT_OUTPUT("Unknown OS specified!\n");
		return -1;
	}

	_XTIER.os = choice;

	ret = XTIER_ioctl(XTIER_IOCTL_SET_GLOBAL_XTIER_STATE, &_XTIER);

	// Return to caller
	if(_return_to)
		_return_to(cmdline);

	return ret;
}

/*
 * Handle kvm exits due to XTIER.
 */
void XTIER_handle_exit(CPUState *env, u64 exit_reason)
{
	PRINT_DEBUG("Handling EXIT: %lld...\n", exit_reason);

	switch(exit_reason)
	{
		case XTIER_EXIT_REASON_INJECT_FINISHED:
			XTIER_handle_injection_finished();
			XTIER_switch_to_XTIER_mode(env);
			PRINT_OUTPUT(XTIER_PROMPT);
			return;
		case XTIER_EXIT_REASON_INJECT_FAULT:
			XTIER_handle_injection_fault();
			XTIER_switch_to_XTIER_mode(env);
			PRINT_OUTPUT(XTIER_PROMPT);
			return;
		case XTIER_EXIT_REASON_INJECT_COMMAND:
			// sync state
			XTIER_synchronize_state(env);

			// handle
			XTIER_inject_handle_interrupt(env, &_external_command_redirect);
			return;
		case XTIER_EXIT_REASON_DEBUG:
			PRINT_INFO("Debug exit requested.\n");
			XTIER_switch_to_XTIER_mode(env);
			PRINT_OUTPUT(XTIER_PROMPT);
			return;
		default:
			PRINT_ERROR("Unknown exit reason!\n");
	}
}
