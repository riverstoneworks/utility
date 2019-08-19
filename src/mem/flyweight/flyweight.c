/*
 * flyweight.c
 *
 *  Created on: Feb 22, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>

#include <utility/mem/flyweight.h>
#define ElementPool ut_fw_ElementPool
#define ele_addr_ind(pool,blk,ind) (((intptr_t)blk->eles)+pool->s_ele*(ind))

typedef enum { DISUSED=-2,LOCKED, UNLOCKED } pool_stat;

typedef struct _BLK{
	struct _BLK *next;
	intptr_t eles;			//size = data+next_pointer
//	size_t s_ele;		//each element size
//	unsigned int n_eles;
}Block;

struct _pool{
	volatile atomic_intptr_t blocks; // @suppress("Type cannot be resolved")
	volatile atomic_intptr_t left; // @suppress("Type cannot be resolved")
	volatile atomic_intptr_t left_end; // @suppress("Type cannot be resolved")
	const size_t s_ele;
	const unsigned int n_init;
	const unsigned int n_auto_inc;
//	volatile atomic_uint n_left; // @suppress("Type cannot be resolved")

	unsigned short n_blocks; // @suppress("Type cannot be resolved")
	const unsigned short n_max_blocks;
	intptr_t es;
	intptr_t ee;
	volatile atomic_char stat; // @suppress("Type cannot be resolved")
};



static inline void* pop(struct _pool* pool){ // @suppress("Type cannot be resolved")
	intptr_t e=pool->left;
	while(1){
		if(e==pool->left_end)
			return NULL;
		else if(!*(intptr_t*)e)
			e=pool->left;
		else if(atomic_compare_exchange_week(&pool->left,&e,*((intptr_t*)e)))
			return e;
	}
}

//return number of Elements left
static long poolDec(struct _pool* pool){
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
	*f=NULL;
	fe=ff=atomic_exchange(&pool->left,f);

	int i;
	for(i=0;fe!=pool->left_end&&i<pool->n_auto_inc/2;fe=*fe){
		if(!*fe)
			continue;
		else
			++i;
	}

	while(fe!=pool->left_end&&!*fe);
	e=*fe;
	*fe=NULL;
	fe=atomic_exchange(&pool->left_end,fe);
	*f=ff;

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
		unsigned num;
	}*bc;


	if((bc=calloc(n_blk,sizeof(struct _bc)))){
		struct _bc *bcN=bc+n_blk-1;

		//init bc[]
		Block* fb=b;
		for(int i=0;i<n_blk-1;++i){
			bc[i].addr_blk=fb;
			bc[i].addrs=fb->eles;
			bc[i].addrf=(intptr_t)fb->eles+pool->s_ele*pool->n_auto_inc;
			fb=fb->next;
		}

		bcN->addr_blk=fb;
		bcN->addrs=fb->eles;
		bcN->addrf=(intptr_t)fb->eles+pool->s_ele*pool->n_init;


		//statistics
		while(e){
			if(e!=fe&&!*e)
				continue;
			for(int i=0;i<n_blk;++i){
				if(e>=(intptr_t*)bc[i].addrs&&e<(intptr_t*)bc[i].addrf){
					*e=(intptr_t)bc[i].h;
					bc[i].h=e;
					bc[i].num++;
					if(!bc[i].e)
						bc[i].e=bc[i].h;
					break;
				}
			}
			e=*e;
		}

		//sort: full(fb) or not(b)
		fb=NULL;
		b=bcN->addr_blk;
		for(int j=n_blk-1,i=j-1;i>-1;--i){
//			printf("%d:%d ; ",bc[i].cap,bc[i].num);
			if(pool->n_auto_inc==bc[i].num){
				bc[i].addr_blk->next=fb;
				fb=bc[i].addr_blk;
			}else{
				bc[i].addr_blk->next=b;
				b=bc[i].addr_blk;

				if(bc[i].num){
					bcN->num+=bc[i].num;
					*bcN->e=(intptr_t)bc[i].h;
					bcN->e=bc[i].e;
					if(!bcN->h)
						bcN->h=bc[i].h;
				}
			}
		}

		if(bcN->num>0)
			*(intptr_t*)atomic_exchange(&pool->left_end,(intptr_t)bcN->e)=(bcN->h);

		n_blk=bcN->num;
		free(bc);
		//free needless blocks
		for (Block* tb=fb;fb;tb=fb=fb->next) {
			free(tb->eles);
			free(tb);
			--(pool->n_blocks);
		}

		pool->es=(intptr_t)((Block*)pool->blocks)->eles;
		pool->ee=pool->es+((pool->n_blocks>1?pool->n_auto_inc:pool->n_init)-1)*pool->s_ele;

		pool->blocks=(intptr_t)b; //unlock

		return n_blk;
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

	Block* blk=malloc(sizeof(Block));
	if(!blk)
		return -3;

	if(!(blk->eles=malloc(n_eles*s_ele))){
		free(blk);
		return -3;
	}

	intptr_t i=(intptr_t)blk->eles , e=(intptr_t)blk->eles+s_ele*(n_eles-1);
	while(i<e){
		*((intptr_t*)i)=i+s_ele;
		i+=s_ele;
	}
	*((intptr_t*)i)=(intptr_t)NULL;


	blk->next = (Block*) b;
	b = (intptr_t) blk;

	// end block;
	pool->es=(intptr_t)blk->eles;
	pool->ee=pool->es+(s_ele-1)*n_eles;

	*((intptr_t*)atomic_exchange(&pool->left_end,i))=(intptr_t)blk->eles;

	pool->n_blocks++;
	pool->blocks = b;	//unlock

	return 0;
}


static void * eleAlloc(struct _pool* pool){
	void* e;
	while(1){
		if((e=pop(pool))){
			void* r = e;
			return r;
		}else if(-1>poolInc(pool,pool->n_auto_inc,pool->s_ele))
			return NULL;
	}
}

static void eleRec(struct _pool* pool,  const void * const d){
	intptr_t e=(intptr_t)d;
	if(e>=pool->es&&e<=pool->ee){
		*((intptr_t*)e)=0;
		*((intptr_t*)atomic_exchange(&pool->left_end,e))=e;
	}else
		*(intptr_t*)e=atomic_exchange(&(pool->left),e);

}

static int destory(struct _pool** pool){
	Block* b=(Block*)(*pool)->blocks,*t;
	if(b<0)
		return -1;
	else if(!atomic_compare_exchange_strong(&((*pool)->blocks),(intptr_t*)(&b),LOCKED))
		return -1;

	while(b){
		free((void*)b->eles);
		t=b->next;
		free(b);
		b=t;
	}
	free(*pool);
	*pool=NULL;
	return 0;
}

static ut_fw_PoolsDaemon daemon={NULL,NULL};

static int destoryWithAutoMaintain(struct _pool* pool){
//	pool->blocks

	return 0;
}

static void showInfo(struct _pool* pool){
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
		pool->n_init,
		pool->s_ele,
		((Block*)pool->blocks)->eles,
		((Block*)pool->blocks)->next);
}

ut_fw_ElementPool newPool(size_t size_Element,unsigned n_Element,unsigned n_auto_inc,unsigned short n_max_blocks){
	static struct _ut_fw_ElementPool_op OP={
			.destory=(int (* const)(ut_fw_ElementPool*))destory,
			.eleAlloc=(void* (* const )(ut_fw_ElementPool))eleAlloc,
			.eleRec=(void (*const)(ut_fw_ElementPool,void const * const))eleRec,
			.showInfo=(void (* const)(ut_fw_ElementPool))showInfo
	};
	struct _pool* pool=calloc(1,sizeof(struct _pool));
	if (pool) {
		*((unsigned short*)&(pool->n_max_blocks)) = n_max_blocks;
		*((unsigned int*)&(pool->n_auto_inc)) = n_auto_inc;
		*((size_t*)&(pool->s_ele))=size_Element<sizeof(void*)?sizeof(void*):size_Element;
		*((unsigned int*)&(pool->n_init))=n_Element;
		if(poolInc(pool,n_Element,pool->s_ele))
			free(pool);
		pool->stat=UNLOCKED;
	}

	return (ut_fw_ElementPool){
		.d=pool,
		.o=&OP
	};
}
//
//struct _daemon{
//	struct _pp{
//		volatile struct _pool* pool;
//		struct _pp* next;
//	}*PQ;
//};
//
//
//static void destoryDaemon(ut_fw_PoolsDaemon* daemon){
//	free(daemon->d);
//	*daemon=(ut_fw_PoolsDaemon){NULL,NULL};
//}
//
//static ut_fw_ElementPool pp={NULL,NULL};
//static int append(ut_fw_PoolsDaemon* daemon, struct _pool* pool){
//	if(pp.d||(pp=newPool(sizeof(struct _pp),32,32,8))){
//		struct _pp* t=pp.o->eleAlloc(&pp);
//		if(t){
//			t->pool = pool;
//			t->next = daemon->d->PQ;
//			daemon->d->PQ = t;
//			return 0;
//		}
//	}
//	return -1;
//}
//
//static int remove(struct _daemon* daemon, struct _pool* pool){
//	struct _pp* p=daemon->PQ;
//	while(p&&pool!=p->pool)
//		p=p->next;
//	if(p)
//		p->pool=NULL;
//	return 0;
//}
//
//static int gc(struct _daemon* daemon){
//	struct _pp* p=daemon->PQ;
//	while(p){
//		struct _pool* pl=p->pool;
//		if(pl)
//			poolDec(pl);
//		else
//	}
//}
//ut_fw_PoolsDaemon newPoolsDaemon(){
//	static struct _ut_fw_PoolsDaemon_op OP={
//			.destory=destoryDaemon,
//			.append=append,
//			.remove=remove,
//			.gc=NULL
//	};
//	return (ut_fw_PoolsDaemon){
//		.d=calloc(1,sizeof(struct _pp*)),
//		.o=&OP
//	};
//}
//
//ut_fw_ElementPool newPoolWithAutoMaintan(size_t size_Element,unsigned n_Element,unsigned n_auto_inc,unsigned short n_max_blocks){
//
//	if(daemon.d||(daemon=newPoolsDaemon()).d){
//		ut_fw_ElementPool np=newPool(size_Element,n_Element,n_auto_inc,n_max_blocks);
//		if(np.d)
//			if(daemon.o->append(&daemon,np.d))
//				np.o->destory(&np);
//			else
//				return np;
//	}
//
//	return (ut_fw_ElementPool){NULL,NULL};
//}

#undef ElementPool
