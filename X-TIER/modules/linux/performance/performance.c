#include <linux/time.h>
#include "performance.h"

#define NSEC_PER_SEC 1000000000L
#define NSEC_PER_MSEC 1000000L

struct timespec _starttime;
struct timespec _endtime;

void start_timing(void)
{
	getnstimeofday(&_starttime);
}

u64 end_timing(void)
{
	getnstimeofday(&_endtime);

	return (timespec_to_ns(&_endtime) - timespec_to_ns(&_starttime));
}

void print_time(u64 time, unsigned int executions)
{
	u64 sec, asec = 0;
	u64 ms, ams = 0;
	u64 ns, ans = 0;
	u64 temp_time = time;

	sec = temp_time / NSEC_PER_SEC;
	temp_time = temp_time - (sec * NSEC_PER_SEC);
	ms = temp_time / NSEC_PER_MSEC;
	ns = temp_time % NSEC_PER_MSEC;

	temp_time = time / executions;
	asec = temp_time / NSEC_PER_SEC;
        temp_time = temp_time - (asec * NSEC_PER_SEC);
        ams = temp_time / NSEC_PER_MSEC;
        ans = temp_time % NSEC_PER_MSEC;

	printk("Average Execution Time: %llu s %llu ms %llu ns\n", asec, ams, ans);
	printk("Total Execution Time: %llu s %llu ms %llu ns\n", sec, ms, ns);
}
