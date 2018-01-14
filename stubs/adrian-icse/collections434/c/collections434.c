#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <limits.h>

typedef struct stListEntry
{
	int element;
	struct stListEntry * next;
	struct stListEntry * prev;

} ListEntry;

void ListEntry_ListEntry(ListEntry * pEntry, int element, ListEntry * next, ListEntry * previous)
{
	pEntry->element = element;
	pEntry->next = next;
	pEntry->prev = previous;
}

typedef struct stLinkedList
{
	ListEntry * header;
	int size ;
} LinkedList;

void LinkedList_LinkedList(LinkedList * pList)
{
	pList->header = (ListEntry *)malloc(sizeof(ListEntry));
	pList->header->next = pList->header->prev = pList->header;
	pList->size = 0;
}

ListEntry * LinkedList_addBefore( LinkedList * pList,  int e, ListEntry * entry)
{
	ListEntry * newEntry = (ListEntry *)malloc(sizeof(ListEntry));
	ListEntry_ListEntry(newEntry, e, entry, entry->prev);
	newEntry->prev->next = newEntry;
	newEntry->next->prev = newEntry;
	pList->size ++;
	return newEntry;
}

ListEntry * LinkedList_addAfter(LinkedList * pList, int e, ListEntry * entry)
{

}

bool LinkedList_isEmpty(LinkedList * pList)
{
	return pList->size == 0;
}

bool LinkedList_add(LinkedList * pList , int e)
{
	LinkedList_addBefore(pList, e, pList->header);
	return true;
}

void LinkedList_addLast(LinkedList * pList, int e)
{
	LinkedList_addBefore(pList, e, pList->header);
}

void LinkedList_print(LinkedList * pList)
{
	ListEntry * e ;

	for(e = pList->header->next; e != pList->header; e = e->next)
	{
		printf("%d ", e->element);
	}

	printf("\n");
}

void LinkedList_destroy(LinkedList * pList)
{
	ListEntry * e = pList->header->next;
	ListEntry * next;

	while(e!= pList->header)
	{
		next = e->next;
		free(e);
		e = next;
	}

	free(pList->header);
}

int LinkedList_indexOf(LinkedList * pList, int o)
{
	ListEntry * e;
	int index = 0;

	for(e = pList->header->next; e != pList->header; e = e->next)
	{
		if( o == e->element)
		{
			return index;
		}

		index ++;
	}

	return -1;
}	

bool LinkedList_contains(LinkedList * pList, int o)
{
	return LinkedList_indexOf(pList, o);
}

typedef struct stNode
{	
	struct stNode * previous;
	struct stNode * next;
	int value;

} Node;

void Node_Node0( Node * pNode )
{
	pNode->previous = pNode;
	pNode->next = pNode;
}

void Node_Node1( Node * pNode, int value )
{
	pNode->value = value;
}

void Node_Node3( Node * pNode, Node * previous, Node * next, int value)
{
	pNode->previous = previous;
	pNode->next = next;
	pNode->value = value;
}

int Node_getValue(Node * pNode)
{
	return pNode->value;
}

void Node_setValue(Node * pNode, int value)
{
	pNode->value = value;
}

Node * Node_getPreviousNode(Node * pNode)
{
	return pNode->previous;
}

void Node_setPreviousNode(Node * pNode, Node * previous)
{
	pNode->previous = previous;
}

Node * Node_getNextNode( Node * pNode)
{
	return pNode->next;
}

void Node_setNextNode(Node * pNode, Node * next)
{
	pNode->next = next;
}

typedef struct stCursorableLinkedList
{
	Node * header;
	int size;
} CursorableLinkedList;


void CursorableLinkedList_CursorableLinkedList(CursorableLinkedList * pList)
{
	pList->header = (Node *)malloc(sizeof(Node));
	Node_Node0(pList->header);
	pList->size = 0;
}

