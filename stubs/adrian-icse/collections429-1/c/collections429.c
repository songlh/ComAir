#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define DEFAULT_CAPACITY 100000
#define DEFAULT_THRESHOLD 80000
#define MAXIMUM_CAPACITY 1 << 30
#define DEFAULT_LOAD_FACTOR 0.75f

typedef struct stArrayList
{
	int size;
	int capacity;
	int * data;
} ArrayList;

int max(int a, int b)
{
	if(a >= b)
	{
		return a;
	}

	return b;
}

void ArrayList_ArrayList1(ArrayList * pArray, int capacity)
{
	if(capacity < 0)
	{
		printf("IllegalArgumentException!\n");
		exit(-1);
	}

	pArray->data = (int *)malloc(sizeof(int) * capacity);

	if(pArray->data == NULL)
	{
		printf("Cannot Allocate Memory!\n");
		exit(-1);
	}

	pArray->capacity = capacity;
	pArray->size = 0;

}

void ArrayList_ArrayList0(ArrayList * pArray)
{
	ArrayList_ArrayList1(pArray, DEFAULT_CAPACITY);
}

void ArrayList_ensureCapacity(ArrayList * pArray, int minCapacity)
{
	int current = pArray->capacity;
	
	if(minCapacity > current)
	{
		pArray->capacity = max(current * 2, minCapacity);
		int * newData = (int *)malloc(max(current * 2, minCapacity) * sizeof(int));
		int * oldData = pArray->data;
		memcpy(newData, oldData, pArray->size * sizeof(int));
		pArray->data = newData;
		free(oldData);
	}
}

int ArrayList_size(ArrayList * pArray)
{
	return pArray->size;
}

bool ArrayList_isEmpty(ArrayList * pArray)
{
	return pArray->size == 0;
}

int ArrayList_indexOf(ArrayList * pArray, int e)
{
	int i = 0;

	for(i=0; i < pArray->size; i ++)
	{
		if(pArray->data[i] == e)
		{
			return i;
		}
	}

	return -1;
}

bool ArrayList_contains(ArrayList * pArray, int o)
{
	return ArrayList_indexOf(pArray, o) >= 0;
}

bool ArrayList_add1(ArrayList * pArray, int e)
{
	if(pArray->size == pArray->capacity )
	{	
		ArrayList_ensureCapacity(pArray, pArray->size + 1);
	}

	pArray->data[pArray->size++] = e;
	return true;
}

void ArrayList_rangeCheckForAdd(ArrayList * pArray, int index)
{
	if(index > pArray->size || index < 0)
	{
		printf("IndexOutOfBoundsException %d\n", index);
		exit(-1);
	}
}

void ArrayList_add2(ArrayList * pArray, int index, int element )
{
	ArrayList_rangeCheckForAdd(pArray, index);
	ArrayList_ensureCapacity(pArray, pArray->size + 1);

	int size = pArray->size;

	while(size > index)
	{
		pArray->data[size] = pArray->data[size-1];
		size --;
	}

	pArray->data[index] = element;
	pArray->size++;

}

void ArrayList_fastRemove(ArrayList * pArray, int index)
{
	int numMoved = pArray->size - index - 1;
	if(numMoved)
	{
		while(index < pArray->size - 1)
		{
			pArray->data[index] = pArray->data[index+1];
			index ++;
		}
	}

	pArray->size --;
}

bool ArrayList_remove(ArrayList * pArray, int o)
{
	int index = 0;
	for(index = 0; index < pArray->size; index ++ )
	{
		if(pArray->data[index] == o)
		{
			ArrayList_fastRemove(pArray, index);
			return true;
		}
	}

	return false;
}

bool ArrayList_batchRemove(ArrayList * pArray, ArrayList * c, bool complement)
{
	int * elementData = pArray->data;
	int r = 0, w = 0;
	bool modified = false;

	for(; r < pArray->size; r ++ )
	{
		if(ArrayList_contains(c, elementData[r]) == complement)
		{
			elementData[w++] = elementData[r];
		}
	}

	if(w != pArray->size)
	{
		pArray->size = w;
		modified = true;
	}

	return modified;
}

bool ArrayList_removeAll(ArrayList * pArray, ArrayList * c)
{
	return ArrayList_batchRemove(pArray, c, false);
}

void ArrayList_print(ArrayList * pArray)
{
	printf("size: %d\n", pArray->size);
	printf("capacity: %d\n", pArray->capacity);

	int i =0;
	for(; i < pArray->size && i < 50; i ++)
	{
		printf("%d ", (pArray->data[i]));
	}

	printf("\n");
}

