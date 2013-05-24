/*
 * X-TIER_qemu.h
 *
 *  Created on: Nov 14, 2011
 *      Author: Sebastian Vogl <vogls@sec.in.tum.de>
 */

#ifndef XTIER_QEMU_H_
#define XTIER_QEMU_H_

#include "../include/qemu-common.h"
#include "../include/monitor/monitor.h"

#include "../../X-TIER/X-TIER.h"

// PROMPT
#define XTIER_PROMPT "(X-TIER >> "

extern struct XTIER_state _XTIER;

/**
 * Parse the current user input to detect X-TIER commands.
 */
void XTIER_handle_input(Monitor *mon, const char *cmdline, void *opaque);

/**
 * Handle X-TIER related VM Exits.
 */
void XTIER_handle_exit(CPUState *env, u64 exit_reason);

/**
 * Switch to the X-TIER shell.
 */
void XTIER_switch_to_XTIER_mode(CPUState *env);

/**
 * Wrapper for ioctl commands.
 *
 * @param command The number of the command to send to the kernel module.
 * @param arg A pointer to the argument that should be provided to the
 *            kernel module with the command.
 */
int XTIER_ioctl(unsigned int command, void *arg);

// X-TIER COMMAND HANDLERS
// THESE SHOULD NOT BE CALLED! THEY ONLY EXIT FOR INPUT HANDLING.
/**
 * Switch to the monitor, but keep the VM paused.
 */
void XTIER_switch_to_monitor_mode_keep_paused(const char *cmdline);

/**
 * Switch to the monitor and continue the execution of the VM.
 */
void XTIER_switch_to_monitor_mode(const char *cmdline);

/**
 * Print all X-TIER commands.
 */
void XTIER_print_help(const char *cmdline);

/**
 * Inject a module.
 */
void XTIER_command_inject(const char *cmdline);

/**
 * Select an event-based module for the injection.
 */
void XTIER_command_event_based_inject(const char *cmdline);

/**
 * Set the auto-injection for the current module.
 */
void XTIER_command_auto_inject(const char *cmdline);

/**
 * Set timed injection for the current module.
 */
void XTIER_command_time_inject(const char *cmdline);

/**
 * Receive an external command.
 */
void XTIER_command_receive_external_command(const char *cmdline);

// QUESTIONS
// THESE SHOULD NOT BE CALLED! THEY ONLY EXIT FOR INPUT HANDLING.
/**
 * Print the current question.
 */
void XTIER_ask_current_question(void);

/**
 * Ask the user to specify an OS.
 */
int XTIER_question_specify_os(const char *cmdline);

/**
 * Ask the user which file should be injected.
 */
int XTIER_question_inject_get_file(const char *cmdline);

/**
 * Ask the user to select an event-based module for injection.
 */
int XTIER_question_event_inject_select_module(const char *cmdline);

#endif /* XTIER_QEMU_H */
