#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <limits.h>

#define DEFAULT_CAPACITY 1000000
#define DEFAULT_INITIAL_CAPACITY 1000000
#define MAXIMUM_CAPACITY 1 << 30
#define DEFAULT_LOAD_FACTOR 0.75


typedef	long long word;		/* "word" used for optimal copy speed */

#define	wsize	sizeof(word)
#define	wmask	(wsize - 1)

/*
 * Copy a block of memory, handling overlap.
 * This is the routine that actually implements
 * (the portable versions of) bcopy, memcpy, and memmove.
 */
void *
memmove_new( void *dst0, const void *src0, register size_t length )
{
	register char *dst = dst0;
	register const char *src = src0;
	register size_t t;

	if (length == 0 || dst == src)		/* nothing to do */
		goto done;

	/*
	 * Macros: loop-t-times; and loop-t-times, t>0
	 */
#define	TLOOP(s) if (t) TLOOP1(s)
#define	TLOOP1(s) do { s; } while (--t)

	if ((unsigned long)dst < (unsigned long)src) 
	{
		/*
		 * Copy forward.
		 */
		t = (int)src;	/* only need low bits */
		if ((t | (int)dst) & wmask) 
		{
			/*
			 * Try to align operands.  This cannot be done
			 * unless the low bits match.
			 */
			if ((t ^ (int)dst) & wmask || length < wsize)
				t = length;
			else
				t = wsize - (t & wmask);
			length -= t;
			TLOOP1(*dst++ = *src++);
		}
		/*
		 * Copy whole words, then mop up any trailing bytes.
		 */
		t = length / wsize;
		TLOOP(*(word *)dst = *(word *)src; src += wsize; dst += wsize);
		t = length & wmask;
		TLOOP(*dst++ = *src++);
	}
	else 
	{
		/*
		 * Copy backwards.  Otherwise essentially the same.
		 * Alignment works as before, except that it takes
		 * (t&wmask) bytes to align, not wsize-(t&wmask).
		 */
		src += length;
		dst += length;
		t = (int)src;
		if ((t | (int)dst) & wmask) 
		{
			if ((t ^ (int)dst) & wmask || length <= wsize)
				t = length;
			else
				t &= wmask;
			length -= t;
			TLOOP1(*--dst = *--src);
		}
		t = length / wsize;
		TLOOP(src -= wsize; dst -= wsize; *(word *)dst = *(word *)src);
		t = length & wmask;
		TLOOP(*--dst = *--src);
	}
	done:
	return (dst0);
}

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


typedef struct stMapEntry 
{
	int key;
	int value;
	int hash;
	struct stMapEntry * next;
} MapEntry;

void MapEntry_MapEntry(MapEntry * pEntry, int h, int k, int v, MapEntry * n)
{
	pEntry->value = v;
	pEntry->next = n;
	pEntry->key = k;
	pEntry->hash = h;
}

int MapEntry_getKey(MapEntry * pEntry)
{
	return pEntry->key;
}

int MapEntry_getValue(MapEntry * pEntry)
{
	return pEntry->value;
}

int MapEntry_setValue(MapEntry * pEntry, int value)
{
	int oldValue = pEntry->value;
	pEntry->value = value;
	return oldValue;
}

bool MapEntry_equals(MapEntry * p1, MapEntry * p2)
{
	if(p1->key == p2->key && p1->value == p2->value)
	{
		return true;
	}

	return false;
}

typedef struct stHashMap
{
	int size;
	int threshold;
	int capacity;
	float loadFactor;
	MapEntry ** table;

} HashMap;

void HashMap_HashMap2( HashMap * pHash, int initialCapacity, float loadFactor)
{
	if(initialCapacity < 0)
	{
		printf("Illegal initial capacity %d\n", initialCapacity);
		exit(-1);
	}

	if(initialCapacity > MAXIMUM_CAPACITY)
	{
		initialCapacity = MAXIMUM_CAPACITY;
	}

	if(loadFactor <= 0)
	{
		printf("Illegal load factor: %f\n", loadFactor);
		exit(-1);
	}

	pHash->capacity = 1;
	while(pHash->capacity < initialCapacity)
	{
		pHash->capacity <<= 1;
	}

	pHash->loadFactor = loadFactor;
	pHash->threshold = (int)(pHash->capacity * loadFactor);
	pHash->table = (MapEntry **) malloc(sizeof(MapEntry *) * pHash->capacity);
	memset(pHash->table, 0, sizeof(MapEntry *) * pHash->capacity);
	pHash->size = 0;	
}