void ArrayList_destroy(ArrayList * pArray)
{
	free(pArray->data);
}

typedef struct stHashEntry
{
	int key;
	int value;
	int hashCode;
	struct stHashEntry * next;
} HashEntry;

void HashEntry_HashEntry(HashEntry * pEntry, HashEntry * next, int hashCode, int key, int value)
{
	pEntry->next = next;
	pEntry->hashCode = hashCode;
	pEntry->key = key;
	pEntry->value = value;
	//printf("%d %d\n", pEntry->key, pEntry->value);
}

int hash(int key)
{
	int h = key;
	h = ((h >> 16) ^ h) * 0x45d9f3b;
	h = ((h >> 16) ^ h) * 0x45d9f3b;
	h = ((h >> 16) ^ h);
	return h;
}

typedef struct stAbstractHashedMap
{
	HashEntry ** data;
	float loadFactor;
	int threshold;
	int size;
	int capacity;

}AbstractHashedMap;


int AbstractHashedMap_calculateThreshold(int newCapacity, float factor)
{
	return (int)(newCapacity * factor);
}

int AbstractHashedMap_calculateNewCapacity(int proposedCapacity)
{
	int newCapacity = 1;
	if(proposedCapacity > MAXIMUM_CAPACITY)
	{
		newCapacity = MAXIMUM_CAPACITY;
	}
	else
	{
		while(newCapacity < proposedCapacity)
		{
			newCapacity <<= 1 ;
		}

		if(newCapacity > MAXIMUM_CAPACITY)
		{
			newCapacity = MAXIMUM_CAPACITY;
		}
	}

	return newCapacity;
}

void AbstractHashedMap_AbstractHashedMap3(AbstractHashedMap * pHashedMap, int initialCapacity, float loadFactor, int threshold)
{
	pHashedMap->loadFactor = loadFactor;
	pHashedMap->data = (HashEntry **)malloc(sizeof(HashEntry *) * initialCapacity);
	memset(pHashedMap->data, 0, sizeof(HashEntry *) * initialCapacity);
	pHashedMap->capacity = initialCapacity;
	pHashedMap->threshold = threshold;
	pHashedMap->size = 0;
}

void AbstractHashedMap_AbstractHashedMap2(AbstractHashedMap * pHashedMap, int initialCapacity, float loadFactor)
{
	pHashedMap->loadFactor = loadFactor;
	initialCapacity = AbstractHashedMap_calculateNewCapacity(initialCapacity);
	pHashedMap->threshold = AbstractHashedMap_calculateThreshold(initialCapacity, loadFactor);
	pHashedMap->data = (HashEntry **)malloc(sizeof(HashEntry *) * initialCapacity);
	memset(pHashedMap->data, 0, sizeof(HashEntry *) * initialCapacity);
	pHashedMap->capacity = initialCapacity;
	pHashedMap->size = 0;

}

void AbstractHashedMap_AbstractHashedMap1(AbstractHashedMap * pHashedMap, int initialCapacity)
{
	AbstractHashedMap_AbstractHashedMap2(pHashedMap, initialCapacity, DEFAULT_LOAD_FACTOR);
}

void AbstractHashedMap_AbstractHashedMap0(AbstractHashedMap * pHashedMap)
{
	AbstractHashedMap_AbstractHashedMap1(pHashedMap, DEFAULT_CAPACITY);
}


void AbstractHashedMap_addMapping(AbstractHashedMap * pHashedMap, int hashIndex, int hashCode, int key, int value)
{
	HashEntry * entry = (HashEntry *)malloc(sizeof(HashEntry));
	memset(entry, 0, sizeof(HashEntry));
	HashEntry_HashEntry(entry, pHashedMap->data[hashIndex], hashCode, key, value);
	pHashedMap->data[hashIndex] = entry;
	pHashedMap->size ++;
}

int hashIndex(int hashCode, int dataSize)
{
	return hashCode & (dataSize - 1);
}


void AbstractHashedMap_put(AbstractHashedMap * pHashedMap, int key, int value)
{
	int hashCode = hash(key);
	int index = hashIndex(hashCode, pHashedMap->capacity);
	HashEntry * entry = pHashedMap->data[index];

	while(entry != NULL)
	{
		if(entry->hashCode == hashCode && key == entry->key)
		{
			//int oldValue = entry->value;
			entry->value = value;
		}

		entry = entry->next;
	}

	AbstractHashedMap_addMapping(pHashedMap, index, hashCode, key, value);
}


