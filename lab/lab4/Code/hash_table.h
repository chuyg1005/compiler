#ifndef __HASH_TABLE__
#define __HASH_TABLE__

#include "ir.h"
#define HASH_SIZE 0x3ff // 1023
/* #define INV_VALUE 0 */

typedef Operand Key;
/* typedef Operand Value; */
typedef IRList* Value;

typedef struct HashNode HashNode;
typedef struct HashTable HashTable;

struct HashNode {
    Key key;
    Value value;
    HashNode *next;
};

struct HashTable {
   HashNode *table[HASH_SIZE];
   /* HashNode *hot; */
   int size; 
};

HashNode* newHashNode(Key key, Value);
HashTable* newHashTable();
void HT_insert(HashTable *self, Key key, Value value);
Value HT_find(HashTable *self, Key key);
void HT_clear(HashTable *self);

#endif
