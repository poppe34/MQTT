/*
 * dlList.h
 *
 *  Created on: Nov 17, 2013
 *      Author: poppe
 */

#ifndef DLLIST_H_
#define DLLIST_H_
#include "rtthread.h"

/*** STUCTURES  ENUMS  and UNIONS   ***/
enum dllTypes
{
	DLL_FIFO,
	DLL_FIL0,
};

struct Node
{
	struct Node *next;
	struct Node *prev;
	void *		data;
};

struct List
{
	int type;
	int len;
	struct Node *current;
	struct Node *first;
	struct Node *last;
};

/***    TYPEDEFS   ***/
typedef struct Node 	Node_t;
typedef struct List 	List_t;

/***   DEFINES   ***/
#define DLL_MALLOC(size)	rt_malloc(size);
#define DLL_FREE(data)		rt_free(data);

/***	PROTOTYPES 	***/
int  dllist_initList(List_t *list, int type);
int  dllist_newNode(Node_t **node);
int  dllist_destryNode(Node_t *node);
Node_t * dllist_getNodeAt(List_t *list, int pos);
void * dllist_getDataAt(List_t *list, int pos);
Node_t *dllist_addData(List_t *list, void *data);
int  dllist_addNode(List_t *list, Node_t *node);
int  dllist_insertBefore(List_t *list, Node_t *node, Node_t *newNode);
int  dllist_addNodeToEnd(List_t *list, Node_t *node);
int  dllist_addNodeToFront(List_t *list, Node_t *node);
int dllist_removeNode(List_t *list, Node_t *node, int freeNode);
int  dllist_listLen(List_t *list);
//void dllist_foreachNode(List_t *list, int (*prototype(void *, void *)), void *arg)
int  dllist_findData(List_t list, void *data, int (*prototype(void *first, void *second)));
int  dllist_NodeExsist(List_t *list, Node_t *node);
void *dllist_searchWithinData(List_t * list, int(*prototype(void *, void*)), void *arg);
void dllist_foreachNode(List_t *list, void(*prototype)(Node_t *node, void *arg), void *arg);
#endif /* DLLIST_H_ */
