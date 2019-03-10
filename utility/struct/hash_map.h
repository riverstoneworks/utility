/*
 * hash_table.h
 *
 *  Created on: Mar 2, 2019
 *      Author: Like.Z(sxpc722@aliyun.com)
 */

#ifndef STRUCT_HASH_MAP_H_
#define STRUCT_HASH_MAP_H_

typedef struct _ut_ht_HashMap ut_ht_HashMap;
typedef struct _ut_ht_entry ut_ht_Entry;
struct _ut_ht_entry{
	void* obj;
	char* key;
	size_t keySize;
	ut_ht_Entry* next;
};


struct _ut_ht_HashMap{
	const void* const hmd;
	struct _hashmap_op{
		int (*init)(void** hmd, unsigned int cap);
		int (*put)(ut_ht_HashMap* ,void* val,const char* const key,const size_t keySize);
		void *(*get)(ut_ht_HashMap* ,const char* key,const size_t keySize);
		void (*empty)(ut_ht_HashMap* );
		ut_ht_Entry (*remove)(ut_ht_HashMap* ,const char* const key,const size_t keySize);
		void (*destroy)(ut_ht_HashMap* );
		unsigned int (*getLength)(ut_ht_HashMap*);
		unsigned int (*getCapacity)(ut_ht_HashMap*);
	}const * const op;
};

extern ut_ht_HashMap * hashMapCreate(unsigned int cap);

#endif /* STRUCT_HASH_MAP_H_ */
