/*
 * flyweight.h
 *
 *  Created on: Feb 21, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */

#ifndef FLYWEIGHT_H_
#define FLYWEIGHT_H_
#include <stddef.h>
typedef struct _ut_fw_ElementPool ut_fw_ElementPool;
struct _ut_fw_ElementPool{
	struct _pool const*  d;
	struct _ut_fw_ElementPool_op{
		void* (* const eleAlloc)(ut_fw_ElementPool);
		void (* const eleRec)(void const * const,ut_fw_ElementPool);
		int (* const destory)(ut_fw_ElementPool*);
		void (* const showInfo)(ut_fw_ElementPool);
	}const * o;
};

extern ut_fw_ElementPool newPool(size_t size_element,unsigned n_init,unsigned n_auto_inc,unsigned short n_max_blocks);

typedef struct _ut_fw_PoolsDaemon ut_fw_PoolsDaemon;
struct _ut_fw_PoolsDaemon{
	struct _daemon const* d;
	struct _ut_fw_PoolsDaemon_op{
		void (* const gc)(ut_fw_PoolsDaemon*);
		void (* const append)(ut_fw_PoolsDaemon*,ut_fw_ElementPool*);
		void (* const remove)(ut_fw_PoolsDaemon*, ut_fw_ElementPool*);
		int (* const destory)(ut_fw_PoolsDaemon*);
	}const * o;
};

extern ut_fw_PoolsDaemon newPoolsDaemon();
//#define Element ut_fw_Element
//#define ElementPool ut_fw_ElementPool
#endif /* FLYWEIGHT_H_ */