void CursorableLinkedList_destroy(CursorableLinkedList * pList)
{
	Node * e = pList->header->next;
	Node * next ;

	while(e != pList->header)
	{
		next = e->next;
		free(e);
		e = next;
	}

	free(pList->header);
}


Node * CursorableLinkedList_getNode(CursorableLinkedList * pList, int index, bool endMarkerAllowed)
{
	int currentIndex;
	Node * node;
	if(index < pList->size / 2 )
	{
		node = pList->header->next;
		for(currentIndex = 0; currentIndex < index ; currentIndex ++ )
		{
			node = node->next;
		}
	}
	else
	{
		node = pList->header;
		for(currentIndex = 0; currentIndex > index ; currentIndex --)
		{
			node = node->previous;
		}
	}

	return node;
}

void CursorableLinkedList_addNode(CursorableLinkedList * pList, Node * nodeToInsert, Node * insertBeforeNode)
{
	nodeToInsert->next = insertBeforeNode;
	nodeToInsert->previous = insertBeforeNode->previous;
	insertBeforeNode->previous->next = nodeToInsert;
	insertBeforeNode->previous = nodeToInsert;
	pList->size ++;
}

void CursorableLinkedList_removeNode(CursorableLinkedList * pList, Node * node)
{
	node->previous->next = node->next;
	node->next->previous = node->previous;
	pList->size --;
}

void CursorableLinkedList_addNodeAfter(CursorableLinkedList * pList, Node * node, int value)
{
	Node * newNode = (Node *)malloc(sizeof(Node));
	Node_Node1(newNode, value);
	CursorableLinkedList_addNode(pList, newNode, node->next);
}

void CursorableLinkedList_addNodeBefore(CursorableLinkedList * pList, Node * node, int value)
{
	Node * newNode = (Node *)malloc(sizeof(node));
	Node_Node1(newNode, value);
	CursorableLinkedList_addNode(pList, newNode, node);
}

bool CursorableLinkedList_addLast(CursorableLinkedList * pList, int o)
{
	CursorableLinkedList_addNodeBefore(pList, pList->header, o);
	return true;
}

bool CursorableLinkedList_add(CursorableLinkedList * pList, int value)
{
	CursorableLinkedList_addLast(pList, value);
	return true;
}

int CursorableLinkedList_indexOf(CursorableLinkedList * pList, int value)
{
	int i = 0;
	Node * node;

	for( node = pList->header->next; node != pList->header; node = node->next)
	{
		if(value == node->value)
		{
			return i;
		}

		i ++;
	}

	return -1;
}

bool CursorableLinkedList_contains(CursorableLinkedList * pList, int value)
{
	return CursorableLinkedList_indexOf(pList, value) != -1;
}

bool CursorableLinkedList_containsAll(CursorableLinkedList * pList, LinkedList * coll)
{
	ListEntry * entry;

	for(entry = coll->header->next; entry != coll->header; entry = entry->next)
	{
		if(!CursorableLinkedList_contains(pList, entry->element))
		{
			return false;
		}
	}

	return true;
}

int main(int argc, char ** argv)
{
	int i = 0;
	int size = atoi(argv[1]);

	CursorableLinkedList * cursorable = (CursorableLinkedList *)malloc(sizeof(CursorableLinkedList));
	CursorableLinkedList_CursorableLinkedList(cursorable);

	for(i = 0; i < size; i ++ )
	{
		CursorableLinkedList_add(cursorable, i);
	}

	LinkedList * list = (LinkedList *)malloc(sizeof(LinkedList));
	LinkedList_LinkedList(list);

	for(i = 0; i < size; i ++ )
	{
		LinkedList_add(list, i);
	}

	bool flag = CursorableLinkedList_containsAll(cursorable, list);

	printf("%d\n", flag);

	LinkedList_destroy(list);
	free(list);
	CursorableLinkedList_destroy(cursorable);
	free(cursorable);
}