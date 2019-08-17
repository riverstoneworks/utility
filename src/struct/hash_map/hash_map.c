/*
 * hash_map.c
 *
 *  Created on: Mar 2, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */

#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

#include <utility/struct/hash_map.h>
#define HashMap ut_hm_HashMap

typedef struct _Entry{
	void* obj;
	char* key;
	size_t keySize;
	struct _Entry* next;
}Entry;

struct _HashMap{
	struct _hashMapData{
		volatile atomic_intptr_t* list; // @suppress("Type cannot be resolved")
		Entry* const entrys;
		volatile atomic_intptr_t head; // @suppress("Type cannot be resolved")
		volatile atomic_uint length; // @suppress("Type cannot be resolved")
		const unsigned int cap;
	}* hmd;
	const struct _ut_hm_hashmap_op* op;
};


static void empty(HashMap* hm){
	struct _hashMapData* hmd=hm->hmd;

	for(int i=0;i<hmd->cap;++i){
		intptr_t e=hmd->list[i];
		while(e==-1||!atomic_compare_exchange_strong(hmd->list+i,&e,-1)){
				e=hmd->list[i];
		}
	}

	memset(hmd->entrys,0,sizeof(Entry)*hmd->cap);
	for(int i=0;i<hmd->cap-1;++i){
		hmd->entrys[i].next=hmd->entrys+i+1;
	}

	hmd->length=0;
	memset((void*)hmd->list,0,sizeof(Entry*)*hmd->cap);
}

static void destroy(HashMap* hm){
	struct _hashMapData* hmd=hm->hmd;
	free(hmd->list);
	free(hmd->entrys);
	free(hmd);
	free(hm);
}

static inline unsigned long long hashCode(const char* key,size_t keySize){
    unsigned long long h = 0;
	for (const char *p = key + keySize - 1; p >= key; --p) {
		h = h * 31 + *p;
	}
    return h;
}

//Find out its address if it exists, if not, return an address where it can be stored
static inline Entry * indx(Entry* en,const char* key,size_t keySize){
	while (en) {
		if (!(en->keySize == keySize ? memcmp(en->key, key, keySize) : 1))
			break;
		en = en->next;
	};
	return en;
}

static ut_hm_Entry remove(HashMap* hm,const char* const key, const size_t keySize){
	struct _hashMapData* hmd=hm->hmd;

	int ind=hashCode(key,keySize)%hmd->cap;
	if (hmd->list[ind]) {
		//lock list[ind]=-1
		intptr_t i = hmd->list[ind];
		while (i == -1 || !atomic_compare_exchange_strong(hmd->list + ind, &i, -1)) {
			i = hmd->list[ind];
		}

		Entry _e = { .next = (Entry*)i }, *e = &_e;
		while (e->next != NULL) {
			if (!(e->next->keySize == keySize ?
					memcmp(e->next->key, key, keySize) : 1))
				break;
			e = e->next;
		};

		if (e->next) {
			if(e->next==(Entry*)i){
				i=(intptr_t)e->next->next;
				e=e->next;
			}else{
				Entry* ee=e->next;
				e->next=ee->next;
				e=ee;
			}
			_e.key=e->key;
			_e.keySize=e->keySize;
			_e.obj=e->obj;

			e->key=NULL;
			do{
				e->next=(Entry*)hmd->head;
			}while(!atomic_compare_exchange_strong(&hmd->head,(intptr_t*)(&(e->next)),(intptr_t)e));

			hmd->length++;
			//unlock
			hmd->list[ind] = i;
			return (ut_hm_Entry){_e.obj,_e.key,_e.keySize};
		} else
			//unlock
			hmd->list[ind] = i;
	}
	return (ut_hm_Entry){0};
}

static int put(HashMap* hm,void* obj,const char* const key, const size_t keySize){
	struct _hashMapData* hmd=hm->hmd;
	int ind=hashCode(key,keySize)%hmd->cap;
	//lock list[ind]=-1
	intptr_t i=hmd->list[ind];
	while(i==-1||!atomic_compare_exchange_strong(hmd->list+ind,&i,-1)){
			i=hmd->list[ind];
	}

	Entry * en=indx((Entry*)i,key,keySize);
	if(en!=NULL){
		en->obj=obj;
		hmd->list[ind]=i;
		return 0;
	}else{
		unsigned int l = hmd->length;
		while(l<hmd->cap){
			if(atomic_compare_exchange_strong(&hmd->length,&l,l+1)){
				Entry* e=(Entry*)atomic_exchange(&(hmd->head),(intptr_t)((Entry*)hmd->head)->next);
				e->key=key;
				e->keySize=keySize;
				e->obj=obj;
				e->next=(Entry*)i;
				hmd->list[ind]=(intptr_t)e;
				return 0;
			}else
				l = hmd->length;
		}

		hmd->list[ind]=i;
		return -1;
	}
}

static void * get(HashMap* hm,const char* const key,size_t keySize){
	struct _hashMapData* hmd=hm->hmd;

	int ind=hashCode(key,keySize)%hmd->cap;
	intptr_t i=hmd->list[ind];
	while(i==-1||!atomic_compare_exchange_strong(hmd->list+ind,&i,-1)){
			i=hmd->list[ind];
	}

	Entry* e=indx((Entry*)i,key,keySize);

	hmd->list[ind]=i;
	return e?e->obj:NULL;
}

static unsigned int getLength(HashMap* hm){
	return ((struct _hashMapData*)hm->hmd)->length;
}

static unsigned int getCap(HashMap* hm){
	return ((struct _hashMapData*)hm->hmd)->cap;
}

static const ut_hm_Entry* iterate(const ut_hm_Entry* e,const ut_hm_HashMap* hm){
	struct _hashMapData *hmd=hm->hmd;
	Entry* p=e;
	if(p<hmd->entrys)
		return NULL;

	Entry* end=hmd->entrys+hmd->cap-1;
	while(++p<=end){
		if(p->key)
			return p;
	};
	return NULL;
}

ut_hm_HashMap * hashMapCreate(unsigned int cap){
	if(cap<2)
		return NULL;
//	printf("%lu\n",sizeof(HashTableData));
	struct _HashMap* hm= malloc(sizeof(HashMap));
	if(!hm)
		return NULL;
	if(!(hm->hmd=malloc(sizeof(struct _HashMap))))
		goto ec;

	if(	!(hm->hmd->list = calloc(cap, sizeof(Entry*)))||
		!(*(Entry**)(&(hm->hmd->entrys)) = calloc(cap, sizeof(Entry))))
		goto ec_d;

	*((unsigned*)(&(hm->hmd->cap)))=cap;
	hm->hmd->length=0;
	/*link*/
	for(int i=0;i < hm->hmd->cap-1;++i)
		hm->hmd->entrys[i].next=hm->hmd->entrys+i+1;

	hm->hmd->head=(intptr_t)hm->hmd->entrys;
	static const struct _ut_hm_hashmap_op OP = {
			.get = get,
			.put = put,
			.empty = empty,
			.destroy = destroy,
			.getLength = getLength,
			.getCapacity = getCap,
			.remove=remove,
			.iterate=iterate
	};
	hm->op=&OP;
	return hm;

ec_d:
	free(hm->hmd->entrys);
	free(hm->hmd->list);
	free(hm->hmd);
ec:
	free(hm);
	return NULL;
}


#undef HashMap
