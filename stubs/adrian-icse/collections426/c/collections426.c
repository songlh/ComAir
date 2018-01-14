#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <limits.h>


#define DEFAULT_CAPACITY 100000
#define DEFAULT_INITIAL_CAPACITY 100000
#define MAXIMUM_CAPACITY 1 << 30
#define DEFAULT_LOAD_FACTOR 0.75


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

void ArrayList_clear(ArrayList * pArray)
{
	pArray->size = 0;
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
	HashMap_HashMap0(pSet->map);
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
	HashMap_HashMap1(pSet->map, max((int)(c->size / DEFAULT_LOAD_FACTOR) + 1, DEFAULT_INITIAL_CAPACITY));
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
						prev = n;
					}
					else
					{
						prev->next = n;
					}

					free(current);
					modified = true;
					current = n;
					//prev = n;
					pSet->map->size --;
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
					prev = n;
				}
				else
				{
					prev->next = n;
				}

				free(current);
				modified = true;
				current = n;
				//prev = n;
				pSet->map->size--;
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


typedef struct stListOrderedSet
{
	HashSet * collection;
	ArrayList * setOrder;

} ListOrderedSet;

void ListOrderedSet_ListOrderedSet(ListOrderedSet * pSet)
{
	pSet->collection = (HashSet *)malloc(sizeof(HashSet));
	HashSet_HashSet(pSet->collection);

	pSet->setOrder = (ArrayList *)malloc(sizeof(ArrayList));
	ArrayList_ArrayList0(pSet->setOrder);
}

void ListOrderedSet_destroy(ListOrderedSet * pSet)
{
	HashSet_destroy(pSet->collection);
	free(pSet->collection);

	ArrayList_destroy(pSet->setOrder);
	free(pSet->setOrder);
}

bool ListOrderedSet_add(ListOrderedSet * pSet, int object)
{
	if(HashSet_add(pSet->collection, object))
	{
		ArrayList_add1(pSet->setOrder, object);
		return true;
	}

	return false;
}

bool ListOrderedSet_retainAll(ListOrderedSet * pSet, ArrayList * coll)
{
	bool result = HashSet_retainAll(pSet->collection, coll);
	if(result == false)
	{
		return false;
	}

	if(pSet->collection->map->size == 0)
	{
		ArrayList_clear(pSet->setOrder);
	}
	else
	{
		int i = 0;
		for( i = 0; i < pSet->setOrder->size; i ++ )
		{
			if(!HashSet_contains(pSet->collection, pSet->setOrder->data[i]))
			{
				ArrayList_fastRemove(pSet->setOrder, i);
				i --;
			}
		}
	}

	return result;
}

int main(int argc, char ** argv)
{
	int size = atoi(argv[1]);
	int i;

	ArrayList * list = (ArrayList *)malloc(sizeof(ArrayList));
	ArrayList_ArrayList0(list);

	for(i=0; i < size; i ++)
	{
		ArrayList_add1(list, i);
	}

	ListOrderedSet * set = (ListOrderedSet *)malloc(sizeof(ListOrderedSet));
	ListOrderedSet_ListOrderedSet(set);
	
	for(i=size; i < size * 2; i ++)
	{
		ListOrderedSet_add(set, i);
	}

	ListOrderedSet_retainAll(set, list);
	//HashMap_print(set->collection->map);
	//ArrayList_print(set->setOrder);

	ListOrderedSet_destroy(set);
	free(set);

	ArrayList_destroy(list);
	free(list);
}