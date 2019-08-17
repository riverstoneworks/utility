/*
 * semaphore.h
 *
 *  Created on: May 15, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */

#ifndef SCHED_SEMAPHORE_ENHC_H_
#define SCHED_SEMAPHORE_ENHC_H_

#include<stdbool.h>

typedef struct{
	void * data;
	void* (*callback)(void*);
	unsigned long req;
	void * next;
}Waiter;

typedef struct{
	void * data;
	void (*wakeUp)(Waiter*,void*);
}Awakener;

typedef struct _sem_sched_ut Sem_sched_ut;
struct _sem_sched_ut{
	struct _sem const * d;
	struct _op_sem_sched_ut{
		bool (*read)(Sem_sched_ut);
		bool (*write)(Sem_sched_ut);
		bool (*wait)(Sem_sched_ut,Waiter);
		int (* destory)(Sem_sched_ut*);
		unsigned long (*showVal)(Sem_sched_ut);
	}const * o;
};

extern Sem_sched_ut newSem(unsigned long init,unsigned long max, Awakener aw);

#endif /* SCHED_SEMAPHORE_ENHC_H_ */
