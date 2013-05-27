/*
 * X-TIER_inject.c
 *
 *  Created on: May 21, 2013
 *      Author: Sebastian Vogl <vogls@sec.in.tum.de>
 */

#ifndef XTIER_DEBUG_H_
#define XTIER_DEBUG_H_

/*
 * DEBUGGING
 */
#define DEBUG_FULL 6
#define DEBUG 5
#define INFO 4
#define WARNING 3
#define ERROR 2
#define CRITICAL 1

#define DEBUG_LEVEL 5

#define msg_print(level, level_string, fmt, ...) \
        do { if (DEBUG_LEVEL >= level) printk("[ X-TIER - %s ] %d : %s(): " fmt, \
                                              level_string, __LINE__, __func__, ##__VA_ARGS__); \
           } while (0)

#define PRINT_DEBUG_FULL(fmt, ...) msg_print(DEBUG_FULL, "DEBUG", fmt, ##__VA_ARGS__)
#define PRINT_DEBUG(fmt, ...) msg_print(DEBUG, "DEBUG", fmt, ##__VA_ARGS__)
#define PRINT_INFO(fmt, ...) msg_print(INFO, "INFO", fmt, ##__VA_ARGS__)
#define PRINT_WARNING(fmt, ...) msg_print(WARNING, "WARNING", fmt, ##__VA_ARGS__)
#define PRINT_ERROR(fmt, ...) msg_print(ERROR, "ERROR", fmt, ##__VA_ARGS__)

#endif
