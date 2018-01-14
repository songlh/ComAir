#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#define DEFAULT_CAPACITY 100000

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


typedef struct stIdentifyStack
{
	int * elementData;
	int elementCount;
	int capacityIncrement;
	int capacity;

} IdentifyStack;

void IdentifyStack_IdentifyStack2(IdentifyStack * pStack,  int initialCapacity, int capacityIncrement)
{
	if(initialCapacity < 0)
	{
		printf("Illegal Capacity: %d\n", initialCapacity);
		exit(0);
	}

	pStack->elementData = (int *)malloc(sizeof(int) * initialCapacity);
	pStack->capacityIncrement = capacityIncrement;
	pStack->capacity = initialCapacity;
	pStack->elementCount = 0;
}

void IdentifyStack_IdentifyStack1( IdentifyStack * pStack, int initialCapacity)
{
	IdentifyStack_IdentifyStack2(pStack, initialCapacity, 0);
}

void IdentifyStack_IdentifyStack0(IdentifyStack * pStack)
{
	IdentifyStack_IdentifyStack1(pStack, DEFAULT_CAPACITY);
}

void IdentifyStack_print(IdentifyStack * pStack)
{
	int i = 0;
	for(; i < pStack->elementCount; i ++)
	{
		printf("%d ", pStack->elementData[i]);
	}

	printf("\n");
}


void IdentifyStack_destroy(IdentifyStack * pStack)
{
	free(pStack->elementData);
}

void IdentifyStack_ensureCapacityHelper(IdentifyStack * pStack, int minCapacity)
{
	int oldCapacity = pStack->capacity;
	if(minCapacity > oldCapacity)
	{
		int * oldData = pStack->elementData;
		int newCapacity = (pStack->capacityIncrement > 0) ? (oldCapacity + pStack->capacityIncrement) : (oldCapacity * 2);

		if(newCapacity < minCapacity)
		{
			newCapacity = minCapacity;
		}

		pStack->elementData = (int *)malloc(newCapacity * sizeof(int));
		memcpy(pStack->elementData, oldData, pStack->elementCount * sizeof(int));
	}
}

void IdentifyStack_ensureCapacity(IdentifyStack * pStack, int minCapacity)
{
	IdentifyStack_ensureCapacityHelper(pStack, minCapacity);
}

void IdentifyStack_addElement(IdentifyStack * pStack, int obj)
{
	IdentifyStack_ensureCapacity(pStack, pStack->elementCount + 1);
	pStack->elementData[pStack->elementCount++] = obj;
}

int IdentifyStack_push(IdentifyStack * pStack, int item)
{
	IdentifyStack_addElement(pStack, item);
	return item;
}

int IdentifyStack_indexOf2(IdentifyStack * pStack, int o, int index)
{
	int i;

	for(i = index; i < pStack->elementCount; i ++ )
	{
		if( o == pStack->elementData[i])
		{
			return i;
		}
	}

	return -1;
}

int IdentifyStack_indexOf1( IdentifyStack * pStack, int o)
{
	return IdentifyStack_indexOf2(pStack, o, 0);
}

bool IdentifyStack_contains(IdentifyStack * pStack, int o)
{
	return IdentifyStack_indexOf2(pStack, o, 0) >= 0;
}

void IdentifyStack_removeElementAt(IdentifyStack * pStack, int index)
{
	if(index >= pStack->elementCount)
	{
		printf("%d >= %d\n", index, pStack->elementCount);
		exit(-1);
	}	
	else if(index < 0)
	{
		printf("Array Index Out Of Bounds Exception: %d\n", index);
		exit(-1);
	}

	int j = pStack->elementCount - index - 1;
	
	if(j > 0)
	{
		while(index < pStack->elementCount - 1)
		{
			pStack->elementData[index] = pStack->elementData[index + 1];
			index ++;
		}
	}

	pStack->elementCount --;

}

bool IdentifyStack_removeElement(IdentifyStack * pStack, int obj)
{
	int i = IdentifyStack_indexOf1(pStack, obj);

	if(i >= 0)
	{
		IdentifyStack_removeElementAt(pStack, i);
		return true;
	}

	return false;
}

bool IdentifyStack_remove(IdentifyStack * pStack, int o)
{
	return IdentifyStack_removeElement(pStack, o);
}

bool IdentifyStack_removeAll(IdentifyStack * pStack, ArrayList * c )
{
	bool modified = false;

	int i = 0;

	for(; i < pStack->elementCount; i ++ )
	{
		if(ArrayList_contains(c, pStack->elementData[i]))
		{
			IdentifyStack_removeElementAt(pStack, i);
			i --;
			modified = true;
		}
	}


	return modified;
}


int main(int argc, char ** argv)
{
	int size = atoi(argv[1]);
	int i;

	IdentifyStack * stack = (IdentifyStack *)malloc(sizeof(IdentifyStack));
	IdentifyStack_IdentifyStack0(stack);

	for(i = 0; i < size; i ++)
	{
		IdentifyStack_push(stack, i);
	}

	ArrayList * list = (ArrayList *)malloc(sizeof(ArrayList));
	ArrayList_ArrayList0(list);

	for(i = size ; i < 2 * size; i ++ )
	{
		ArrayList_add1(list, i);
	}

	IdentifyStack_removeAll(stack, list);
	//IdentifyStack_print(stack);
	IdentifyStack_destroy(stack);
	free(stack);
	ArrayList_destroy(list);
	free(list);
}