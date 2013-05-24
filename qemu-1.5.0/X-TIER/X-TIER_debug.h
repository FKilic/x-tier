#ifndef XTIER_DEBUG_H_
#define XTIER_DEBUG_H_

/*
 * DEBUGGING
 */
#define DEBUG 5
#define INFO 4
#define WARNING 3
#define ERROR 2
#define CRITICAL 1

#define DEBUG_LEVEL 4


#define msg_print(level, level_string, fmt, ...) \
        do { if (DEBUG_LEVEL >= level) monitor_printf(default_mon, "[ X-TIER - %s ] %d : %s(): " fmt, \
                                			level_string, __LINE__, __func__, ##__VA_ARGS__); } while (0)


#define PRINT_DEBUG(fmt, ...) msg_print(DEBUG, "DEBUG", fmt, ##__VA_ARGS__)
#define PRINT_INFO(fmt, ...) msg_print(INFO, "INFO", fmt, ##__VA_ARGS__)
#define PRINT_WARNING(fmt, ...) msg_print(WARNING, "WARNING", fmt, ##__VA_ARGS__)
#define PRINT_ERROR(fmt, ...) msg_print(ERROR, "ERROR", fmt, ##__VA_ARGS__)


/*
 * Monitor output
 */
#define PRINT_OUTPUT(fmt, ...) do {\
                                    monitor_printf(default_mon, "" fmt, ##__VA_ARGS__);\
	                                monitor_flush(default_mon);\
                                  } while(0)

#endif /* TRACING_DEBUG_H */
