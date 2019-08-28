/*
 * flywtAuto.h
 *
 *  Created on: Aug 23, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */

#ifndef MEM_FLYWEIGHT_AUTO_H_
#define MEM_FLYWEIGHT_AUTO_H_

typedef struct _ut_fw_auto ut_fw_auto;
struct _ut_fw_auto{
	struct _auto const*  d;
	struct _ut_fw_auto_op{
		void (* const append)(ut_fw_ElementPool,ut_fw_auto);
		void (* const maintain)(ut_fw_auto);
		void (* const destory)(ut_fw_auto*);
	}const *o;

};

#endif /* MEM_FLYWEIGHT_AUTO_H_ */