void HashMap_HashMap1(HashMap * pHash, int initialCapacity)
{
	HashMap_HashMap2(pHash, initialCapacity,  DEFAULT_LOAD_FACTOR);
}

void HashMap_HashMap0(HashMap * pHash)
{
	pHash->loadFactor = DEFAULT_LOAD_FACTOR;
	pHash->threshold = (int)(DEFAULT_INITIAL_CAPACITY * DEFAULT_LOAD_FACTOR);
	pHash->capacity = DEFAULT_INITIAL_CAPACITY;
	pHash->table = (MapEntry **)malloc(sizeof(MapEntry *) * DEFAULT_INITIAL_CAPACITY);
	memset(pHash->table, 0, sizeof(MapEntry *) * DEFAULT_INITIAL_CAPACITY );
	pHash->size = 0;
}

int HashMap_hash(int h)
{
	//h ^= (h >> 20) ^ (h >> 12);
	//return h ^ (h >> 7) ^ (h >> 4);
	h = ((h >> 16) ^ h) * 0x45d9f3b;
	h = ((h >> 16) ^ h) * 0x45d9f3b;
	h = ((h >> 16) ^ h);
	return h;
}

int HashMap_indexFor(int h, int length)
{
	return h & (length -1);
}

MapEntry * HashMap_getEntry(HashMap * pHash, int key)
{
	int hash = HashMap_hash(key);
	MapEntry * e = pHash->table[HashMap_indexFor(hash, pHash->capacity)];

	for(; e != NULL; e = e->next)
	{
		if(e->hash == hash && e->key == key)
		{
			return e;
		}
	}

	return NULL;
}

int HashMap_get(HashMap * pHash, int key)
{
	int hash = HashMap_hash(key);
	MapEntry * e = pHash->table[HashMap_indexFor(hash, pHash->capacity)];

	for(; e != NULL; e = e->next)
	{	
		if(e->hash == hash && e->key == key)
		{
			return e->value;
		}
	}

	return INT_MIN;
}

bool HashMap_containsKey(HashMap * pHash, int key)
{
	return HashMap_getEntry(pHash, key) != NULL;
}

void HashMap_transfer(HashMap * pHash, MapEntry ** newTable, int newCapacity)
{
	MapEntry ** src = pHash->table;
	int j =0;

	for(j = 0; j < pHash->capacity; j ++)
	{
		MapEntry * e = src[j];

		if(e != NULL)
		{
			src[j] = NULL;
			do 
			{
				MapEntry * next = e->next;
				int i = HashMap_indexFor(e->hash, newCapacity);
				e->next = newTable[i];
				newTable[i] = e;
				e = next;
			} while(e != NULL);
		}
	}
}

void HashMap_resize(HashMap * pHash, int newCapacity)
{
	MapEntry ** oldTable = pHash->table;
	int oldCapacity = pHash->capacity;

	if(oldCapacity == MAXIMUM_CAPACITY)
	{
		pHash->threshold = INT_MAX;
		return;
	}

	MapEntry ** newTable = (MapEntry **)malloc(sizeof(MapEntry *) * newCapacity);
	memset(newTable, 0, sizeof(MapEntry *) * newCapacity);
	HashMap_transfer(pHash, newTable, newCapacity);
	free(pHash->table);
	pHash->table = newTable;
	pHash->capacity = newCapacity;
	pHash->threshold = (int)(newCapacity * pHash->loadFactor);
}

void HashMap_addEntry(HashMap * pHash, int hash, int key, int value, int bucketIndex)
{
	MapEntry * e = pHash->table[bucketIndex];
	MapEntry * pEntry = (MapEntry *)malloc(sizeof(MapEntry));
	MapEntry_MapEntry(pEntry, hash, key, value, e);
	pHash->table[bucketIndex] = (MapEntry *)pEntry;

	if(pHash->size++ >= pHash->threshold)
	{
		HashMap_resize(pHash, 2 * pHash->capacity);
	}
}

bool HashMap_put(HashMap * pHash, int key, int value)
{
	int hash = HashMap_hash(key);
	int i = HashMap_indexFor(hash, pHash->capacity);
	MapEntry * e = pHash->table[i];

	for(; e != NULL; e = e->next)
	{
		if(e->hash == hash && key == e->key)
		{
			e->value = value;
			return false;
		}
	}

	HashMap_addEntry(pHash, hash, key, value, i);
	return true;
}

