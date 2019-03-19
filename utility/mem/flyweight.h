/*
 * flyweight.h
 *
 *  Created on: Feb 21, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */

#ifndef FLYWEIGHT_H_
#define FLYWEIGHT_H_
#include <stdint.h>
typedef struct _ut_fw_ElementPool ut_fw_ElementPool;
struct _ut_fw_ElementPool{
	struct _ut_fw_ElementPool_op{
		void* (* const eleAlloc)(ut_fw_ElementPool*);
		void (* const eleRec)(ut_fw_ElementPool*,void const * const);
		int (* const poolInc)(ut_fw_ElementPool*,const unsigned n_eles, const size_t s_ele);
		long (* const poolDec)(ut_fw_ElementPool*);
		int (* const destoryPool)(ut_fw_ElementPool*);
		void (* const showInfo)(ut_fw_ElementPool*);
	}const *const op;
	const int d[sizeof(intptr_t)/sizeof(int)*2+2];
};

extern ut_fw_ElementPool* newPool(size_t size_element,unsigned n_element,unsigned n_auto_inc,unsigned short n_max_blocks);

//#define Element ut_fw_Element
//#define ElementPool ut_fw_ElementPool
#endif /* FLYWEIGHT_H_ */
