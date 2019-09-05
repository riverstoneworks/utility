/*
 * flyweight.h
 *
 *  Created on: Feb 21, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */

#ifndef FLYWEIGHT_H_
#define FLYWEIGHT_H_
#include <stddef.h>
#include <stdbool.h>

typedef struct _ut_fw_ElementPool ut_fw_ElementPool;
struct _ut_fw_ElementPool{
	struct _pool const*  d;
	struct _ut_fw_ElementPool_op{
		void* (* const eleAlloc)(ut_fw_ElementPool);
		void (* const eleRec)(void const * const,ut_fw_ElementPool);
		int (* const destory)(ut_fw_ElementPool*);
		int (*const poolDec)(ut_fw_ElementPool);
		void (* const showInfo)(ut_fw_ElementPool);
	}const * o;
};

extern ut_fw_ElementPool newPool(size_t size_element,unsigned n_init,unsigned n_auto_inc,unsigned short n_max_blocks, bool auto_manage);

//automatic manager
typedef struct _ut_fw_manager ut_fw_manager;
typedef enum { ERROR=-3,SLEEP=0,RUNNING,STOP } manager_stat;
struct _ut_fw_manager{
	struct _manager const*  d;
	struct _ut_fw_auto_op{
		int (* const append)(ut_fw_ElementPool,ut_fw_manager);
		manager_stat (* const maintain)(ut_fw_manager);
		void (* const destory)(ut_fw_manager*);
	}const *o;

};
//how to run function 'maintain'
typedef int(*launch)(ut_fw_manager);

extern ut_fw_manager newFwManager(launch lau);

extern int init_global_atm(launch);
extern void destory_global_atm();
#endif /* FLYWEIGHT_H_ */
