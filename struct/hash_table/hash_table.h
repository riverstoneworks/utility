/*
 * Hash.h
 *
 *  Created on: Apr 16, 2018
 *      Author: xpc
 */

#ifndef HASH_H_
#define HASH_H_
struct Element;
struct HashTable;
struct HashTableData;
typedef struct HashTableData HashTableData;
typedef struct Element Element ;
typedef struct HashTable HashTable;
extern HashTable hashTableCreate(unsigned int cap);

struct HashTable{
	struct HashTableData* htd;
	int (*set)(HashTable* ,void* ,const char* ,size_t );
	void *(*get)(HashTable* ,const char* ,size_t );
	void (*empty)(HashTable* );
	void (*destroy)(HashTable* );
	unsigned int (*getLength)(HashTable*);
	unsigned int (*getCapacity)(HashTable*);
};


#endif /* HASH_H_ */
