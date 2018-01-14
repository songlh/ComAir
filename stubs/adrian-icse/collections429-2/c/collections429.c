#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


#define DEFAULT_BUCKETS 255


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



typedef struct stNode
{
	int key;
	int value;
	struct stNode * next;

} Node;

void Node_Node(Node * pNode, int key, int value, Node * next)
{
	pNode->key = key;
	pNode->value = value;
	pNode->next = next;
}

typedef struct stStaticBucketMap
{
	Node ** buckets;

} StaticBucketMap;

void StaticBucketMap_StaticBucketMap(StaticBucketMap * pMap)
{
	pMap->buckets = (Node **)malloc(sizeof(Node *) * DEFAULT_BUCKETS);
	memset(pMap->buckets, 0, sizeof(Node *) * DEFAULT_BUCKETS);
}


void StaticBucketMap_print(StaticBucketMap * pMap)
{
	int i = 0;
	int total = 0;
	for(; i < DEFAULT_BUCKETS; i ++ )
	{
		Node * pNode = pMap->buckets[i];
		int count = 0;

		while(pNode != NULL)
		{
			printf("%d ", pNode->value);
			pNode = pNode->next;
			
		}

		
		//printf("\n");
	}

	printf("total: %d\n", total);
}



void StaticBucketMap_destroy(StaticBucketMap * pMap)
{
	int i = 0 ;

	for(; i < DEFAULT_BUCKETS; i ++ )
	{
		Node * pNode = pMap->buckets[i];
		Node * next ;

		while(pNode != NULL)
		{
			next = pNode->next;
			free(pNode);
			pNode = next;
		}

	}

	free(pMap->buckets);
}

int getHash(int key)
{
	int h = key;
	h = ((h >> 16) ^ h) * 0x45d9f3b;
	h = ((h >> 16) ^ h) * 0x45d9f3b;
	h = ((h >> 16) ^ h);
	h %= DEFAULT_BUCKETS;
	return (h < 0) ? h * -1 : h;
}

void StaticBucketMap_put(StaticBucketMap * pMap, int key, int value)
{
	int hash = getHash(key);
	Node * n = pMap->buckets[hash];

	if(n == NULL)
	{
		n = (Node *)malloc(sizeof(Node));
		n->key = key;
		n->value = value;
		n->next = NULL;
		pMap->buckets[hash] = n;

	}

	Node * next = n;
	for(; next != NULL; next = next->next)
	{
		n = next;
		if(n->key == key )
		{
			n->value = value;
			return;
		}
	}

	Node * newNode = (Node *)malloc(sizeof(Node));
	newNode->key = key;
	newNode->value = value;
	newNode->next = NULL;
	n->next = newNode;
}

bool contains(StaticBucketMap * pMap, int k)
{
	int i = 0;
	Node * node = pMap->buckets[0];
	Node * pResult = NULL;

	while(node != NULL || i < DEFAULT_BUCKETS)
	{
		if(node == NULL)
		{
			node = pMap->buckets[++i];
			continue;
		}

		if(node->key == k)
		{
			pResult = node;
			break;
		}

		node = node->next;
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

bool containsAll(StaticBucketMap * pMap, ArrayList * c)
{
	int i ;
	for(i = 0; i < c->size; i ++ )
	{
		if(!contains(pMap, c->data[i]))
		{
			return false;
		}
	}

	return true;
}

int main(int argc, char ** argv)
{
	int size = atoi(argv[1]);
	int i;
	StaticBucketMap * multi = (StaticBucketMap *)malloc(sizeof(StaticBucketMap));
	StaticBucketMap_StaticBucketMap(multi);

	for(i = 0; i < size; i ++ )
	{
		StaticBucketMap_put(multi, i , i );
	}

	ArrayList * toContain = (ArrayList *)malloc(sizeof(ArrayList));
	ArrayList_ArrayList0(toContain);

	//StaticBucketMap_print(multi);

	for(i = size - 1; i >=0; i--)
	{
		ArrayList_add1(toContain, i);
	}

	bool ret = containsAll(multi, toContain);
	printf("%d\n", ret);

	StaticBucketMap_destroy(multi);
	free(multi);

	ArrayList_destroy(toContain);
	free(toContain);

}