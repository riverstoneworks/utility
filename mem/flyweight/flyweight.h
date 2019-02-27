/*
 * flyweight.h
 *
 *  Created on: Feb 21, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */

#ifndef FLYWEIGHT_H_
#define FLYWEIGHT_H_
#include<stdatomic.h>

typedef struct _ELE Element;
struct _ELE{
	Element * next;
	void* data;
};

typedef struct _BLK Block;
struct _BLK{
	Block *next;
	Element* eles;
	void* data;
	size_t s_ele;
	unsigned int n_eles;

};

typedef struct _ELEPOOL {
	volatile atomic_intptr_t head; // @suppress("Type cannot be resolved")
	volatile atomic_intptr_t end; // @suppress("Type cannot be resolved")
	volatile atomic_intptr_t blocks; // @suppress("Type cannot be resolved")
	unsigned int n_auto_inc;
	unsigned short n_blocks;
	unsigned short n_max_blocks;
}ElementPool;

extern ElementPool* newPool(size_t size_element,unsigned n_element,unsigned n_auto_inc,unsigned short n_max_blocks);
extern Element* eleAlloc(ElementPool*);
extern int eleRec(ElementPool*,Element*);
extern int poolInc(ElementPool*,unsigned n_eles,size_t s_ele);
extern unsigned poolDec(ElementPool*);
extern int destoryPool(ElementPool*);

#endif /* FLYWEIGHT_H_ */
