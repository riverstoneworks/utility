/*
 * flyweight_auto.c
 *
 *  Created on: Aug 23, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>
#include <stdbool.h>
#include<utility/mem/flyweight.h>
#include<utility/mem/flyweight_auto.h>
#define EF ((void*)-1)
#define LCK ((void*)-2)
struct _auto{
	volatile atomic_intptr_t h; // @suppress("Type cannot be resolved")
	volatile atomic_char stat; // @suppress("Type cannot be resolved")
	struct _ut_fw_ElementPool_op const* const op;
};
struct _pool{
	struct _pool* next;
	volatile atomic_char stat; // @suppress("Type cannot be resolved")
};

bool isMaintained(ut_fw_ElementPool pool){
	if(((struct _pool*)pool.d)->next)
		return true;
	else
		return false;
}

void append (ut_fw_ElementPool pool,struct _auto *at){
	if(isMaintained(pool))
		return;
	else{
		while(!atomic_compare_exchange_weak(&at->h,
				(intptr_t*)(&((struct _pool*)pool.d)->next),
				(intptr_t)pool.d));
	}
}

void destory(struct _auto** at){
	(*at)->h;

	free(*at);
	*at=NULL;
}

int maintain(struct _auto* at){
	struct _pool **pp=(struct _pool**)&(at->h),*p=*pp,*t;
	while(p!=EF){
		if(p->stat==DISUSED){
			t=p;
			if(atomic_compare_exchange_strong(&at->h,(intptr_t*)(&t),(intptr_t)(p->next))){
				at->op->destory((ut_fw_ElementPool*)&p);
				p=(*pp)->next;
			}else{
				while(*(pp=&((*pp)->next))!=p);
				break;
			}
		}else{
			at->op->poolDec((ut_fw_ElementPool){p});
			pp=&(p->next);
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
	return 0;
}