MapEntry * HashMap_removeEntryForKey(HashMap * pHash, int key)
{
	int hash = HashMap_hash(key);
	int i = HashMap_indexFor(hash, pHash->capacity);

	MapEntry * prev = pHash->table[i];
	MapEntry * e = prev;

	while(e != NULL)
	{
		MapEntry * next = e->next;

		if(e->hash == hash && e->key == key)
		{
			pHash->size--;

			if(prev == e)
			{
				pHash->table[i] = next;
			}
			else
			{
				prev->next = next;
			}

			//return e;
			break;
		}

		prev = e;
		e = next;
	}

	if(e != NULL)
	{
		free(e);
	}

	return e;
}

bool HashMap_remove(HashMap * pHash, int key)
{
	MapEntry * e = HashMap_removeEntryForKey(pHash, key);
	return e != NULL;
}

void HashMap_print(HashMap * pHash)
{
	int i = 0;

	printf("capacity: %d\n", pHash->capacity);

	for(i=0; i < pHash->capacity; i ++)
	{
		//if(pHash->table[i] != NULL)
		//{
		//	printf("%d %d %d\n", i, pHash->table[i]->key, pHash->table[i]->value);
		//}

		MapEntry * e = pHash->table[i];

		for(; e != NULL; e = e->next)
		{
			printf("%d %d %d\n", i, e->key, e->value);
		}

	}
}

void HashMap_destroy(HashMap * pHash)
{
	int i = 0;

	for(i = 0; i < pHash->capacity; i ++ )
	{
		MapEntry * c = pHash->table[i];
		MapEntry * n = c;

		while(c != NULL)
		{
			n = c->next;
			free(c);
			c = n;
		}
	}

	free(pHash->table);
}

typedef struct stHashSet
{
	HashMap * map;
} HashSet;

void HashSet_HashSet(HashSet * pSet)
{
	pSet->map = (HashMap *)malloc(sizeof(HashMap));
	HashMap_HashMap2(pSet->map, DEFAULT_INITIAL_CAPACITY, DEFAULT_LOAD_FACTOR);
}

void HashSet_destroy(HashSet * pSet)
{
	HashMap_destroy(pSet->map);
	free(pSet->map);
}

bool HashSet_add(HashSet * pSet, int e)
{
	return HashMap_put(pSet->map, e, e);
}

bool HashSet_addAll(HashSet * pSet, ArrayList * c)
{
	bool modified = false;
	int i = 0;
	for(i=0; i < c->size; i ++)
	{
		if(HashSet_add(pSet, c->data[i]))
		{
			modified = true;
		}
	}

	return modified;
}


void HashSet_HashSet1(HashSet * pSet, ArrayList * c)
{
	pSet->map = (HashMap *)malloc(sizeof(HashMap));
	HashMap_HashMap1(pSet->map, max((int)(c->size / 0.75) + 1, 16));
	HashSet_addAll(pSet, c);
}

bool HashSet_contains(HashSet * pSet, int o)
{
	return HashMap_containsKey(pSet->map, o);
}

bool HashSet_remove(HashSet * pSet, int o)
{
	return HashMap_remove(pSet->map, o);
}

bool HashSet_removeAll(HashSet * pSet, ArrayList * c)
{
	bool modified = false;
	int i;

	if(pSet->map->size > c->size)
	{
		for(i = 0; i < c->size; i ++)
		{
			modified |= HashSet_remove(pSet, c->data[i]);
		}
	}
	else
	{
		for(i = 0; i < pSet->map->capacity; i ++ )
		{
			MapEntry * current = pSet->map->table[i];
			MapEntry * prev = current;
			MapEntry * n = current;

			while(current != NULL)
			{
				n = current->next;

				if(ArrayList_contains(c, current->key))
				{
					if(prev == current)
					{
						pSet->map->table[i] = n;
					}
					else
					{
						prev->next = n;
					}

					free(current);
					modified = true;
					current = n;
					prev = n;
				}
				else
				{
					prev = current;
					current = n;
				}
			}
		}
	}

	return modified;
}

bool HashSet_retainAll(HashSet * pSet, ArrayList * c)
{
	bool modified = false;
	int i;
	for(i = 0; i < pSet->map->capacity; i ++ )
	{
		MapEntry * current = pSet->map->table[i];
		MapEntry * prev = current;
		MapEntry * n = current;

		while(current != NULL)
		{
			n = current->next;

			if(!ArrayList_contains(c, current->key))
			{
				if(prev == current)
				{
					pSet->map->table[i] = n;
				}
				else
				{
					prev->next = n;
				}

				free(current);
				modified = true;
				current = n;
				prev = n;
			}
			else
			{
				prev = current;
				current = n;
			}
		}
	}

	return modified;
}


