/*
 * flyweight.h
 *
 *  Created on: Feb 21, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */

#ifndef FLYWEIGHT_H_
#define FLYWEIGHT_H_

typedef struct _ut_fw_Element ut_fw_Element;
typedef struct _ut_fw_ElementPool ut_fw_ElementPool;

extern ut_fw_ElementPool* newPool(size_t size_element,unsigned n_element,unsigned n_auto_inc,unsigned short n_max_blocks);
extern ut_fw_Element* eleAlloc(ut_fw_ElementPool*);
extern int eleRec(ut_fw_ElementPool*,ut_fw_Element*);
extern int poolInc(ut_fw_ElementPool*,unsigned n_eles,size_t s_ele);
extern unsigned poolDec(ut_fw_ElementPool*);
extern int destoryPool(ut_fw_ElementPool*);
extern void showInfo(ut_fw_ElementPool*);

//#define Element ut_fw_Element
//#define ElementPool ut_fw_ElementPool
#endif /* FLYWEIGHT_H_ */
