/*
 * flyweight.c
 *
 *  Created on: Feb 22, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */


#include "../../mem/flyweight/flyweight.h"

#include <stdlib.h>
#include<stdatomic.h>

static inline int push_from_end(ElementPool* pool,Element* h,Element* e){
	e->next=NULL;
	Element* p=(Element*)atomic_exchange(&(pool->end),(intptr_t)e);
	p->next=h;
	return 0;
}

static inline int push_from_head(ElementPool* pool,Element* h, Element* e){
	e->next=NULL;
	Element* p=(Element*)atomic_exchange(&(pool->head),(intptr_t)h);
	e->next=p;
	return 0;
}

//return number of elements left
unsigned poolDec(ElementPool* pool){
	//get lock
	Block* b = (Block*)pool->blocks;
	if (b < 0)
		return -1;
	else if (!atomic_compare_exchange_strong(&(pool->blocks), (intptr_t*)(&b), -1))
		return -1;

	if (pool->n_blocks < 2)
		return -2;

	Element* h,*t, *p;
	while(1){
		p=(Element*)pool->head;
		if(p&&p->next){
			if(atomic_compare_exchange_strong(&(pool->head),(intptr_t*)(&p),(intptr_t)(p->next))){
				p->next=NULL;
				h=(Element*)atomic_exchange(&(pool->head),(intptr_t)p);
				break;
			}else
				continue;
		}else{
			pool->blocks=(intptr_t)b;
			return 1;
		}
	}

	t=h;
	for(int i=2;t&&i++<pool->n_auto_inc;t=t->next);

	struct _bc {
		Element* h;
		Element* e;
		unsigned num;
	}*bc;
	if(t&&t->next&&(bc=calloc(pool->n_blocks+1,sizeof(struct _bc)))){
		Element* e=t->next;
		t->next=NULL;
		t=(Element*)atomic_exchange(&(pool->end),(intptr_t)t);

		p->next = h; //connect front and back

		while (e) {
			int i = 1;
			for (Block* blk = (Block*) b; blk; blk = blk->next) {
				if (e >= blk->eles && e <= (blk->eles + blk->n_eles - 1)) {
					bc[i].num++;
					if (bc[i].h)
						bc[i].e = bc[i].e->next = e;
					else {
						bc[i].e = bc[i].h = e;
					}
					break;
				}
				i++;
			}
			while (!e->next && e != t)
				;
			e = e->next;
		}

		Block _fb={.next=b},*fb=&_fb;

		for (int i = 1, j = pool->n_blocks; i <= j; i++) {

			if (bc[i].num == b->n_eles) {
				fb->next = (b->next);
				free(b->data);
				free(b->eles);
				free(b);
				pool->n_blocks--;
			} else {
				fb = fb->next;
				if (bc[i].num) {
					if (bc[0].h) {
						bc[0].e->next = bc[i].h;
						bc[0].e = bc[i].e;
					} else {
						bc[0].h = bc[i].h;
						bc[0].e = bc[i].e;
					}
					bc[0].num += bc[i].num;
				}
			}
			b = fb->next;
		}

		if(bc[0].h)
			push_from_end(pool,bc[0].h,bc[0].e);

		unsigned left=bc[0].num-1+pool->n_auto_inc;
		free(bc);

		pool->blocks=(intptr_t)_fb.next; //unlock

		return left;
	}else{
		p->next = h; //connect front and back
		pool->blocks = (intptr_t)b; //unlock

		return 0;
	}

}

int poolInc(ElementPool* pool,unsigned n_eles,size_t s_ele){
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
	blk->eles=malloc(n_eles*sizeof(Element));
	blk->data=malloc(n_eles*s_ele);

	if(!blk||!blk->eles||!blk->data){
		free(blk->data);
		free(blk->eles);
		free(blk);
		return -3;
	}

	int i = -1;
	while (++i < (n_eles - 1)) {
		(blk->eles + i)->data = blk->data + i * s_ele;
		(blk->eles + i)->next = blk->eles + i + 1;
	}
	(blk->eles + i)->data = blk->data + i * s_ele;
	(blk->eles + i)->next = NULL;

	blk->n_eles = n_eles;
	blk->s_ele = s_ele;
	blk->next = (Block*)b;
	b = atomic_exchange(&(pool->end), (intptr_t )(blk->eles + n_eles - 1));
	if (b)
		((Element*) b)->next = blk->eles;

	pool->n_blocks++;
	pool->blocks = (intptr_t)blk;	//unlock

	return 0;
}


ElementPool* newPool(size_t size_element,unsigned n_element,unsigned n_auto_inc,unsigned short n_max_blocks){
	ElementPool* epl=calloc(1,sizeof(ElementPool));
	if (epl) {
		epl->n_max_blocks = n_max_blocks;
		epl->n_auto_inc = n_auto_inc;

		if(poolInc(epl,n_element,size_element))
			free(epl);
		else{
			epl->head=(intptr_t)((Block*)epl->blocks)->eles;
			return epl;
		}
	}

	return NULL;
}

Element * eleAlloc(ElementPool* pool){

	Element* e;
	while(1){
		e=(Element*)pool->head;
		if(e&&e->next){
			if(atomic_compare_exchange_strong(&(pool->head),(intptr_t*)(&e),(intptr_t)(e->next))){
				e->next=NULL;
				return e;
			}else
				continue;
		}else if(pool->head==pool->end){
			if(-1>poolInc(pool,pool->n_auto_inc,((Block*)pool->blocks)->s_ele))
				return NULL;
		}
	}

}

int eleRec(ElementPool* pool, Element* e){
	const Block* b=(Block*)pool->blocks;
	const int half=pool->n_blocks/2;
	int i=b>0?0:half;
	while(i<half){
		if(e>=b->eles&&e<=(b->eles+(b->n_eles-1)))
			break;
		else
			b=b->next;
		++i;
	}
	(i<half?push_from_end:push_from_head)(pool,e,e);
	return 0;
}

int destoryPool(ElementPool* pool){
	Block* b=(Block*)pool->blocks,*t;
	if(b<0)
		return -1;
	else if(!atomic_compare_exchange_strong(&(pool->blocks),(intptr_t*)(&b),-1))
		return -1;

	while(b){
		free(b->eles);
		free(b->data);
		t=b->next;
		free(b);
		b=t;
	}
	free(pool);

	return 0;
}
