#include "common.h"
#include "hashMap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HashMap *hMap;

int createHashMap() {

    hMap = (HashMap *) malloc(sizeof(HashMap));
    hMap->elements = (struct _hashMapElem **) calloc(
            SLOTLENGTH, sizeof(struct _hashMapElem *));

    if (!hMap || !hMap->elements) {
        printf("Error in createHashMap(), momory allocation not success\n");
        return STATUS_ERROR;

    } else {

        hMap->size = 0;
        hMap->totalElem = 0;
        return STATUS_SUCCESS;
    }
}

void destroyHashMap() {

    HashMap *hm = hMap;
    if (hm == NULL) {
        printf("destroyHashMap(): cannot destroy hMap, it is NULL.\n");

    }

    free(hm->elements);
    free(hm);
}


unsigned int keyToIndex(char *key) {  // return -1 means error

    unsigned long int key_int;
    unsigned int index;

    if (key == NULL) {

        printf("Error in keyToIndex(), empty key.\n");
        return -1;

    }

    key_int = stringToInt(key);
    index = key_int % SLOTLENGTH;

    if (index >= 0)
        return index;
    else
        return -1;

}

int hashMapRemove(char *key) {

    hashMapElem *p, *q;
    unsigned int index;
    char *string_addr;
    string_addr = key;

    if (hMap == NULL) {
        printf("Error in HashMapRemove(), empty hMap.\n");
        return STATUS_NULL;
    }

    if (key == NULL) {
        printf("Error in HashMapRemove(), empty key.\n");
        return STATUS_NULL;
    }

    index = keyToIndex(string_addr);
    //printf("-----------------entering HashMapRemove(), key is %s, value is %d, index is %u.\n", key, value, index);

    if (index == -1) {
        printf("Error in HashMapRemove(), keyToIndex error.\n");
        return STATUS_NULL;
    }

    if ((hMap->elements[index]) != NULL) {

        p = hMap->elements[index];
        if (!strcmp(p->key, key)) { // slot element is the to be removed one.

            hMap->elements[index] = p->next;
            //printf("slot element remove with key:%s\n",p->key);
            free(p->key);
            free(p);

            if (hMap->elements[index] == NULL) {
                hMap->size--;// if hMap->elements[index] is empty, size--
            }
            hMap->totalElem--;
            //printf("---------successfully remove the element.\n ");
            return STATUS_SUCCESS;
        }

        while (p->next != NULL) {

            if (!strcmp(p->next->key, key)) { // list element is the to be removed one.
                q = p->next; // q needs to be removed
                //printf("list element remove with key:%s\n",q->key);
                p->next = q->next; // ******needs to be very careful
                free(q->key);
                free(q);
                hMap->totalElem--;
                //printf("---------successfully remove the element.\n");
                return STATUS_SUCCESS;
            }
            p = p->next;
        }

        printf("element with key does not in the list\n");
        return STATUS_SUCCESS;

    } else {

        printf("element does not exist, slot empty\n");
        return STATUS_SUCCESS;
    }
}

/**
 *
 * @param key
 * @param value
 * @return old value of elem <key,value>
 */
