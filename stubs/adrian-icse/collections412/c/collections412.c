#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <limits.h>

#define DEFAULT_CAPACITY 1000000
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
		//while(index < pArray->size - 1)
		//{
		//	pArray->data[index] = pArray->data[index+1];
		//	index ++;
		//}

		int * dst = pArray->data + index;
		int * src = pArray->data + index + 1;
		int count = pArray->size  - index - 1;

		while(count-->0)
		{
			*dst ++ = *src ++;
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

bool ArrayList_addAll(ArrayList * pArray, ArrayList * c)
{
	int numNew = c->size;
	ArrayList_ensureCapacity(pArray, pArray->size + numNew);
	memcpy(pArray->data + pArray->size, c->data, numNew * sizeof(int));
	pArray->size += numNew;
	return numNew != 0;
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

ArrayList * CollectionUtils_substract(ArrayList * a, ArrayList * b, bool p)
{
	ArrayList * list = (ArrayList *)malloc(sizeof(ArrayList));
	ArrayList_ArrayList0(list);

	ArrayList_addAll(list, a);

	int i ;

	for(i = 0; i < b->size; i ++ )
	{
		if(p)
		{
			ArrayList_remove(list, b->data[i]);
		}
	}

	return list;
}

int main(int argc, char ** argv)
{
	int size = atoi(argv[1]);
	int i ;
	ArrayList * one = (ArrayList *)malloc(sizeof(ArrayList));
	ArrayList_ArrayList0(one);

	for(i = 0; i < 2 * size; i ++)
	{
		ArrayList_add1(one, i);
	}

	ArrayList * two = (ArrayList *)malloc(sizeof(ArrayList));
	ArrayList_ArrayList0(two);

	for(i = size; i < 2 * size; i ++)
	{
		ArrayList_add1(two, i);
	}

	ArrayList * temp = CollectionUtils_substract(one, two, true);
	//ArrayList_print(temp);
	ArrayList_destroy(temp);
	free(temp);

	ArrayList_destroy(one);
	free(one);

	ArrayList_destroy(two);
	free(two);
}
