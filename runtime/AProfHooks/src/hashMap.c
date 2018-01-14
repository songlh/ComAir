#include "common.h"
#include "hashMap.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HashMap *hMap;

int createHashMap() {

    hMap = (HashMap *) malloc(sizeof(HashMap));
    hMap->elements = (struct _hashMapElem **) calloc(
            SLOTLENGTH, sizeof(struct _hashMapElem *));

    if (!hMap || !hMap->elements) {
        log_error("Error in createHashMap(), memory allocation not success.");
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
        log_error("destroyHashMap(): cannot destroy hMap, it is NULL.");
    }

    free(hm->elements);
    free(hm);
}


unsigned int keyToIndex(char *key) {  // return -1 means error

    unsigned long int key_int;
    unsigned int index;

    if (key == NULL) {

        log_error("Error in keyToIndex(), empty key.");
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
        log_error("Error in HashMapRemove(), empty hMap.");
        return STATUS_NULL;
    }

    if (key == NULL) {
        log_error("Error in HashMapRemove(), empty key.");
        return STATUS_NULL;
    }

    index = keyToIndex(string_addr);
    //printf("-----------------entering HashMapRemove(), key is %s, value is %d, index is %u.\n", key, value, index);

    if (index == -1) {
        log_error("Error in HashMapRemove(), keyToIndex error.");
        return STATUS_NULL;
    }

    if ((hMap->elements[index]) != NULL) {

        p = hMap->elements[index];
        if (!strcmp(p->key, key)) { // slot element is the to be removed one.

            hMap->elements[index] = p->next;
            log_debug("slot element remove with key:%s ", p->key);
            free(p->key);
            free(p);

            if (hMap->elements[index] == NULL) {
                hMap->size--;// if hMap->elements[index] is empty, size--
            }
            hMap->totalElem--;
            log_debug("---------successfully remove the element.");
            return STATUS_SUCCESS;
        }

        while (p->next != NULL) {

            if (!strcmp(p->next->key, key)) { // list element is the to be removed one.
                q = p->next; // q needs to be removed
                log_debug("list element remove with key:%s ",q->key);
                p->next = q->next; // ******needs to be very careful
                free(q->key);
                free(q);
                hMap->totalElem--;
                log_debug("---------successfully remove the element.");
                return STATUS_SUCCESS;
            }
            p = p->next;
        }

        log_debug("element with key does not in the list.");
        return STATUS_SUCCESS;

    } else {

        log_debug("element does not exist, slot empty.");
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
    long old_value;
    unsigned int index;

    if (hMap == NULL) {
        log_error("Error in HashMapPut(), empty hMap.");
        return STATUS_NULL;
    }

    if (key == NULL) {
        log_error("Error in HashMapPut(), empty key.");
        return STATUS_NULL;
    }

    index = keyToIndex(start_addr);
    log_trace("HashMapPut(), key is %s, value is %d, index is %u.",
              key, value, index);

    if (index == -1) {
        log_error("Error in HashMapPut(), keyToIndex error.");
        return -1;
    }

    if (hMap->elements[index] == NULL) { // the index slot is empty, put the first element
        log_trace("elements[%u] position is NULL.",index);
        p = (struct _hashMapElem *) malloc(sizeof(struct _hashMapElem));
        if (p == NULL) {
            log_error("Error in HashMapPut() 1, HashMapElem allocation error.");
            return STATUS_NULL;
        }

        p->key = key;
//        strcpy(p->key, key);
        log_debug("hashMapPut() put key is %s", p->key);
        p->next = NULL;
        p->value = value;
        hMap->elements[index] = p;
        hMap->size++;
        hMap->totalElem++;

        log_trace("successfully added one element to elements[%d], key is %s, value is %d.",
                  index, hMap->elements[index]->key, hMap->elements[index]->value);
        return MAP_SUCCESS;

    } else { // the index slot is not empty

        q = hMap->elements[index];

        if (!strcmp(q->key, key)) {
            // the put element is the first elem
            log_trace("the put element is the first element of elements[%u]", index);
            old_value = q->value;
            q->value = value;
            return old_value;

        } else {

            while (q->next != NULL) { // search in the list of elements[index]

                if (!strcmp(q->next->key, key)) {
                    log_trace("the put element is in the list of elements[%u]",index);
                    old_value = q->next->value;
                    q->next->value = value;
                    return old_value;
                }

                q = q->next;
            }

            log_trace("the put element is not in the list of elements[%u]",index);
            //the put element is not in the list. put the new element at the first position
            p = (struct _hashMapElem *) malloc(sizeof(struct _hashMapElem));

            if (p == NULL) {
                log_error("Error in HashMapPut() 2, HashMapElem allocation error.");
                return STATUS_NULL;
            }

            p->key = key;
//            strcpy(p->key, key);
            p->value = value;
            p->next = NULL;
            // ***here needs to be very careful. insert the put node as the first node
            p->next = hMap->elements[index];
            hMap->elements[index] = p;
            hMap->totalElem++;
            log_trace("successfully added one element to the list of elements[%d], key is %s, value is %d.",
                      index, hMap->elements[index]->key,hMap->elements[index]->value);
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

        log_error("Error in HashMapGet(), empty hMap.");
        return STATUS_NULL;

    }

    if (key == NULL) {

        log_error("Error in HashMapGet(), empty key");
        return STATUS_NULL;
    }

    index = keyToIndex(key);

    if (index != -1) {

        p = hMap->elements[index];

        log_debug("p->key is %s, current key is %s", p->key, key);
        if (!strcmp(p->key, key)) {// the slot element's key matches.
            log_debug(" slot element match.");
            return p->value;
        }

        while (p->next != NULL) {

            if (!strcmp(p->next->key, key)) { // the key is in the list of hMap->elements[index]
                log_debug("list element matches.");
                return p->next->value;
            }

            p = p->next;
        }

        log_error("HashMapGet() return, does not find.");
        return MAP_MISS;

    } else {

        log_error("keyToIndex() is -1, wrong HashMapGet() return.");
        return MAP_MISS;
    }
}

unsigned long int stringToInt(char *str) {

    unsigned long int intaddr;
    if (str == NULL) {
        log_error("Error in stringToInt(), empty str.");
        return STATUS_ERROR;

    } else {
        intaddr = strtol(str, NULL, 16);
        return intaddr;
    }
}

unsigned long currentSlotSize() {
    HashMap *ph = hMap;
    if (ph == NULL) {

        log_error("Error in currentSlotSize(), empty hMap.");
        return STATUS_ERROR;

    } else {

        return ph->size;
    }

}

unsigned long currentElemSize() {

    HashMap *ph = hMap;

    if (ph == NULL) {

        log_error("Error in currentElemSize(), empty hMap.");
        return STATUS_ERROR;

    } else {

        return ph->totalElem;
    }
}
