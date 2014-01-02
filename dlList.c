/*
 * dlList.c
 *
 *  Created on: Nov 17, 2013
 *      Author: poppe
 */
#include <dlList.h>
#include <string.h>
#include <rtdebug.h>

#define DLL_ASSERT(x) RT_ASSERT(x)

/**
 * @brief Initilize Double Linked List with know list type
 * @param[in] list to initilize
 * @param[in] type of list 
 * @return Error code
 **/
int dllist_initList(List_t *list, int type)
{
	DLL_ASSERT(list);
	//set list memory all to zero
	memset(list, '\0', sizeof(struct List));
	list->type = type;
	return 0;
}

/**
 * @brief Allocates memory for a new Node
 * @param[out] list to initilize
 * @return Error code
 **/
int dllist_newNode(Node_t **node)
{
	DLL_ASSERT(node);

	*node = DLL_MALLOC(sizeof(struct Node));

	if(!*node)
		return -1;
		
	memset(*node, '\0', sizeof(struct Node));
	
	return 0;
}

int dllist_destroyNode(Node_t *node)
{
	DLL_ASSERT(node);

	DLL_FREE(node);
	return 0;
}

Node_t *dllist_findNodeWithData(List_t *list, void *data)
{
	DLL_ASSERT(list);
	DLL_ASSERT(data);
	
	Node_t *tempNode = list->first;
	while(tempNode)
	{
		if(data == tempNode->data)
			return tempNode;

		tempNode = tempNode->next;
	}
	return NULL;
}

Node_t *dllist_getNodeAt(List_t *list, int pos)
{
	DLL_ASSERT(list);
	
	Node_t *tempNode;
	
	if(pos > list->len)
		return NULL;

	tempNode = list->first;
	pos--;
	while(pos && tempNode)
	{
		tempNode = tempNode->next;
		pos--;
	}
	
	return tempNode;
}

void *dllist_getDataAt(List_t *list, int pos)
{
	DLL_ASSERT(list);
	void *data;
	Node_t *tempNode = dllist_getNodeAt(list, pos);

	if(tempNode == NULL)
		return NULL;
	data = tempNode->data;

	return data;
}

Node_t *dllist_addData(List_t *list, void *data)
{
	DLL_ASSERT(list);
	DLL_ASSERT(data);
	
	Node_t *newNode;
	dllist_newNode(&newNode);
	
	newNode->data = data;
	
	dllist_addNode(list, newNode);

	return newNode;
	
}

int dllist_addNode(List_t *list, Node_t *node)
{
	DLL_ASSERT(list);
	DLL_ASSERT(node);
	
	if(list->type == DLL_FIFO)
		return dllist_addNodeToEnd(list, node);
	
	return dllist_addNodeToFront(list, node);
}

	
int dllist_insertBefore(List_t *list, Node_t *node, Node_t *newNode)
{
	DLL_ASSERT(list);
	DLL_ASSERT(node);
	DLL_ASSERT(newNode);
	
	Node_t *tempNode;
	
	tempNode = node->prev;
	tempNode->next = newNode;
	newNode->prev = tempNode;
	node->prev = newNode;
	newNode->next = node;

	list->len++;
	return 0;
}

int dllist_addNodeToEnd(List_t *list, Node_t *node)
{
	DLL_ASSERT(list);
	DLL_ASSERT(node);
	
	Node_t *tempNode;
	if(list->len == 0)
	{
		list->first = node;
		list->last = node;
		list->len++;
		return 0;
	}
	tempNode = list->last;
	tempNode->next = node;
	
	node->prev = tempNode;
	
	list->last = node;
	list->len++;

	return 0;
}

int dllist_addNodeToFront(List_t *list, Node_t *node)
{
	DLL_ASSERT(list);
	DLL_ASSERT(node);
	
	Node_t *tempNode;
	
	if(list->len == 0)
	{
		list->first = node;
		list->last = node;
		list->len++;
		return 0;
	}
	tempNode = list->first;
	tempNode->prev = node;
	
	node->next = tempNode;
	
	list->first = node;
	list->len++;
	
	return 0;
}

