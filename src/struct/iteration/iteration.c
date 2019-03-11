/*
 * iteration.c
 *
 *  Created on: Mar 11, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */

#include <stddef.h>
#include "utility/struct/iteration.h"

typedef struct _iteration{
	void* (*next)(ut_st_Iteration*);
	struct{
		void* container;
		void* current;
		void* (*next)(void* current,void* container);
	}p;
}Iteration;

static void* next(ut_st_Iteration* ite){
	Iteration *it=(Iteration*)ite;
	return it->p.current=it->p.next(it->p.current,it->p.container);
}

ut_st_Iteration newIteration(void* container,void* (*_next)(void* current,void* container)){
	ut_st_Iteration it={
			.next=next,
			.p={
					container,
					NULL,
					_next
			}
	};
	return it;
}
