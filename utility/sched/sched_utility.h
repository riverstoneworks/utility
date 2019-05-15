/*
 * schd_utility.h
 *
 *  Created on: Dec 19, 2018
 *      Author: Like.Z(sxpc722@aliyun.com)
 */

#ifndef SCHED_UTILITY_H_
#define SCHED_UTILITY_H_
#include <threads.h>
typedef struct {
	thrd_t thd;
	unsigned short *cpu_ids;
	unsigned short num;
}thd_cpu_affinity_conf;

extern int set_thd_cpu_affinity(thd_cpu_affinity_conf* tcac,int num);

#endif /* SCHED_UTILITY_H_ */