int dllist_removeNode(List_t *list, Node_t *node, int freeNode)
{
	DLL_ASSERT(list);
	DLL_ASSERT(node);
	
	Node_t *tempNode;
	
	if(!(dllist_NodeExsist(list, node)))
		return -1;
	
	tempNode = node->prev;
	if(tempNode)
	{
		tempNode->next = node->next;
		if(node->next)
			node->next->prev = tempNode;
	}
	list->len--;
	
	if(list->first == node)
		list->first = node->next;

	if(list->last == node)
		list->last = node->prev;

	if(freeNode)
		dllist_destroyNode(node);
	
	return 0;
}

int dllist_removeNodeWithData(List_t *list, void *data, int freeNode)
{
	DLL_ASSERT(list);
	DLL_ASSERT(data);

	Node_t *tempNode = dllist_findNodeWithData(list, data);

	if(tempNode)
		dllist_removeNode(list, tempNode, freeNode);
	//TODO: I have no inditcaton if we don't find the node... need to fix
	return 0;
}
int  dllist_listLen(List_t *list)
{
	DLL_ASSERT(list);
	
	return list->len;
}

void dllist_foreachNode(List_t *list, void(*prototype)(Node_t *node, void *arg), void *arg)
{
	DLL_ASSERT(list);
	DLL_ASSERT(prototype);
	
	int len = list->len;
	Node_t *tempNode = list->first;
	Node_t *nextTempNode;
	while(tempNode)
	{
		nextTempNode = tempNode->next;
		prototype(tempNode, arg);
		tempNode = nextTempNode;
	}
}

void dllist_foreachData(List_t *list, void(*prototype)(void *data, void *arg), void *arg)
{
	DLL_ASSERT(list);
	DLL_ASSERT(prototype);

	int len = list->len;
	Node_t *tempNode = list->first;
	Node_t *nextTempNode;
	while(tempNode)
	{
		/*
		 * I keep the next temp node so if the prototype deletes the node we can still move on the list
		 * I am not sure that I need this but it can't hurt
		 */
		nextTempNode = tempNode->next;
		prototype(tempNode->data, arg);
		tempNode = nextTempNode;
	}
}

void dllist_foreachNodeWithBreak(List_t *list, int(*prototype(void*, void*)), void *arg)
{
	DLL_ASSERT(list);
	DLL_ASSERT(prototype);

	int len = list->len, loopBreak = 0;
	Node_t *tempNode = list->first;

	while(tempNode && !loopBreak)
	{
		loopBreak = prototype(tempNode->data, arg);
		tempNode = tempNode->next;
	}
}

void *dllist_searchWithinData(List_t * list, int(*prototype(void *, void*)), void *arg)
{
	DLL_ASSERT(list);
	DLL_ASSERT(prototype);

	int len = list->len, loopBreak = 0;
	Node_t *tempNode = list->first;

	while(tempNode)
	{
		loopBreak = prototype(tempNode->data, arg);

		if(loopBreak)
			return tempNode->data;

		tempNode = tempNode->next;
	}

	return NULL;
}

int dllist_swapData(Node_t *f, Node_t *s)
{
	DLL_ASSERT(f);
	DLL_ASSERT(s);
	
	void *tempData = f->data;
	f->data = s->data;
	s->data = tempData;
	
	return 0;
}
	
	

int dllist_NodeExsist(List_t *list, Node_t *node)
{
	DLL_ASSERT(list);
	DLL_ASSERT(node);
	int len = list->len;
	Node_t *tempNode = list->first;
	
	while(len && tempNode)
	{
		if(tempNode == node)
			return -1;
			
		len--;
	}
	
	return 0;
}