int hashMapPut(char *key, unsigned long value) {
    struct _hashMapElem *p, *q;
    char *start_addr = key;
    int oldvalue;
    unsigned int index;

    if (hMap == NULL) {
        printf("Error in HashMapPut(), empty hMap.\n");
        return -1;
    }

    if (key == NULL) {
        printf("Error in HashMapPut(), empty key.\n");
        return -1;
    }

    index = keyToIndex(start_addr);
    //printf("-----------------entering HashMapPut(), key is %s, value is %d, index is %u.\n", key, value, index);

    if (index == -1) {
        printf("Error in HashMapPut(), keyToIndex error.\n");
        return -1;
    }

    if (hMap->elements[index] == NULL) { // the index slot is empty, put the first element
        //printf("elements[%u] position is NULL. \n",index);

        p = (struct _hashMapElem *) malloc(sizeof(struct _hashMapElem));
        if (p == NULL) {

            printf("Error in HashMapPut() 1, HashMapelem allocation error.\n");
            return -1;
        }

        p->key = key;
        p->next = NULL;
        p->value = value;
        hMap->elements[index] = p;
        hMap->size++;
        hMap->totalElem++;

//        printf("successfully added one element to elements[%d], key is %s, value is %d.\n", index, hMap->elements[index]->key,hMap->elements[index]->value);
        return MAP_SUCCESS;

    } else { // the index slot is not empty

        q = hMap->elements[index];

        if (!strcmp(q->key, key)) {
            // the put element is the first elem
//            printf("the put element is the first element of elements[%u]\n",index);
            oldvalue = q->value;
            q->value = value;
            return oldvalue;

        } else {

            while (q->next != NULL) { // search in the list of elements[index]

                if (!strcmp(q->next->key, key)) {

//                    printf("the put element is in the list of elements[%u]\n",index);
                    oldvalue = q->next->value;
                    q->next->value = value;
                    return oldvalue;
                }

                q = q->next;
            }

            //printf("the put element is not in the list of elements[%u]\n",index);
            //the put element is not in the list. put the new element at the first position
            p = (struct _hashMapElem *) malloc(sizeof(struct _hashMapElem));

            if (p == NULL) {

//                printf("Error in HashMapPut() 2, HashMapelem allocation error.\n");
                return -1;
            }

            p->key = key;
            p->value = value;
            p->next = NULL;
            // ***here needs to be very careful. insert the put node as the first node
            p->next = hMap->elements[index];
            hMap->elements[index] = p;
            hMap->totalElem++;
//            printf("successfully added one element to the list of elements[%d], key is %s, value is %d.\n", index, hMap->elements[index]->key,hMap->elements[index]->value);
            return MAP_SUCCESS;

        }
    }
}

/**
 * key points to a string showing an address in hex format
 * @param key
 * @return
 */
int hashMapGet(char *key) {
    unsigned int index;
    struct _hashMapElem *p, *q;
    //printf("---------entering HashMapGet().\n");

    if (hMap == NULL) {

        printf("Error in HashMapGet(), empty hMap.\n");
        return -1;

    }

    if (key == NULL) {

        printf("Error in HashMapGet(), empty key");
        return MAP_MISS;
    }

    index = keyToIndex(key);

    if (index != -1) {

        p = hMap->elements[index];
        q = p;
        //printf("find index:%u, hMap->elements[%u] has key:%s\n", index, index, p->key);
        //printf("printing list elements on this index:\n");
        /*while (q!=NULL){
            printf("             key:%s, value:%d \n", q->key, q->value);
            q = q->next;
        }*/

        if (!strcmp(p->key, key)) {// the slot element's key matches.
            //printf(" slot element match.\n\n");
            return p->value;
        }

        while (p->next != NULL) {

            if (!strcmp(p->next->key, key)) { // the key is in the list of hMap->elements[index]
                //printf("list element matches.\n\n");
                return p->next->value;
            }

            p = p->next;
        }

        //printf("HashMapGet() return, does not find\n");
        return MAP_MISS;

    } else {
        //printf("keyToIndex() is -1, wrong HashMapGet() return\n");

        return MAP_MISS;
    }
}

unsigned long int stringToInt(char *str) {

    unsigned long int intaddr;
    if (str == NULL) {
        printf("Error in stringToInt(), empty str\n");
        return -1;
    } else {
        intaddr = strtol(str, NULL, 16);
        return intaddr;
    }
}

unsigned long currentSlotSize() {
    HashMap *ph = hMap;
    if (ph == NULL) {

        printf("Error in currentSlotSize(), empty hMap\n");
        return -1;

    } else {

        return ph->size;
    }

}

unsigned long currentElemSize() {

    HashMap *ph = hMap;

    if (ph == NULL) {

        printf("Error in currentElemSize(), empty hMap\n");
        return -1;

    } else {

        return ph->totalElem;
    }
}
