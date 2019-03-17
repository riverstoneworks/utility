/*
 * flyweight.c
 *
 *  Created on: Feb 22, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>

#include "utility/mem/flyweight.h"
#define ElementPool ut_fw_ElementPool

typedef struct _BLK{
	struct _BLK *next;
	intptr_t* eles;			//size = data+next_pointer
	size_t s_ele;		//each element size
	unsigned int n_eles;
}Block;

struct _pool{
	volatile atomic_intptr_t left; // @suppress("Type cannot be resolved")
	volatile atomic_intptr_t blocks; // @suppress("Type cannot be resolved")
	unsigned int n_auto_inc;
	unsigned short n_blocks;
	unsigned short n_max_blocks;
};

static inline void push(volatile atomic_intptr_t* const top,intptr_t * const h, intptr_t * const e){ // @suppress("Type cannot be resolved")
	do{
		*e=(*top);
	}while(!atomic_compare_exchange_strong(top,e,*h));
}

static inline void* pop(volatile atomic_intptr_t* const top){ // @suppress("Type cannot be resolved")
	intptr_t e;
	do{
		if(!(e=*top))
			return NULL;
	}while(!atomic_compare_exchange_strong(top,&e,e));
	return (void*)e;
}

//return number of Elements left
unsigned poolDec(ElementPool* epl){
	struct _pool* pool=epl->d;
	//get lock
	Block* b = (Block*)pool->blocks;
	if (b < 0)
		return -1;
	else if (!atomic_compare_exchange_strong(&(pool->blocks), (intptr_t*)(&b), -1))
		return -1;

	if (pool->n_blocks < 2)
		return -2;

	struct _bc {
		Block* addr_blk;
		intptr_t* addrs;
		intptr_t* addrf;
		intptr_t* h;
		intptr_t* e;
		unsigned cap;
		unsigned num;
	}*bc,bcN={0};

	if((bc=calloc(pool->n_blocks,sizeof(struct _bc)))){
		Block* bb=b;
		for(int i=0;i<pool->n_blocks;++i){
			bc[i].addr_blk=bb;
			bc[i].addrs=bb->eles;
			bc[i].addrf=bb->eles+bb->s_ele*bb->n_eles;
			bc[i].cap=bb->n_eles;
			bb=bb->next;
		}

		intptr_t* e;
		while((e=pop(&pool->left))){
			for(int i=0;i<pool->n_blocks;++i){
				if(e>=bc[i].addrs&&e<bc[i].addrf){
					*e=(intptr_t)bc[i].h;
					bc[i].h=e;
					bc[i].num++;
					if(!bc[i].e)
						bc[i].e=bc[i].h;
					break;
				}
			}
		}


		b=bb=NULL;
		for(int i=0;i<pool->n_blocks;++i){
			if(bc[i].cap==bc[i].num){
				bc[i].addr_blk->next=bb;
				bb=bc[i].addr_blk;
			}else{
				bc[i].addr_blk->next=b;
				b=bc[i].addr_blk;

				if(bc[i].num){
					bcN.num+=bc[i].num;
					*bc[i].e=(intptr_t)bcN.h;
					bcN.h=bc[i].h;
					if(!bcN.e)
						bcN.e=bc[i].e;
				}
			}
		}

		free(bc);

		while(bb){
			if(bcN.num>=pool->n_auto_inc){
				free(bb->eles);
				Block * t=bb;
				bb=bb->next;
				free(t);
			}else{
				*(bb->eles+(bb->n_eles-1) * bb->s_ele)=(intptr_t)bcN.h;
				bcN.h=bb->eles;
				bcN.num+=bb->n_eles;

				if(!bcN.e)
					bcN.e=bb->eles+ (bb->n_eles-1)* bb->s_ele;

				Block *t=bb;
				bb=bb->next;
				t->next=b;
				b=t;
			}
		}

		if(bcN.num>0)
			push(&pool->left,bcN.h,bcN.e);

		pool->blocks=(intptr_t)b; //unlock

		return bcN.num;
	}else{
		pool->blocks = (intptr_t)b; //unlock

		return 0;
	}

}

int poolInc(ElementPool* epl,unsigned n_eles,size_t s_ele){
	struct _pool* pool=epl->d;
	//get lock
	intptr_t b=pool->blocks;
	if(b<0)
		return -1;
	else if(!atomic_compare_exchange_strong(&(pool->blocks),&b,-1))
		return -1;

	if(pool->n_max_blocks<=pool->n_blocks){
		pool->blocks=b;	//unlock
		return -2;
	}

	Block* blk=malloc(sizeof(Block));
	if(!blk)
		return -3;

	blk->n_eles = n_eles;
	blk->s_ele = s_ele+sizeof(void*);
	if(!(blk->eles=malloc(n_eles*(blk->s_ele)))){
		free(blk);
		return -3;
	}

	int i = -1;
	while (++i < (n_eles - 1)) {
		*((void**)(blk->eles + i*(blk->s_ele))) = (blk->eles + (i+1)*(blk->s_ele));
	}
	*((void**)(blk->eles + i*(blk->s_ele)))  = NULL;


	blk->next = (Block*)b;

	push(&pool->left, blk->eles, blk->eles + (n_eles - 1)*blk->s_ele);

	pool->n_blocks++;
	pool->blocks = (intptr_t)blk;	//unlock

	return 0;
}


void * eleAlloc(ElementPool* epl){
	struct _pool* pool=epl->d;

	void* e;
	while(1){
		if((e=pop(&pool->left))){
				void* r=e+sizeof(void*);
				return r;
		}else if(-1>poolInc(epl,pool->n_auto_inc,((Block*)pool->blocks)->s_ele-sizeof(void*)))
			return NULL;
	}
}

void eleRec(ElementPool* epl,  const void * const d){
	struct _pool* pool=epl->d;

	intptr_t* e=(intptr_t*)(d-sizeof(void*));
	push(&(pool->left),e,e);
}

int destoryPool(ElementPool* epl){
	struct _pool* pool=epl->d;
	Block* b=(Block*)pool->blocks,*t;
	if(b<0)
		return -1;
	else if(!atomic_compare_exchange_strong(&(pool->blocks),(intptr_t*)(&b),-1))
		return -1;

	while(b){
		free(b->eles);
		t=b->next;
		free(b);
		b=t;
	}
	free(pool);

	return 0;
}

void showInfo(ut_fw_ElementPool* epl){
	struct _pool* pool=epl->d;
	printf("blocks: %u\n"
		"max_blocks: %u\n"
		"auto_inc: %u\n"
		"head: %p\n"
		"block: %p\n"
		"n_element: %d\n"
		"s_element: %ld\n"
		"ele: %p\n"
		"next_block: %p\n\n",
		pool->n_blocks,
		pool->n_max_blocks,
		pool->n_auto_inc,
		(void*)pool->left,
		(void*)pool->blocks,
		((Block*)pool->blocks)->n_eles,
		((Block*)pool->blocks)->s_ele,
		((Block*)pool->blocks)->eles,
		((Block*)pool->blocks)->next);
}

ut_fw_ElementPool* newPool(size_t size_Element,unsigned n_Element,unsigned n_auto_inc,unsigned short n_max_blocks){
	ElementPool* epl=calloc(1,sizeof(ElementPool));
	struct _pool* pool=epl->d;
	if (epl) {
		pool->n_max_blocks = n_max_blocks;
		pool->n_auto_inc = n_auto_inc;

		if(poolInc(epl,n_Element,size_Element))
			free(epl);
		else{
			static struct _ut_fw_ElementPool_op OP={
					.destoryPool=destoryPool,
					.eleAlloc=eleAlloc,
					.eleRec=eleRec,
					.poolDec=poolDec,
					.poolInc=poolInc,
					.showInfo=showInfo
			};
			*((struct _ut_fw_ElementPool_op**)(&epl->op))=&OP;
			return epl;
		}
	}

	return NULL;
}

#undef ElementPool
