/*
 * schd_utility.c
 *
 *  Created on: Dec 19, 2018
 *      Author: Like.Z(sxpc722@aliyun.com)
 */

#include "../sched/sched_utility.h"

#include <stdio.h>
#ifndef __USE_GNU
	#define __USE_GNU
#endif
#include <pthread.h>
int set_thd_cpu_affinity(thd_cpu_affinity_conf* tcac,int num){
	int e=0;
	cpu_set_t cs;
	while(num-->0){
		CPU_ZERO_S(sizeof(cpu_set_t),&cs);
		int j=tcac[num].num;
		while(j-->0)
			CPU_SET_S(tcac[num].cpu_ids[j],sizeof(cs),&cs);
		if(pthread_setaffinity_np(tcac[num].thd,sizeof(cs),&cs)<0){
			perror("sched_getaffinity");
			--e;
		}
	}
	return e;
}
