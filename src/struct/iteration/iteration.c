/*
 * iteration.c
 *
 *  Created on: Mar 11, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */

#include <stddef.h>
#include "utility/struct/iteration.h"

typedef struct _iteration{
	const void* (*next)(ut_st_Iteration*);
	struct{
		const void* container;
		const void* current;
		const void* (*next)(const void* current, const void* container);
	}p;
}Iteration;

static const void* next(ut_st_Iteration* ite){
	Iteration *it=(Iteration*)ite;
	return it->p.current=it->p.next(it->p.current,it->p.container);
}

ut_st_Iteration newIteration(const void* container,const void* (*iterate)(const void* current, const void* container)){
	ut_st_Iteration it={
			.next=next,
			.p={
					container,
					NULL,
					iterate
			}
	};
	return it;
}
