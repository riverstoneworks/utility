/*
 ============================================================================
 Name        : Hash.c
 Author      : Like.Z
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "hash_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct Element{
	Element* next;
	void* obj;
	char* key;
	size_t keySize;
};

struct HashTableData{
	Element** list;
	unsigned int size;
	const unsigned int cap;
	Element* element;
};

static void hashTableEmpty(HashTable* ht){
	HashTableData* htd=ht->htd;
	memset(htd->list,0,sizeof(Element*)*htd->cap);
	for(int i=htd->size-1;i>-1;--i)
		free(htd->element[i].key);
	htd->size=0;
}

static void hashTableDestroy(HashTable* ht){
	HashTableData* htd=ht->htd;
	free(htd->list);
	for(int i=htd->size-1;i>-1;--i)
		free(htd->element[i].key);
	free(htd->element);
	free(htd);
}

static inline unsigned long long hashValue(const char* key,size_t keySize){
    unsigned long long h = 0;
	for (const char *p = key + keySize - 1; p >= key; --p) {
		h = h * 31 + *p;
	}
    return h;
}

//Find out its address if it exists, if not, return an address where it can be stored
static inline Element ** hashTableInd(HashTable* ht,const char* key,size_t keySize){
	HashTableData * htd=ht->htd;
	Element el={.next=NULL},*e=&el;
	int index=hashValue(key,keySize)%htd->cap;printf("%d\n",index);
	if(htd->list[index]!=NULL){
		e->next=htd->list[index];
		do{
			if(!(keySize==e->next->keySize?memcmp(e->next->key,key, keySize):-1))
				break;
			e=e->next;
		}while(e->next!=NULL);
		return &(e->next);
	}else{
		return htd->list+index;
	}
}

static int hashTableSet(HashTable* ht,void* obj,const char* key,size_t keySize){
	HashTableData * htd=ht->htd;
	Element ** el=hashTableInd(ht,key,keySize);
	if(*el!=NULL){
		(*el)->obj=obj;
	}else{

		if(!(htd->size<htd->cap)){
			return -1;
		}else{
			Element* e=htd->element+htd->size++;
			e->keySize=keySize;
			e->key=malloc(keySize);
			memcpy(e->key,key,keySize);
			e->obj=obj;
			e->next=NULL;

			*el=e;
		}
	}
	return htd->size;
}

static void * hashTableGet(HashTable* ht,const char* key,size_t keySize){
	Element** e=hashTableInd(ht,key,keySize);
	return *e==NULL?NULL:(*e)->obj;
}

static unsigned int hashTableGetLength(HashTable* ht){
	return ht->htd->size;
}

static unsigned int hashTableGetCap(HashTable* ht){
	return ht->htd->cap;
}

HashTable hashTableCreate(unsigned int cap){
	assert(cap>0);
	printf("%lu\n",sizeof(HashTableData));
	HashTableData* d=malloc(sizeof(HashTableData));
	*((unsigned*)(&(d->cap)))=cap;
	d->size=0;
	d->element=malloc(sizeof(Element) * cap);
	d->list = malloc(sizeof(Element*) * cap);
	HashTable ht = {
			.htd = d,
			.get = hashTableGet,
			.set = hashTableSet,
			.empty = hashTableEmpty,
			.destroy= hashTableDestroy,
			.getLength= hashTableGetLength,
			.getCapacity=hashTableGetCap
	};
	return ht;
}