void AbstractHashedMap_print(AbstractHashedMap * pHashedMap)
{
	printf("Size: %d\n", pHashedMap->size);
	printf("Capacity: %d\n", pHashedMap->capacity);

	int i = pHashedMap->capacity -1;

	for(; i >=0; i--)
	{
		HashEntry * entry = pHashedMap->data[i];
		while(entry != NULL)
		{
			printf("%d %d\n", entry->key, entry->value);
			entry = entry->next;
		}
	}
}


void AbstractHashedMap_destroy(AbstractHashedMap * pHashedMap)
{
	int i = pHashedMap->capacity - 1;
	for(; i >=0; i --)
	{
		HashEntry * entry = pHashedMap->data[i];
		HashEntry * n = entry;

		while(entry != NULL)
		{
			n = entry->next;
			free(n);
			entry = n;
		}
	}
}

typedef struct stHashIterator
{
	int hashIndex;
	HashEntry * last;
	HashEntry * next;
	AbstractHashedMap * parent;
} HashIterator;


void HashIterator_HashIterator(HashIterator * pIterator, AbstractHashedMap * pHashedMap)
{
	HashEntry ** data = pHashedMap->data;
	int i = pHashedMap->capacity;

	HashEntry * next = NULL;
	while(i>0 && next == NULL)
	{
		next = data[--i];
	}

	pIterator->next = next;
	pIterator->hashIndex = i;
	pIterator->parent = pHashedMap;
}

bool HashIterator_hashNext(HashIterator * pIterator)
{
	return pIterator->next != NULL;
}

HashEntry * HashIterator_nextEntry(HashIterator * pIterator)
{
	HashEntry * newCurrent = pIterator->next;
	HashEntry ** data = pIterator->parent->data;
	int i = pIterator->hashIndex;
	HashEntry * n = newCurrent->next;

	while(n == NULL && i > 0)
	{
		n = data[--i];
	}

	pIterator->next = n;
	pIterator->hashIndex = i;
	pIterator->last = newCurrent;
	return newCurrent;
}


bool contains(AbstractHashedMap * pHashedMap, int k)
{
	//HashIterator * pIterator = (HashIterator *)malloc(sizeof(HashIterator));
	//HashIterator_HashIterator(pIterator, pHashedMap);

	HashEntry * pResult = NULL;

	//while(HashIterator_hashNext(pIterator))
	//{
	//	HashEntry * entry = HashIterator_nextEntry(pIterator);
	//	if(entry->key == k)
	//	{
	//		pResult = entry;
	//		break;
	//	}	
	//}

	//free(pIterator);
	HashEntry ** data = pHashedMap->data;
	int i = pHashedMap->capacity;

	HashEntry * next = NULL;

	while(i>0 && next == NULL)
	{
		next = data[--i];
	}

	while(next != NULL)
	{
		if(next->key == k)
		{
			pResult = next;
			break;
		}

		next = next->next;
		while(next == NULL && i > 0 )
		{
			next = data[--i];
		}
	}



	if(pResult == NULL)
	{
		return false;
	}
	else
	{
		return true;
	}
}


bool containsAll(AbstractHashedMap * pHashedMap, ArrayList * c)
{
	int i = 0;
	for(; i < c->size; i ++ )
	{
		if(!contains(pHashedMap, c->data[i]))
		{
			return false;
		}
	}

	return true;
}


int main(int argc, char ** argv)
{
	int size = atoi(argv[1]);
	int i = 0;
	AbstractHashedMap * multi = (AbstractHashedMap *)malloc(sizeof(AbstractHashedMap));
	AbstractHashedMap_AbstractHashedMap0(multi);

	for(i=0; i < size; i ++)
	{
		AbstractHashedMap_put(multi, i, i);
	}

	ArrayList * toContain = (ArrayList *)malloc(sizeof(ArrayList));
	ArrayList_ArrayList0(toContain);

	for(i = size - 1; i >= 0; i --)
	{
		ArrayList_add1(toContain, i);
	}

	bool bResult = containsAll(multi, toContain);
	printf("%d\n", bResult);
	//AbstractHashedMap_print(multi);
	printf("\n");

	ArrayList_destroy(toContain);
	free(toContain);
	AbstractHashedMap_destroy(multi);
	free(multi);
}