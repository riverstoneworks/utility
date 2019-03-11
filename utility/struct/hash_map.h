/*
 * hash_table.h
 *
 *  Created on: Mar 2, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */

#ifndef STRUCT_HASH_MAP_H_
#define STRUCT_HASH_MAP_H_

typedef struct _ut_hm_HashMap ut_hm_HashMap;
typedef struct _ut_hm_entry{
	void* obj;
	char* key;
	size_t keySize;
}ut_hm_Entry;


struct _ut_hm_HashMap{
	const void* const hmd;
	struct _hashmap_op{
		int (*init)(void** hmd, unsigned int cap);
		int (*put)(ut_hm_HashMap* ,void* val,const char* const key,const size_t keySize);
		void *(*get)(ut_hm_HashMap* ,const char* const key,const size_t keySize);
		void (*empty)(ut_hm_HashMap* );
		ut_hm_Entry (*remove)(ut_hm_HashMap* ,const char* const key,const size_t keySize);
		void (*destroy)(ut_hm_HashMap* );
		unsigned int (*getLength)(ut_hm_HashMap*);
		unsigned int (*getCapacity)(ut_hm_HashMap*);
		const ut_hm_Entry* (*iterate)(const ut_hm_Entry*, const ut_hm_HashMap*);
	}const * const op;
};

extern ut_hm_HashMap * hashMapCreate(unsigned int cap);

#endif /* STRUCT_HASH_MAP_H_ */
