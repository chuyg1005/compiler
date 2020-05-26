#include "hash_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
/* #include <time.h> */
/* #define DEBUG */

/* void Log(const char *format, ...); */
void printHashTable(HashTable *self);
void testHashTable(HashTable *self);
bool keyEqual(Key key1, Key key2);
void Log(const char *format, ...);
/* const Value INV_VALUE = { .kind = OP_INV }; */

/* static HashNode* find(Key key); */
static unsigned hash(unsigned char *key, unsigned len);

HashNode* newHashNode(Key key, Value value) {
    HashNode* hashNode = (HashNode*)malloc(sizeof(HashNode));
    hashNode->key = key;
    hashNode->value = value;
    hashNode->next = NULL;
    return hashNode;
}

HashTable* newHashTable() {
    HashTable *hashTable = (HashTable*)malloc(sizeof(HashTable));
    memset(hashTable->table, 0, sizeof(hashTable->table));
    hashTable->size = 0;
    /* hashTable->hot = NULL; */
    return hashTable;
}

unsigned hash(unsigned char *key, unsigned len) {
    unsigned val = 0;
    for(unsigned i = 0; i < len; i++) {
        val = 31 * val + key[i];
    }
    /* unsigned val = 0; */
    /* for(unsigned i = 0; *key && i < len; i++) { */
    /*     val = (val << 2) + *key; */
    /*     if((i = val & ~HASH_SIZE) != 0) { */
    /*         val = (val ^ (i >> 12)) & ~HASH_SIZE; */
    /*     } */
    /* } */
    val ^= (val >> 20) ^ (val >> 12);
    return (val ^ (val >> 7) ^ (val >> 4)) % HASH_SIZE;
}

Value HT_find(HashTable *self, Key key) {
    unsigned hashcode = hash((unsigned char*)&key, sizeof(Key));
    /* Value value = INV_VALUE;    // pre assign to invalid value */
    assert(hashcode < HASH_SIZE);
    HashNode *node = self->table[hashcode];
    while(node) {
        if(keyEqual(node->key, key)) break;
        node = node->next;
    }
    return node == NULL ? NULL : node->value;
}


void HT_insert(HashTable *self, Key key, Value value) {
    unsigned hashcode = hash((unsigned char*)&key, sizeof(Key));
    /* Value value = INV_VALUE;    // pre assign to invalid value */
    assert(hashcode < HASH_SIZE);
    HashNode *cur = self->table[hashcode];
    if(!cur) {
        self->table[hashcode] = newHashNode(key, value);
        self->size++;
        Log("insert into new key value pair, key: %d, value: %d\n", key, value);
        return;
    }
    HashNode *prev = cur;
    while(cur) {
        if(keyEqual(cur->key, key)) break;
        prev = cur;
        cur = cur->next;
    }
    if(cur) {
        cur->value = value;
        Log("override previous value, key: %d, value: %d\n", key, value);
    } else {
        HashNode *node = newHashNode(key, value);
        prev->next = node;
        self->size++;
        Log("insert into new key value pair, key: %d, value: %d\n", key, value);
    }
}

void HT_clear(HashTable *self) {
    for(int i = 0; i < HASH_SIZE; i++) {
        HashNode *node = self->table[i];
        while(node) {
            HashNode *p = node;
            node = node->next;
            free(p);
        }
    }
    self->size = 0;
    memset(self->table, 0, sizeof(self->table));
}

bool keyEqual(Key key1, Key key2) {
    int len = sizeof(Key);
    for(int i = 0; i < len; i++) {
        if(*((unsigned char*)&key1 + i) != *((unsigned char*)&key2 + i)) {
            return false;
        }
    }
    return true;
}

void printHashTable(HashTable *self) {
    assert(self);
    for(int i = 0; i < HASH_SIZE; i++) {
        HashNode *node = self->table[i];
        int j = 0;
        while(node) {
            /* printf("key = %d, value = %d\n", node->key, node->value); */
            node = node->next;
            j++;
        }
        Log("bucket size = %d\n", j);
    }

    Log("table size = %d, load factor = %.2f\n", self->size, (double)(self->size) / HASH_SIZE);
}

void testHashTable(HashTable *self) {
    /* assert(self); */
    /* assert(self->size == 0); */
    /* /1* srand(time(NULL)); *1/ */
    /* for(int i = 0; i < HASH_SIZE * 2; i++) { */
    /*     /1* int r = rand(); *1/ */
    /*     /1* HT_insert(self, r, r+1); *1/ */
    /*     HT_insert(self, i, i+1); */
    /* } */
    /* assert(self->size == 2 * HASH_SIZE); */
    /* printHashTable(self); */
    /* for(int i = 0; i < HASH_SIZE * 2; i++) { */
    /*     /1* int r = rand(); *1/ */
    /*     /1* HT_insert(self, r, r+1); *1/ */
    /*     assert(HT_find(self, i) == i+1); */
    /* } */
    /* assert(self->size == 2 * HASH_SIZE); */
    /* HT_clear(self); */
    /* assert(self->size == 0); */
}

/* void Log(const char *format, ...) { */
/* #ifdef DEBUG */
/*     va_list ap; */
/*     va_start(ap, format); */
/*     vprintf(format, ap); */
/*     va_end(ap); */
/* #endif */
/* } */


/* int main() { */
/*     HashTable *hashTable = newHashTable(); */
/*     testHashTable(hashTable); */
/*     free(hashTable); */
/* } */
