#ifndef _HASHTABLE_LOGFILE_H_
#define _HASHTABLE_LOGFILE_H_

#include <stdio.h>
#include <stddef.h>

typedef struct logfile_withref_s {
    FILE            *fd;
    size_t          ref_count;
} logfile_withref_t;
    
#define VALUETYPE               logfile_withref_t*
#define NONEXISTVALUE           NULL

#define KEYTYPESTRING
// #ifdef KEYTYPESTRING

#define KEYTYPE                 char*
#define NONEXISTKEY             NULL
#define ISNONEXISTKEY(key)      ((key) == (KEYTYPE)NONEXISTKEY)
#define ISEQUALKEY(key1, key2)  (strcmp((key1), (key2)) == 0)
#define DEEPCOPYKEY(dstKey, srcKey) \
          {\
            dstKey = (char*) malloc(sizeof(char) * (strlen(srcKey) + 1)); \
            strcpy(dstKey, key); \
          }
#define FREEKEY(key) free(key);

// #else
// 
// #define KEYTYPE       char*
// #define NONEXISTKEY   0xffffffff
// #define ISNONEXISTKEY(key)      ((key) == (KEYTYPE)NONEXISTKEY)
// #define ISEQUALKEY(key1, key2)       ((key1) == (key2))
// #define DEEPCOPYKEY(dstKey, srcKey)  (dstKey) = (KEYTYPE)(srcKey);
// #define FREEKEY(key)
// 
// #endif

typedef struct TableEntry {
    KEYTYPE key;
    VALUETYPE value;
    struct TableEntry *next;
} TableEntry;

typedef struct {
    size_t tableSize;
    size_t curSize;
    size_t (*hashFunc)(const KEYTYPE, size_t);
    struct TableEntry *contents;
} HashTable;

typedef struct HashTableIter {
    TableEntry *endPoint;
    TableEntry *curPoint;
    TableEntry *inBucketPoint;
} HashTableIter;

#define createHashTable createHashTable_logfile
#define destroyHashTable destroyHashTable_logfile

#define removeKey removeKey_logfile
#define putKeyValue putKeyValue_logfile

#define hasKey hasKey_logfile
#define findKey findKey_logfile
#define findValueByKey findValueByKey_logfile

#define getHashTableSize getHashTableSize_logfile

#define getHashTableIter getHashTableIter_logfile
#define destroyHashTableIter destroyHashTableIter_logfile
#define hasNextElement hasNextElement_logfile
#define nextElement nextElement_logfile


HashTable* createHashTable(size_t size);
void destroyHashTable(HashTable *tb);

void removeKey(HashTable *tb, const KEYTYPE key);
void putKeyValue(HashTable *tb, const KEYTYPE key, VALUETYPE value);

int hasKey(HashTable *tb, const KEYTYPE key);
TableEntry *findKey(HashTable *tb, const KEYTYPE key);
VALUETYPE findValueByKey(HashTable *tb, const KEYTYPE key);

size_t getHashTableSize(HashTable *tb);

HashTableIter *getHashTableIter(HashTable *tb);
void destroyHashTableIter(HashTableIter *ite);
int hasNextElement(HashTableIter *ite);
TableEntry *nextElement(HashTableIter *ite);

#endif /* _HASHTABLE_LOGFILE_H_ */
