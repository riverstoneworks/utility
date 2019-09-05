/*
 * semaphore_enhc.c
 *
 *  Created on: May 12, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */

#include <string.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <utility/sched/semaphore_enhc.h>
#include <utility/mem/flyweight.h>

struct _sem{
		volatile atomic_ulong s; // @suppress("Type cannot be resolved")
		unsigned long const max;
		Awakener awakener;
		volatile atomic_intptr_t waiting_to_read; // @suppress("Type cannot be resolved")
		volatile atomic_intptr_t waiting_to_write; // @suppress("Type cannot be resolved")
};

static unsigned long wakeup(struct _sem* sem){
	Waiter *q=NULL,*w;
	unsigned long max=sem->max,s,n=0;
	while((w=(void*)sem->waiting_to_read)){
		if(atomic_compare_exchange_weak(&(sem->waiting_to_read),(intptr_t*)&w,(intptr_t)w->next)){
			if(!w->req){
				s=atomic_exchange(&sem->s,0);
				if(s){
					w->next=q;
					q=w;
					++n;
					continue;
				}
			}else{
				while((s=sem->s)>=w->req){
					if(atomic_compare_exchange_weak(&sem->s,&s,s-w->req))
						break;
				}
				if(s>=w->req){
					w->next = q;
					q = w;
					++n;
					continue;
				}
			}

			do{
				w->next=(void*)sem->waiting_to_read;
			}while(!atomic_compare_exchange_weak(&(sem->waiting_to_read),(intptr_t*)&w->next,(intptr_t)w));

			break;
		}
	}

	while((w=(void*)sem->waiting_to_write)){
		if(atomic_compare_exchange_weak(&(sem->waiting_to_write),(intptr_t*)&w,(intptr_t)w->next)){
			if(!w->req){
				s=atomic_exchange(&sem->s,max);
				if(s<max){
					w->next=q;
					q=w;
					++n;
					continue;
				}
			}else{
				while((s=sem->s)<=max-w->req){
					if(atomic_compare_exchange_weak(&sem->s,&s,s+w->req))
						break;
				}
				if(s<=max-w->req){
					w->next = q;
					q = w;
					++n;
					continue;
				}
			}

			do{
				w->next=(void*)sem->waiting_to_write;
			}while(!atomic_compare_exchange_weak(&(sem->waiting_to_write),(intptr_t*)&w->next,(intptr_t)w));

			break;
		}
	}

	if(q)
		sem->awakener.wakeUp(q,sem->awakener.data);
	return n;
}

unsigned long showVal(struct _sem *sem){
	return sem->s;
}

static inline void sem(struct _sem **sem){
	static ut_fw_ElementPool pool={NULL,NULL};
	if(!pool.d&&
			!(pool=newPool(sizeof(struct _sem),8,8,8,NULL)).d)
		return;

	if(*sem){
		pool.o->eleRec(*sem,pool);
		*sem=NULL;
	}else
		(*sem)=pool.o->eleAlloc(pool);

}

static void waiter(Waiter** waiter){
	static ut_fw_ElementPool pool={NULL,NULL};
	if(!pool.d&&
			!(pool=newPool(sizeof(Waiter),8,8,8,NULL)).d)
		return;

	if(*waiter){
		pool.o->eleRec(*waiter,pool);
		*waiter=NULL;
	}else
		*waiter=pool.o->eleAlloc(pool);
}

static int read_num_wait(struct _sem *sem,unsigned long dec,Waiter wa){
	unsigned long s;
	while((s=sem->s)>0){
		if(atomic_compare_exchange_weak(&(sem->s),&s,s-dec)){
			wakeup(sem);
			return 0;
		}
	}
	if(wa.callback){
		Waiter* w=NULL;
		waiter(&w);
		if(w){
			memcpy(w,&wa,sizeof(wa));
			do{
				w->next=(Waiter*)sem->waiting_to_read;
			}while(!atomic_compare_exchange_weak(&(sem->waiting_to_read),(intptr_t*)&w->next,(intptr_t)w));
		}else
			return -1;
	}

	return 1;
}

static int write_num_wait(struct _sem *sem,unsigned long inc,Waiter wa){
	unsigned long s;
	while((s=sem->s)<=sem->max-inc){
		if(atomic_compare_exchange_weak(&(sem->s),&s,s+inc)){
			wakeup(sem);
			return 0;
		}
	}
	if(wa.callback){
		Waiter* w=NULL;
		waiter(&w);
		if(w){
			memcpy(w,&wa,sizeof(wa));
			do{
				w->next=(Waiter*)sem->waiting_to_write;
			}while(!atomic_compare_exchange_weak(&(sem->waiting_to_write),(intptr_t*)&w->next,(intptr_t)w));
		}else
			return -1;
	}
	return 1;
}

static bool read(struct _sem *sem){
	return read_num_wait(sem,1,(Waiter){0,0});
}

static bool write(struct _sem *sem){
	return write_num_wait(sem,1,(Waiter){0,0});
}

static bool wait(struct _sem *sem, Waiter w){
 return false;
}

static void default_wakeup(Waiter* w,void* d){
	Waiter* t;
	while(w){
		w->callback(w->data);
		t = w;
		w = w->next;
		waiter(&t);
	}
}

static void destory(struct _sem** s){
	if(*s)
		sem(s);
}

Sem_sched_ut newSem(unsigned long init,unsigned long max, Awakener aw){
	static struct _op_sem_sched_ut const OP = {
			.read =	(bool (*)(Sem_sched_ut)) read,
			.write = (bool (*)(Sem_sched_ut)) write,
			.wait = (bool (*)(Sem_sched_ut,Waiter)) wait,
			.destory = (int (*)(Sem_sched_ut*)) destory,
			.showVal = (unsigned long (*)(Sem_sched_ut)) showVal
	};
	struct _sem* s=NULL;
	sem(&s);
	if(s){
		s->awakener.data=aw.data;
		s->awakener.wakeUp=aw.wakeUp?aw.wakeUp:default_wakeup;
		*((unsigned long*)&(s->max))=max;
		s->s=init;
		s->waiting_to_read=s->waiting_to_write=0;
	}
	return (Sem_sched_ut){
		.d=s,
		.o=&OP
	};
}
