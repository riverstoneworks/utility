/*
 * iteration.h
 *
 *  Created on: Mar 11, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */

#ifndef STRUCT_ITERATION_ITERATION_H_
#define STRUCT_ITERATION_ITERATION_H_

typedef struct _ut_st_iteration{
	void* (*next)(struct _ut_st_iteration*);
	const void *const p[3];
}ut_st_Iteration;

extern ut_st_Iteration newIteration( const void* container,const void* (*next)(const void* current, const void* container));
#endif /* STRUCT_ITERATION_ITERATION_H_ */
