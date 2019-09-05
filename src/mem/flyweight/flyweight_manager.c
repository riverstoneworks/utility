/*
 * flyweight_auto.c
 *
 *  Created on: Aug 23, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdatomic.h>
#include <threads.h>
#include <utility/mem/flyweight.h>
#define EF (void*)(-1)
enum {DISUSED=-2};

struct _manager{
	volatile atomic_intptr_t h; // @suppress("Type cannot be resolved")
	volatile atomic_char stat; // @suppress("Type cannot be resolved")
	struct _ut_fw_ElementPool_op const* op;
};
struct _pool{
	struct _pool* next;
	volatile char stat; // @suppress("Type cannot be resolved")
};

static bool isManaged(ut_fw_ElementPool pool){
	if(((struct _pool*)pool.d)->next)
		return true;
	else
		return false;
}

static int append (ut_fw_ElementPool pool,struct _manager *at){
	if(!pool.d||!at)
		return -1;
	else if(isManaged(pool))
		return 0;
	else{
		do{
			if(atomic_compare_exchange_weak(&at->h,
							(intptr_t*)(&pool.d->next),
							(intptr_t)pool.d)){
				return 0;
			}
		}while(pool.d->next!=(struct _pool*)DISUSED);
		((struct _pool*)pool.d)->next=NULL;
		return -1;
	}
}

//all pools in auto-maintain will been destoryed
static void destory(struct _manager** at){
	struct _manager *a=*at;
	if(!a)
		return;
	else
		*at=NULL;
	char flag=SLEEP;
	while(!atomic_compare_exchange_weak(&a->stat,&flag,DISUSED)){
		if (flag == RUNNING){
			thrd_yield();
			flag=SLEEP;
		} else
			return;
	}

	while(a->stat!=STOP)
		thrd_yield();

	struct _pool *t, *p=(struct _pool*)atomic_exchange(&a->h,DISUSED);
	while(p!=EF){
		p=(t=p)->next;
		a->op->destory((ut_fw_ElementPool*)(&t));
	}

	free(a);
}

static manager_stat maintain(struct _manager* at){
	char flag=SLEEP;
	if(!at)
		return ERROR;
	else if(!atomic_compare_exchange_strong(&at->stat,&flag,RUNNING)){
		if(flag==DISUSED)
			at->stat=STOP;
		return at->stat;
	}
	struct _pool **pp=(struct _pool**)&(at->h),*p,*t;
	while((p=*pp)!=EF){
		if(p->stat==DISUSED){
			t=p;
			at->op->destory((ut_fw_ElementPool*)&p);
			if(!atomic_compare_exchange_strong(&at->h,(intptr_t*)(&t),(intptr_t)(t->next))){
				pp=&(*pp)->next;
				break;
			}
		}else{
			at->op->poolDec((ut_fw_ElementPool){p});
			pp=&(p->next);
			break;
		}
	}
	while(*pp!=EF){
		if((*pp)->stat==DISUSED){
			(*pp)=(p=*pp)->next;
			at->op->destory((ut_fw_ElementPool*)&p);
		}else{
			at->op->poolDec((ut_fw_ElementPool){*pp});
			pp=&((*pp)->next);
		}
	}
	at->stat=SLEEP;
	return at->stat;
}

static int running(struct _manager* m){
	while (1) {
		switch (maintain(m)) {
		case ERROR:
			printf("ERROR\n");
			/* no break */
		case STOP:
			break;
		case SLEEP:
			sleep(1);
			/* no break */
		default:
			continue;
		}
		break;
	}
	return 0;
}
static int launch_default(ut_fw_manager m){
	thrd_t m_tid;
	int r;
	((r=thrd_create(&m_tid,(int(*)(void*))&running,(void*)m.d))==thrd_success)&&(r=thrd_detach(m_tid));
	return r==thrd_success?0:-1;
}

ut_fw_manager newFwManager(launch lau) {
	static struct _ut_fw_auto_op OP = {
			.append = (int (* const )(ut_fw_ElementPool,ut_fw_manager)) append,
			.maintain =	(manager_stat (* const )(ut_fw_manager)) maintain,
			.destory = (void (* const )(ut_fw_manager*)) destory };

	struct _manager *a = malloc(sizeof(struct _manager));
	if (a) {
		a->h = (intptr_t) EF;
		a->stat = SLEEP;
		a->op = newPool(0, 0, 0, 0, 0).o;
		ut_fw_manager m={ .d = a, .o = &OP };
		if (lau ? lau(m) : launch_default(m)) {
			free(a);
			a = NULL;
		}
	}

	return (ut_fw_manager ) { .d = a, .o = &OP } ;
}

#undef EF