typedef struct stVectorSet
{
	HashSet * set;
	int * elementData;
	int elementCount;
	int capacityIncrement;
	int capacity;

} VectorSet;

void VectorSet_VectorSet2(VectorSet * pVector, int initialCapacity, int capacityIncrement)
{
	if(initialCapacity < 0)
	{
		printf("Illegal Capacity: %d\n", initialCapacity);
		exit(-1);
	}

	pVector->elementData = (int *)malloc(sizeof(int) * initialCapacity);
	pVector->capacityIncrement = capacityIncrement;
	pVector->elementCount = 0;
	pVector->capacity = initialCapacity;
	pVector->set = (HashSet *)malloc(sizeof(HashSet));
	HashSet_HashSet(pVector->set);
}

void VectorSet_VectorSet1(VectorSet * pVector, int initialCapacity)
{
	VectorSet_VectorSet2(pVector, initialCapacity, 0);
}

void VectorSet_VectorSet0(VectorSet * pVector)
{
	VectorSet_VectorSet1(pVector, DEFAULT_CAPACITY);
}

void VectorSet_ensureCapacityHelper(VectorSet * pVector, int minCapacity)
{
	int oldCapacity = pVector->capacity;
	
	if(minCapacity > oldCapacity)
	{
		int * oldData = pVector->elementData;
		int newCapacity = (pVector->capacityIncrement > 0) ? (oldCapacity + pVector->capacityIncrement) : (oldCapacity * 2);

		if(newCapacity < minCapacity)
		{
			newCapacity = minCapacity;
		}	

		pVector->elementData = (int *)malloc(sizeof(int) * newCapacity);
		memcpy(pVector->elementData, oldData, sizeof(int) * pVector->elementCount);
		free(oldData);
	}
}

void VectorSet_ensureCapacity(VectorSet * pVector, int minCapacity)
{
	VectorSet_ensureCapacityHelper(pVector, minCapacity);
}

void VectorSet_doAdd(VectorSet * pVector, int index, int o)
{
	if(HashSet_add(pVector->set, o))
	{
		int count = pVector->elementCount;
		VectorSet_ensureCapacity(pVector, count + 1);

		if(index != count)
		{
			//memcpy(pVector->elementData + index + 1, pVector->elementData + index, sizeof(int) * (count - index) );

			//memmove_new(pVector->elementData + index + 1, pVector->elementData + index, sizeof(int) * (count - index));
			
			while(count > index)
			{
				pVector->elementData[count] = pVector->elementData[count-1];
				count --;
			}
		}

		pVector->elementData[index] = o;
		pVector->elementCount ++;
	}
}	

void VectorSet_add2(VectorSet * pVector, int index, int o)
{
	VectorSet_doAdd(pVector, index, o);
}

bool VectorSet_add1(VectorSet * pVector, int o)
{
	if(!HashSet_contains(pVector->set, o))
	{
		VectorSet_doAdd(pVector, pVector->elementCount, o);
		return true;
	}

	return false;
}


bool VectorSet_addAll2(VectorSet * pVector, int index, ArrayList * c)
{	
	int i;
	bool changed = false;
	for(i = 0; i < c->size; i ++ )
	{
		int o = c->data[i];
		if(!HashSet_contains(pVector->set, o))
		{
			VectorSet_doAdd(pVector, index++, o);
			changed = true;
		}	
	}

	return changed;
}

void VectorSet_destroy(VectorSet * pVector)
{
	HashSet_destroy(pVector->set);
	free(pVector->set);
	free(pVector->elementData);
}


void VectorSet_print(VectorSet * pVector)
{
	int i = 0;

	printf("elementCount: %d\n", pVector->elementCount);

	for(; i < pVector->elementCount; i ++ )
	{
		printf("%d ", pVector->elementData[i]);
	}

	printf("\n");
}

int main(int argc, char ** argv)
{
	VectorSet * pVector = (VectorSet *)malloc(sizeof(VectorSet));
	VectorSet_VectorSet0(pVector);

	int size = atoi(argv[1]);
	int i;
	for(i = 0; i < size; i ++ )
	{
		VectorSet_add1(pVector, i);
	}

	ArrayList * list = (ArrayList *)malloc(sizeof(ArrayList));
	ArrayList_ArrayList0(list);

	for(i = size; i < 2 * size; i ++ )
	{
		ArrayList_add1(list, i);
	}

	VectorSet_addAll2(pVector, 10, list);

	//VectorSet_print(pVector);


	ArrayList_destroy(list);
	free(list);
	VectorSet_destroy(pVector);
	free(pVector);
}

