
#ifndef SLOTLENGTH
#define SLOTLENGTH 65536  // a large prime use
#endif

#ifndef MAP_MISS
#define MAP_MISS -3
#endif

#ifndef MAP_FULL
#define MAP_FULL -2
#endif

#ifndef MAP_EXCEED
#define MAP_EXCEED -1
#endif

#ifndef MAP_SUCCESS
#define MAP_SUCCESS 0
#endif


typedef struct _hashMapElem {
    char *key;  // indicate the address of the memory cell
    unsigned int value;   // indicate the latest access of the memory cell
    struct _hashMapElem *next; // linked to the next element
    int hash;
} hashMapElem;


typedef struct _hashMap {
    struct _hashMapElem **elements; // a large prime as the length of the element slots
    unsigned long size; // size of the taken slots
    unsigned long totalElem;
} HashMap;


int createHashMap();


void destroyHashMap();


int hashMapRemove(char *key);


unsigned int keyToIndex(char *key);


int hashMapPut(char *key, unsigned int value);


int hashMapGet(char *key);


unsigned long int stringToInt(char *str);


unsigned long currentSlotSize();


unsigned long currentElemSize();
