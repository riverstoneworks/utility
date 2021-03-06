/*
 * flyweight.c
 *
 *  Created on: Feb 22, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>
#include <threads.h>
#include <utility/mem/flyweight.h>
#define ElementPool ut_fw_ElementPool
enum { DISUSED=-2,LOCKED, FREE };

typedef struct _BLK{
	struct _BLK *next;
	intptr_t eles;			//size = data+next_pointer
}Block;

struct _pool{
	struct _pool* next;
	volatile char stat; // @suppress("Type cannot be resolved")
	volatile atomic_intptr_t blocks; // @suppress("Type cannot be resolved")
	volatile atomic_intptr_t left; // @suppress("Type cannot be resolved")
	volatile atomic_intptr_t left_end; // @suppress("Type cannot be resolved")

	const size_t s_ele;
	const unsigned int n_init;
	const unsigned int n_auto_inc;

	unsigned short n_blocks; // @suppress("Type cannot be resolved")
	const unsigned short n_max_blocks;
	intptr_t es;
	intptr_t ee;
};



static inline void* pop(struct _pool* pool){ // @suppress("Type cannot be resolved")
	intptr_t e=pool->left;
	while(1){
		if(e==pool->left_end)
			return NULL;
		else if(!*(intptr_t*)e)
			e=pool->left;
		else if(atomic_compare_exchange_weak(&(pool->left), &e, *((intptr_t*)e))){
			return (void*)e;
		}
	}
}

//return number of Elements left
static int poolDec(struct _pool* pool){
	//get lock
	Block* b = (Block*)pool->blocks;
	if (b < 0)
		return -1;
	else if (!atomic_compare_exchange_strong(&(pool->blocks), (intptr_t*)(&b), LOCKED))
		return -1;

	int n_blk=pool->n_blocks;
	if (n_blk < 2){
		pool->blocks = (intptr_t)b;
		return -2;
	}

	//left enough
	intptr_t *f=pop(pool),*fe,*ff,*e;
	if(!f){
		pool->blocks = (intptr_t)b;
		return 0;
	}
	*f=0;
	fe=ff=(intptr_t*)atomic_exchange(&pool->left,(intptr_t)f);

	int in=1;
	for(;fe!=(intptr_t*)(pool->left_end)&&in<pool->n_auto_inc/2;fe=(intptr_t*)*fe){
		if(!*fe)
			continue;
		else
			++in;
	}
	for(e=fe;e!=(intptr_t*)(pool->left_end)&&in<(pool->n_auto_inc/2+pool->n_auto_inc);e=(intptr_t*)*e){
		if(!*e)
			continue;
		else
			++in;
	}
	if(in<(pool->n_auto_inc/2+pool->n_auto_inc)){
		*f=(intptr_t)ff;
		pool->blocks = (intptr_t)b;
		return 0;
	}

	while((intptr_t)fe!=pool->left_end&&!*fe);
	e=*(intptr_t**)fe;
	*fe=0;
	fe=(intptr_t*)atomic_exchange(&pool->left_end,(intptr_t)fe);
	*f=(intptr_t)ff;

	if(!e){
		pool->blocks = (intptr_t)b;
		return 0;
	}

	struct _bc {
		Block* addr_blk;
		intptr_t addrs;
		intptr_t addrf;
		intptr_t* h;
		intptr_t* e;
		size_t num;
	}*bc;


	if((bc=calloc(n_blk,sizeof(struct _bc)))){
		struct _bc *bcN=bc+n_blk-1;

		//init bc[]
		Block* fb=b;
		for(int i=0;i<n_blk-1;++i){
			bc[i].addr_blk=fb;
			bc[i].addrs=(intptr_t)&fb->eles;
			bc[i].addrf=(intptr_t)&fb->eles+pool->s_ele*pool->n_auto_inc;
			fb=fb->next;
		}

		bcN->addr_blk=fb;
		bcN->addrs=(intptr_t)&fb->eles;
		bcN->addrf=(intptr_t)&fb->eles+pool->s_ele*(pool->n_init+1);


		//statistics
		while(e){
			if(e!=fe&&!*e)
				continue;

			e=*(intptr_t**)(f=e);
			for(int i=0;i<n_blk;++i){
				if(f>=(intptr_t*)bc[i].addrs&&f<(intptr_t*)bc[i].addrf){
					bc[i].num++;
					*f=(intptr_t)bc[i].h;
					bc[i].h=f;
					if(!bc[i].e)
						bc[i].e=f;
					break;
				}
			}

		}

		//sort: full(fb) or not(b)
		fb=NULL;
		b=bcN->addr_blk;
		for(int i=n_blk-2;i>-1;--i){
//			printf("%d:%d ; ",bc[i].cap,bc[i].num);
			if(pool->n_auto_inc==bc[i].num){
				bc[i].addr_blk->next=fb;
				fb=bc[i].addr_blk;
			}else{
				bc[i].addr_blk->next=b;
				b=bc[i].addr_blk;

				if(bc[i].num){
					if(bcN->num){
						*bcN->e=(intptr_t)bc[i].h;
						bcN->e=bc[i].e;
					}else
						bcN=bc+i;
				}
			}
		}

		//take back the element cannot be released
		if(bcN->h)
			*(intptr_t**)atomic_exchange(&pool->left_end,(intptr_t)bcN->e)=(bcN->h);

		free(bc);
		//free needless blocks
		int n=0;
		for (Block* tb=fb;tb;tb=fb) {
			fb=fb->next;
			free(tb);
			++n;
		}

		pool->es=(intptr_t)&b->eles;
		pool->ee=pool->es+(pool->n_blocks>1?pool->n_auto_inc:(pool->n_init-1))*pool->s_ele;

		pool->blocks=(intptr_t)b; //unlock
		pool->n_blocks-=n;
		return n;
	}else{
		pool->blocks = (intptr_t)b; //unlock
		//error: lack of memory
		return -3;
	}

}

static int poolInc(struct _pool* pool,unsigned n_eles,size_t s_ele){
	//get lock
	intptr_t b=pool->blocks;
	if(b<0)
		return -1;
	else if(!atomic_compare_exchange_strong(&(pool->blocks),&b,LOCKED))
		return -1;

	if(pool->n_max_blocks<=pool->n_blocks){
		pool->blocks=b;	//unlock
		return -2;
	}

	Block* blk=malloc(n_eles*s_ele+sizeof(intptr_t));
	if(!blk){
		pool->blocks=b;
		return -3;
	}else{
		// end block;
		intptr_t i=pool->es=(intptr_t)(&blk->eles);
		pool->ee=i+s_ele*(n_eles-1);
		while(i<pool->ee){
			*((intptr_t*)i)=i+s_ele;
			i+=s_ele;
		}
		*((intptr_t*)i)=(intptr_t)NULL;

		*((intptr_t*)atomic_exchange(&pool->left_end,i))=pool->es;

		blk->next = (Block*) b;
		b = (intptr_t) blk;

		pool->n_blocks++;
		pool->blocks = b;	//unlock
	}

	return 0;
}


static void * eleAlloc(struct _pool* pool){
	void* e;
	while(1){
		if((e=pop(pool))){
			return e;
		}else if(-1>poolInc(pool,pool->n_auto_inc,pool->s_ele))
			return NULL;
	}
}

static void eleRec( const void * const d, struct _pool* pool){
	intptr_t e=(intptr_t)d;
	*((intptr_t*)e)=0;
	if(e>=pool->es&&e<=pool->ee){
		*((intptr_t*)atomic_exchange(&pool->left_end,e))=e;
	}else
		*(intptr_t*)e=atomic_exchange(&(pool->left),e);
}

//free mem immediately!
static int destory(struct _pool** pool){
	if((*pool)->next){
		(*pool)->stat=DISUSED;
		return 1;
	}
	Block* b=(Block*)(*pool)->blocks,*t;
	if(b<0)
		return -1;
	else if(!atomic_compare_exchange_strong(&((*pool)->blocks),(intptr_t*)(&b),LOCKED))
		return -1;

	while(b){
		t=b->next;
		free(b);
		b=t;
	}
	free(*pool);
	*pool=NULL;
	return 0;
}

static void showInfo(struct _pool* pool){
	printf("blocks num:		%u\n"
		"max num of blocks:	%u\n"
		"init ele num:		%d\n"
		"ele auto inc num:	%u\n"
		"head:			%p\n"
		"end:			%p\n"
		"element size:		%ld\n"
		"first block:		%p\n"
		"next block:		%p\n\n",
		pool->n_blocks,
		pool->n_max_blocks,
		pool->n_init,
		pool->n_auto_inc,
		(void*)pool->left,
		(void*)pool->left_end,
		pool->s_ele,
		((Block*)pool->blocks),
		((Block*)pool->blocks)->next);
}

//grobal auto manager
static ut_fw_manager atm={0,0};

int init_default_atm(launch l){
	if(atm.d){
		return 1;
	}if((atm=newFwManager(l)).d)
		return 0;
	else
		return -1;
}
void destory_global_atm(){
	if(atm.d)
		atm.o->destory(&atm);
}
ut_fw_ElementPool newPool(size_t size_Element,unsigned n_Element,unsigned n_auto_inc,unsigned short n_max_blocks, bool auto_manage){
	static struct _ut_fw_ElementPool_op OP={
			.destory=(int (* const)(ut_fw_ElementPool*))destory,
			.eleAlloc=(void* (* const )(ut_fw_ElementPool))eleAlloc,
			.eleRec=(void (*const)(void const * const, ut_fw_ElementPool))eleRec,
			.poolDec=(int (*const)(ut_fw_ElementPool))poolDec,
			.showInfo=(void (* const)(ut_fw_ElementPool))showInfo
	};

	struct _pool* pool;
	if (size_Element*n_Element&&(pool=calloc(1,sizeof(struct _pool)))) {
		*((unsigned short*)&(pool->n_max_blocks)) = n_max_blocks;
		*((unsigned int*)&(pool->n_auto_inc)) = n_auto_inc;
		//the element minsize is sizeof(void*) for next ptr.
		*((size_t*)&(pool->s_ele))=size_Element<sizeof(intptr_t)?sizeof(intptr_t):size_Element;
		*((unsigned int*)&(pool->n_init))=n_Element;

		Block* xp;

		if ((xp = malloc(pool->s_ele * (n_Element+1)+sizeof(intptr_t)))) {
			pool->blocks = (intptr_t) xp;
			xp->next = NULL;
			pool->left = pool->es = pool->ee = (intptr_t) &(xp->eles);

			while (pool->ee < pool->es + n_Element * pool->s_ele) {
				*(intptr_t*) (pool->ee) = pool->ee + pool->s_ele;
				pool->ee += pool->s_ele;
			}

			*(intptr_t*) (pool->ee) = 0;
			pool->left_end = pool->ee;
			pool->n_blocks = 1;

			pool->stat = FREE;

			if(auto_manage){
				if(!atm.d){
					atm=newFwManager(NULL);
				}
				if(!atm.d||atm.o->append((ut_fw_ElementPool){pool},atm)){
					free (xp);
					free (pool);
					pool=NULL;
				}
			}
			return (ut_fw_ElementPool ) { .d = pool, .o = &OP } ;
		}
		free(pool);
	}

	return (ut_fw_ElementPool){
		.d=NULL,
		.o=&OP
	};
}

#undef ElementPool
